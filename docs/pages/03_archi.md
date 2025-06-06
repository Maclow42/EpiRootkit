# Architecture

\tableofcontents

## 1. 🌳 Arborescence

```bash
.
├── attacker
├── boot
├── docs
├── rootkit
├── AUTHORS
├── README
└── TODO
```

Ci-dessus se trouve l'architecture globale du dépôt Git. Il contient à la fois le code côté victime et côté attaquant, ainsi que des scripts pour l'installation des machines virtuelles, et enfin la documentation présente. Les différentes parties sont détaillées dans les sections suivantes.
- `attacker` contient les fichiers relatifs au serveur attaquant.
- `boot` contient les scripts liés à la mise en place du projet.
- `docs` regroupe les fichiers de configuration utilisés pour générer la présente documentation.
- `rootkit` contient le code source du rootkit.
- `AUTHORS` répertorie les auteurs du projet.
- `README` explique simplement comment générer la documentation.
- `TODO` est un fichier utilitaire destiné au suivi et au développement du projet.

## 2. 💀 Attacker
```bash
attacker/
├── routes/                         # Contient les différentes routes (endpoints) de l'application Flask
│   ├── api.py                      # API de communication entre le serveur et les clients infectés
│   ├── dashboard.py                # Routes pour afficher le tableau de bord (informations système, etc.)
│   ├── download.py                 # Gère les téléchargements de fichiers depuis les clients
│   ├── explorer.py                 # Permet d'explorer les fichiers distants (navigateur de fichiers)
│   ├── keylogger.py                # Affiche les données du keylogger récupérées des clients
│   ├── terminal.py                 # Interface pour exécuter des commandes shell à distance
│   └── upload.py                   # Gère l'envoi de fichiers vers les machines infectées
├── static/                         # Fichiers statiques (CSS, JS, images, sons, etc.)
│   ├── downloads/                  # Dossier où les fichiers récupérés des clients sont stockés
│   ├── fonts/                      # Polices utilisées dans l'interface graphique
│   ├── img/                        # Images statiques
│   ├── js/                         # Scripts JavaScript pour le front-end (dashboard, terminal, etc.)
│   ├── sounds/                     # Effets sonores pour l’interface utilisateur (eheh)
│   ├── uploads/                    # Fichiers à envoyer vers les clients
│   └── style.css                   # Feuille de style principale de l'application web
├── templates/                      # Templates HTML pour l’interface web
│   ├── partials/                   # Fragments HTML réutilisables (sidebar, header, cartes info, etc.)
│   ├── base.html                   # Template de base (structure commune à toutes les pages)
│   ├── dashboard.html              # Vue principale du tableau de bord
│   ├── download.html               # Vue pour gérer les téléchargements
│   ├── explorer.html               # Vue de l'explorateur de fichiers
│   ├── keylogger.html              # Vue pour afficher les frappes clavier capturées
│   ├── terminal.html               # Interface web du terminal distant
│   └── upload.html                 # Vue pour envoyer des fichiers vers les clients
├── uploads/                        # Contient les fichiers à transférer vers la victime
├── utils/                          # Fonctions utilitaires pour la communication et le chiffrement
│   ├── Crypto/
│   │   └── CryptoHandler.py        # Gère le chiffrement/déchiffrement (AES)
│   ├── DNS/
│   │   └── DNSSender.py            # Communication DNS
│   ├── TCP/
│   │   ├── AESNetworkHandler.py    # Gère les connexions réseau TCP chiffrées avec AES
│   │   └── TCPServer.py            # Serveur TCP pour recevoir les connexions des clients
│   ├── BigMama.py                  # Gestionnaire communications principales (TCP et DNS)
│   └── socat.py                    # Intégration de l'outil socat
├── app.py                          # Initialisation de l'application Flask (création de l’objet app)
├── config.py                       # Configuration de l’application
├── main.py                         # Point d’entrée principal du programme
├── requirements.txt                # Dépendances Python nécessaires au projet
└── server.pem                      # Certificat pour socat

```

