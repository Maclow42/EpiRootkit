# Utilisation
\tableofcontents

L’utilisation du rootkit peut se faire soit en ligne de commande, soit via l’interface graphique intégrée. Nous allons ici décrire les deux méthodes.

> Pour tirer pleinement parti de cette section, veuillez d’abord vous assurer que la mise en place a été correctement effectuée, comme décrit précédemment dans la section [Mise en place](02_install.md).

Dans un premier temps, nous expliquerons comment interagir avec l’interface web. Ensuite, nous détaillerons l’ensemble des commandes de base permettant d’interagir à distance avec le rootkit, accessibles aussi bien en ligne de commande (CLI) côté attaquant qu’à travers des boutons ou des champs de saisie dans l’interface graphique.

## 🌐 Interface Web

### 1. Connexion

Normalement, à ce stade, vous devriez avoir les deux machines virtuelles ouvertes, avec le serveur Python en cours d’exécution, ainsi que l’interface web si vous avez choisi cette option. Vous devriez alors voir un écran similaire à celui présenté ci-dessous. Le rootkit est détecté et connecté, mais une authentification est nécessaire pour accéder à l’ensemble des fonctionnalités et contrôler la machine victime à distance. Cliquez ensuite sur Authenticate et saisissez le mot de passe `evannounet`. Après quelques instants, le tableau de bord principal de l’application devrait s’afficher.
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

### 2. Dashboard

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

Une fois connecté et authentifié, le tableau de bord principal (dashboard) s’affiche. Il permet un aperçu global de l’état de la machine cible, ainsi qu’un accès rapide à certaines fonctionnalités du rootkit. Voici les différents éléments présents ci-dessous.

#### ✅ Connexion
<div class="full_width_table">
| Élément             | Description                                                                 |
|:---------------------|:-----------------------------------------------------------------------------|
| **Status**          | Indique que le rootkit est bien connecté à la machine cible.                |
| **IP**              | Adresse IP locale de la machine cible (ici `192.168.100.3`).                |
| **Port**            | Port utilisé pour la connexion (ici `4242`).                                |
| **Time**            | Heure actuelle sur la machine victime.                                      |
| **Last Command**    | Affiche la dernière commande envoyée à la cible.                            |
| **Avertissement VM**| Affiche un message d’alerte si le rootkit détecte un environnement virtuel. |
</div>

#### 🛠 Actions
<div class="full_width_table">
| Action              | Description                                                                 |
|:---------------------|:-----------------------------------------------------------------------------|
| **Disconnect**      | Permet de fermer proprement la connexion avec la cible.                     |
| **Kill rootkit**    | Met un terme à l’exécution du rootkit sur la machine cible                  |
</div>

#### 🖥 Système
<div class="full_width_table">
| Élément             | Description                                                                 |
|:---------------------|:-----------------------------------------------------------------------------|
| **Architecture**    | Architecture processeur de la machine (ici `x86_64`).                       |
| **CPU Cores**       | Nombre de cœurs processeur détectés (ici `1 cœur`).                         |
| **CPU Model**       | Nom du processeur ou de l’émulateur utilisé (ici `QEMU Virtual CPU`).       |
| **Hostname**        | Nom d’hôte de la machine (ici `victim`).                                    |
| **RAM**             | Quantité totale de RAM disponible (ici `3889 Mo`).                          |
| **Version kernel**  | Version du noyau Linux (ici `6.8.0-60-generic`).                            |
| **Version OS**      | Détail de la distribution Linux et son build.                               |
| **Virtual Env**     | Indique si la machine semble tourner dans une VM (ici `true`).              |
</div>

#### 💻 Shell
<div class="full_width_table">
| Élément             | Description                                                                 |
|:---------------------|:-----------------------------------------------------------------------------|
| **Champ de port**   | Permet de spécifier un port sur lequel ouvrir un shell inversé.             |
| **Launch Shell**    | Lance le shell distant sur le port défini.                                  |
</div>

> **Attention :** Le bouton *Launch Shell* ouvre un terminal sur la machine d'attaque. Par conséquent, le serveur web **et** le navigateur doivent être lancés dans la VM d'attaque pour que cette fonctionnalité fonctionne correctement.

#### 💾 Disque

Un aperçu est fourni via la commande distante `df -h` exécutée sur la machine virtuelle victime, indiquant les différentes partitions, leur taille, l’espace utilisé/disponible et leur point de montage.

