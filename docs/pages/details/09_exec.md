\page exec Exécution
\tableofcontents

# Exécution de commandes userland

Dans Epirootkit, l'exécution de commandes userland est gérée par le module `exec.c`.

L'interêt est immédiat : il permet d'exécuter des commandes shell en mode root sur la machine, ce qui ouvre un contrôle total sur le système.

> **Attention :** Ce module ne permet pas d'obtenir un shell distant interactif, mais uniquement d'exécuter des commandes à la façon de scripts shell. Toutefois, comme expliqué dans la section [Reverse Shell](#reverse-shell-doc), cette fonctionnalité est aussi en effet utilisée dans Epirootkit pour lancer un reverse shell en userland avec`socat`.

# Cahier des charges

Dans notre contexte d'exécution de commandes à distance par un attaquant, le module `exec.c` devait répondre à plusieurs critères :
- **Exécution de commandes shell** : Le module doit pouvoir exécuter n'importe quelle commande shell, comme si l'utilisateur était connecté en tant que root.
- **Récupération des résultats** : Les résultats de l'exécution des commandes doivent être renvoyés à l'attaquant, permettant ainsi de récupérer la sortie standard, les erreurs et le code de retour.
- **Gestion des redirections manuelles** : Si l'utilisateur spécifie manuellement des redirections (par exemple, `> output.txt`), le module doit les gérer correctement afin qu'elles ne rentrent pas en conflit avec la récupération des résultats précédemment mentionnée.
- **Gestion des commandes bloquantes** : L'execution de commandes doit embarquer un timeout pour éviter les blocages indéfinis. Si une commande dépasse le temps imparti, elle doit être interrompue et un message d'erreur doit être renvoyé à l'attaquant. Cela est nécessaire puisque lors de l'envoi d'une commande, le server d'attaque attend une réponse du module, et si la commande est bloquante, le serveur ne recevra jamais de réponse et restera en attente indéfiniment.

# Implémentation

Le module `exec.c` repose sur l'utilisation de l'API `call_usermodehelper` du noyau Linux pour exécuter des commandes shell depuis l’espace kernel. Cela permet à Epirootkit de déclencher l'exécution de n'importe quelle commande en tant que root, avec une gestion fine du comportement (redirection, timeout, etc.). Voici comment chaque exigence du cahier des charges est implémentée :

## Exécution de commandes shell

La fonction principale, `exec_str_as_command_with_timeout()`, reçoit une chaîne de commande utilisateur (`user_cmd`), applique diverses préparations, puis déclenche son exécution à l’aide de la fonction `call_usermodehelper_exec()` via :

```c
char *argv[] = { "/bin/sh", "-c", (char *)cmd_str, NULL };
```

Cela revient à faire :
```sh
/bin/sh -c "<commande>"
```

Mais cette commande est lancée depuis l’espace noyau.

## Récupération des résultats

La récupération standard de sortie (`stdout`) et d’erreur (`stderr`) est assurée par des fichiers de redirection définis globalement, typiquement :
```c
#define STDOUT_FILE HIDDEN_DIR_PATH "/std.out"
#define STDERR_FILE HIDDEN_DIR_PATH "/std.err"
```

Si l’utilisateur n’a pas lui-même inclus des redirections (`>` ou `2>`), le module redirige manuellement ces flux vers les fichiers ci-dessus. Cela permet à l’attaquant de récupérer plus tard les résultats de l'exécution.

Le code de gestion est situé dans la fonction `build_full_command()`, qui assemble la commande finale en fonction de l’état des redirections détectées par `detect_redirections()`.

## Gestion des redirections manuelles

La fonction `detect_redirections()` scrute la commande utilisateur pour déterminer si des redirections explicites sont déjà présentes. Elle repère les motifs `>` (stdout) et `2>` (stderr). Si l'utilisateur a déjà pris en charge les redirections, le module n’ajoute pas les siennes, ce qui évite toute redondance conflictuelle :

```c
*redirect_stderr = (strstr(cmd, "2>") != NULL);
*redirect_stdout = (strstr(cmd, ">") != strstr(cmd, "2>") && strstr(cmd, ">") != NULL);
```

## Gestion des commandes bloquantes (timeout)

Un élément essentiel pour éviter les blocages est le mécanisme de timeout. Le module construit un préfixe de commande `timeout` grâce à `build_timeout_prefix()` :

```c
timeout --signal=SIGKILL --preserve-status <seconds>
```

Ce préfixe est inséré automatiquement dans la commande shell finale. Ainsi, si la commande ne termine pas dans le délai imparti, elle est tuée avec `SIGKILL`, et le code de retour est conservé. Exemple de commande générée :

```sh
timeout --signal=SIGKILL --preserve-status 5 ls -la > HIDDEN_DIR_PATH"/std.out" 2> HIDDEN_DIR_PATH"/std.err"
```

## Résumé du pipeline d’exécution

1. Nettoyage de la commande : suppression des espaces en tête avec `trim_leading_whitespace()`.
2. Analyse des redirections : détection de `>` et `2>` via `detect_redirections()`.
3. Préfixe timeout : si un timeout est demandé, `build_timeout_prefix()` construit la commande.
4. Construction de la commande complète : via `build_full_command()`, en intégrant :
   - Timeout
   - Redirections (automatiques ou manuelles)
5. Exécution : déclenchée via `call_usermodehelper_exec()`.

## Avantages de cette approche

- Séparation claire des responsabilités : chaque aspect (timeout, redirection, parsing) est géré par une fonction spécifique.
- Robustesse contre les commandes piégeuses : les redirections explicites sont respectées.
- Prévention des zombies : grâce à `timeout`, aucun risque de laisser des processus bloquants/infinis dans des threads en arrière-plan.
- Exécution root transparente : la commande s’exécute dans le contexte privilégié du noyau, offrant un contrôle total sur le système.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>