## 3. 🛠️ Rootkit
```bash
rootkit
├── include/                        # Fichiers d’en-tête globaux
│   ├── cmd.h                       # Déclarations liées aux commandes internes du rootkit
│   ├── config.h                    # Définition des macros de configuration
│   ├── download.h                  # Fonctions pour gérer le téléchargement de fichiers
│   ├── epirootkit.h                # En-tête principal du rootkit
│   ├── socat.h                     # Intégration de `socat`
│   ├── upload.h                    # Fonctions pour gérer l’upload de fichiers
│   └── vanish.h                    # Fonctions pour détecter l'environnement virtuel
├── interceptor/                    # Partie responsable des hooks et du contournement du noyau
│   ├── core/                       # Mécanismes internes comme ftrace...
│   │   ├── include/                # Headers spécifiques au cœur du module
│   │   │   ├── ftrace.h            # Déclarations pour l’utilisation de `ftrace`
│   │   │   ├── init.h              # Déclarations pour l’initialisation de l'interceptor
│   │   │   └── menu.h              # Déclarations pour le hooks menu
│   │   ├── array.c                 # Gestion des tableaux dynamiques de hooks
│   │   ├── ftrace.c                # Implémentation du mécanisme ftrace
│   │   ├── init.c                  # Module init/exit avec fichiers par défaut.
│   │   └── menu.c                  # Menu pour ajouter/enlever des fichiers à traiter
│   ├── hooks/
│   │   ├── alterate/               # Module d’altération
│   │   │   ├── alterate_api.c/h
│   │   │   ├── alterate.c/h
│   │   ├── forbid/                 # Module d’interdiction
│   │   │   ├── forbid_api.c/h
│   │   │   ├── forbid.c/h
│   │   └── hide/                   # Module de camouflage
│   │       ├── hide_api.c/h
│   │       ├── hide.c/h
│   └── misc/
│       └── ghost.c                 # Masquer la présence du module .ko
├── network/                        # Modules de communication réseau
│   ├── core/
│   │   ├── network.c/h             # Fonctionnalités générales réseau
│   └── protocols/                  # Protocoles supportés par le rootkit
│       ├── dns/                    # Communication via DNS (tunneling, exfiltration)
│       │   ├── dns.c               # Fonctions coeurs
│       │   └── worker.c            # Thread pour gérer les requêtes DNS
│       └── tcp/                    # Communication TCP
│           ├── socket.c            # Création et gestion des sockets TCP
│           └── worker.c            # Thread pour gérer les requêtes TCP
├── passwd/                         # Module pour interagir avec les credentials du rootkit
│   ├── passwd.c
│   └── passwd.h
├── scripts/
│   ├── romance/
│   │   └── initrd.sh               # Script pour injecter le rootkit dans un initrd
│   ├── format.sh                   # Nettoyage / formatage des fichiers
│   └── generate.sh                 # Génération socat.h
├── utils/                          # Fonctions utilitaires partagées
│   ├── crypto/                     # Chiffrement et hash
│   │   ├── aes.c                   # Implémentation d’AES
│   │   ├── crypto.h
│   │   └── hash.c                  # Fonctions de hachage
│   ├── io/                         # Entrée / sortie bas-niveau (read and write, basically)
│   │   ├── io.c
│   │   └── io.h
│   ├── sysinfo/                    # Récupération d’informations système pour la UI attaquante
│   │   ├── sysinfo.c
│   │   └── sysinfo.h
│   └── ulist/                      # Linked lists globales pour les hooks avec persistance
│       ├── ulist.c
│       └── ulist.h
├── cmd.c                           # Toutes les commandes disponibles côté rootkit
├── download.c                      # Téléchargement de fichiers depuis l'attaquant
├── epikeylog.c                     # Keylogger (capture des frappes clavier)
├── main.c                          # Point d’entrée principal de l’exécutable rootkit
├── socat                           # Binaire `socat`
├── socat.c                         # Code source associé à l’utilisation de socat
├── upload.c                        # Envoi de fichiers vers le serveur distant
├── userland.c                      # Code exécuté en espace utilisateur (surtout pour exec)
├── vanish.c                        # Fonctions pour tester l'environnement de virtualisation
└── Makefile                        # Fichier de compilation
```

## 4. ⚙️ Boot
```bash
boot
├── 1__setup.sh                     # Setup vms, taps, bridge, iptables rules...
├── 2__launch.sh                    # Launch des vms automatiquement
└── 3__clean.sh                     # Cleaning de l'environnement
```

## 5. 📄 Docs
```bash
docs
├── css/
│   └── doxygen-awesome.css         # Thème "Doxygen Awesome" (design moderne et lisible)
├── ext/                            # Extensions JavaScript pour Doxygen
├── img/                            # Images de la doc
├── layout/                         # Templates HTML personnalisés le header et le footer
├── pages/                          # Pages de documentation au format Markdown
│   └── details/                    # Sous-sections détaillées de chaque composant du projet
├── subject/                        # Documents du sujet du projet
└── Doxyfile                        # Fichier de configuration de Doxygen 
                                    # (génère la doc dans un dossier html/)
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

<div class="section_buttons">

| Previous                          | Next                               |
|:----------------------------------|-----------------------------------:|
| [Mise en place](02_install.md)    | [Utilisation](04_usage.md)         |
</div>