#### 📊 Ressources

Un petit graphique à droite affiche en temps réel :
<div class="full_width_table">
| Ressource           | Couleur d'affichage  |
|:--------------------|:---------------------|
| **CPU (%)**         | Rouge                |
| **RAM (%)** n       | Bleu                 |
</div>

### 3. Terminal

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/terminal.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the attacker's terminal interface for remote command execution.
  </figcaption>
</figure>
\endhtmlonly

L’onglet **Terminal** permet de prendre le contrôle de la machine cible à distance en exécutant des commandes comme si l’on utilisait un terminal local.

#### 🔹 Interface utilisateur

- **Champ de commande** : Saisissez ici une commande Unix/Linux classique. Par exemple : `ls -la`, `cat /etc/passwd`, `whoami`, etc.
- **Bouton “Send”** : Permet d’envoyer la commande à la machine cible.
- **Mode d’envoi (TCP/DNS)** :
  - **TCP** (par défaut) : Les données sont envoyées sur une connexion directe. Un timeout de 5 secondes est implémenté côté rootkit pour éviter qu’une commande comme `ping` ne bloque indéfiniment.
  - **DNS** : Permet d’envoyer les commandes via des requêtes DNS (plus discret). Le timeout est de 30 secondes. Les commandes dont la réponse semble trop longue sont automatiquement interrompues avant l’envoi.

#### 🔹 Résultats de la commande

Les résultats de l'exécution apparaissent dans deux blocs distincts :
- **stdout (standard output)** : Affiche le contenu de la sortie standard de la commande.
- **stderr (standard error)** : Affiche le contenu de la sortie d’erreur de la commande.
- **Code de sortie** : Affichage du code de terminaison.

#### 🔹 Historique des commandes

En bas de l’écran, une section "Command history" permet de retrouver :
- Les commandes précédemment envoyées.
- Leur résultat, sous forme repliable pour chaque entrée.
- Ceci facilite le débogage ou la réutilisation de commandes courantes.

Cette fonctionnalité est utile pour :
- Effectuer un audit du système distant.
- Modifier des fichiers ou exécuter des scripts malveillants.
- Mettre en œuvre des actions de persistance ou de nettoyage après compromission.

### 4. Keylogger

FIXME

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/keylogger.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the keylogger interface in the attacker's web UI.
  </figcaption>
</figure>
\endhtmlonly

L’onglet **Keylogger** permet de récupérer les frappes clavier effectuées sur la machine victime. Cette fonctionnalité est particulièrement utile pour collecter des mots de passe, des requêtes tapées dans un navigateur, ou encore pour surveiller l’activité de la victime.

#### 🔹 Affichage des frappes

- Une zone de texte centrale affiche le contenu capturé sous forme brute (sans mise en forme), comme stocké par le module de keylogging sur la machine cible.
- Le bouton **Fetch data** permet de récupérer les nouvelles frappes depuis le module rootkit.
- L’état du module est affiché via un **interrupteur ON/OFF** :
  - Quand le module est actif, les frappes sont enregistrées.
  - Quand il est désactivé, aucune frappe n’est collectée.

#### 🔹 Recherche dans les frappes

- Un champ de recherche permet de filtrer les résultats affichés :
  - **Mode Normal** : la recherche est effectuée en texte brut.
  - **Mode RegEx** : active une recherche utilisant des expressions régulières.
- Le bouton **Search** permet d’appliquer le filtre sur les données affichées.

#### 🔹 Exportation

- Le bouton **Download as .txt** permet de télécharger l’ensemble des frappes capturées sous forme d’un fichier `.txt`, pour une analyse hors-ligne ou un archivage.

Cette interface permet donc une surveillance continue et discrète du poste compromis, tout en offrant des outils de recherche et d’exportation pratiques pour l’attaquant.

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
Cette commande permet de masquer un fichier ou un dossier spécifique en fournissant son chemin absolu. En arrière-plan, le syscall `getdents64` est intercepté afin de filtrer le contenu affiché lors de l’énumération des fichiers. De plus, par défaut, tout fichier commençant par `stdbool_bypassed_ngl_` sera automatiquement caché.

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
Cette commande permet de cacher des ports, notamment dans les fichiers `/proc/net/tcp`... Elle modifie également le comportement de binaires comme `ss` ou `netstat`, en masquant toutes les lignes mentionnant un port source ou destination égal à `[PORT]`.

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