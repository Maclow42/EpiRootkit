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
attacker
├── downloads
├── routes
│   ├── auth.py
│   ├── dashboard.py
│   ├── download.py
│   ├── history.py
│   ├── keylogger.py
│   ├── shell.py
│   ├── terminal.py
│   ├── upload.py
│   └── webcam.py
├── static
│   └── style.css
├── templates
│   ├── base.html
│   ├── dashboard.html
│   ├── download.html
│   ├── history.html
│   ├── keylogger.html
│   ├── login.html
│   ├── shell_remote.html
│   ├── terminal.html
│   ├── upload.html
│   └── webcam.html
├── uploads
├── utils
│   ├── aes.py
│   ├── cli.py
│   ├── dns.py
│   ├── socat.py
│   └── socket.py
├── app.py
├── config.py
├── main.py
├── requirements.txt
├── server.pem
└── server.py
```

## 3. 🛠️ Rootkit
```bash
rootkit
├── include
│   ├── config.h
│   ├── epirootkit.h
│   ├── rootkit_command.h
│   ├── socat.h
│   └── vanish.h
├── interceptor
│   ├── core
│   │   ├── include
│   │   ├── array.c
│   │   ├── ftrace.c
│   │   ├── init.c
│   │   └── menu.c
│   ├── hooks
│   │   ├── alterate
│   │   ├── forbid
│   │   └── hide
│   └── misc
│       └── ghost.c
├── network
│   ├── core
│   │   ├── network.c
│   │   └── network.h
│   └── protocols
│       ├── dns
│       └── tcp
├── passwd
│   ├── passwd.c
│   └── passwd.h
├── persist
│   ├── persist.c
│   └── persist.h
├── scripts
│   ├── format.sh
│   └── generate.sh
├── utils
│   ├── crypto
│   │   ├── aes.c
│   │   ├── crypto.h
│   │   └── hash.c
│   ├── io
│   │   ├── io.c
│   │   └── io.h
│   └── ulist
│       ├── ulist.c
│       └── ulist.h
├── epikeylog.c
├── exec_cmd.c
├── file_ops.c
├── main.c
├── rootkit_command.c
├── socat
├── socat.c
├── vanish.c
└── Makefile
```

## 4. ⚙️ Boot
```bash
boot
├── 1__setup.sh
├── 2__launch.sh
└── 3__clean.sh
```

## 5. 📄 Docs
```bash
.
├── css
│   └── doxygen-awesome.css
├── ext
│   ├── doxygen-awesome-darkmode-toggle.js
│   └── doxygen-awesome-interactive-toc.js
├── img
│   ├── diag.svg
│   ├── logo_no_text.png
│   ├── logo.png
│   └── logo_white.png
├── layout
│   ├── footer.html
│   └── header.html
├── pages
│   ├── details
│   │   ├── 01_details.md
│   │   ├── 02_network.md
│   │   └── 03_hooks.md
│   ├── 01_main.md
│   ├── 02_install.md
│   ├── 03_archi.md
│   ├── 04_usage.md
│   └── 05_env.md
├── subject
│   ├── epirootkit-slides.pdf
│   └── epirootkit-subject.pdf
└── Doxyfile
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