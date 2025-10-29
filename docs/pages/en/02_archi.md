# Architecture

\tableofcontents

## 1. 🌳 Directory Tree

```bash
.
├── attacker
├── boot
├── docs
├── rootkit
├── AUTHORS
├── Makefile
├── README
└── TODO
```

Above is the global architecture of the Git repository. It contains both victim-side and attacker-side code, as well as scripts for virtual machine installation, and finally the present documentation. The different parts are detailed in the following sections.
- `attacker` contains files related to the attacker server.
- `boot` contains scripts related to project setup.
- `docs` groups configuration files used to generate the present documentation.
- `rootkit` contains the rootkit source code.
- `AUTHORS` lists the project authors.
- `Makefile` contains all lab installation and manipulation commands.
- `README` simply explains how to generate the documentation.
- `TODO` is a utility file for project tracking and development.

## 2. 💀 Attacker
```bash
attacker/
├── routes/                         # Contains different routes (endpoints) of the Flask application
│   ├── api.py                      # Communication API between server and infected clients
│   ├── dashboard.py                # Routes to display the dashboard (system information, etc.)
│   ├── download.py                 # Manages file downloads from clients
│   ├── explorer.py                 # Allows exploration of remote files (file browser)
│   ├── keylogger.py                # Displays keylogger data retrieved from clients
│   ├── terminal.py                 # Interface to execute remote shell commands
│   └── upload.py                   # Manages file uploads to infected machines
├── static/                         # Static files (CSS, JS, images, sounds, etc.)
│   ├── downloads/                  # Folder where files retrieved from clients are stored
│   ├── fonts/                      # Fonts used in the graphical interface
│   ├── img/                        # Static images
│   ├── js/                         # JavaScript scripts for front-end (dashboard, terminal, etc.)
│   ├── sounds/                     # Sound effects for the user interface (hehe)
│   ├── uploads/                    # Files to send to clients
│   └── style.css                   # Main stylesheet of the web application
├── templates/                      # HTML templates for the web interface
│   ├── partials/                   # Reusable HTML fragments (sidebar, header, info cards, etc.)
│   ├── base.html                   # Base template (common structure for all pages)
│   ├── dashboard.html              # Main dashboard view
│   ├── download.html               # View to manage downloads
│   ├── explorer.html               # File explorer view
│   ├── keylogger.html              # View to display captured keystrokes
│   ├── terminal.html               # Remote terminal web interface
│   └── upload.html                 # View to send files to clients
├── uploads/                        # Contains files to transfer to the victim
├── utils/                          # Utility functions for communication and encryption
│   ├── Crypto/
│   │   └── CryptoHandler.py        # Manages encryption/decryption (AES)
│   ├── DNS/
│   │   └── DNSSender.py            # DNS communication
│   ├── TCP/
│   │   ├── AESNetworkHandler.py    # Manages AES-encrypted TCP network connections
│   │   └── TCPServer.py            # TCP server to receive client connections
│   ├── BigMama.py                  # Main communications manager (TCP and DNS)
│   └── socat.py                    # socat tool integration
├── app.py                          # Flask application initialization (app object creation)
├── config.py                       # Application configuration
├── main.py                         # Main program entry point
├── requirements.txt                # Python dependencies required for the project
└── server.pem                      # Certificate for socat

```

## 3. 🛠️ Rootkit
```bash
rootkit
├── include/                        # Global header files
│   ├── cmd.h                       # Declarations related to rootkit internal commands
│   ├── config.h                    # Configuration macros definition
│   ├── download.h                  # Functions to manage file downloads
│   ├── epirootkit.h                # Main rootkit header
│   ├── socat.h                     # `socat` integration
│   ├── upload.h                    # Functions to manage file uploads
│   └── vanish.h                    # Functions to detect virtual environment
├── interceptor/                    # Part responsible for hooks and kernel bypass
│   ├── core/                       # Internal mechanisms like ftrace...
│   │   ├── include/                # Headers specific to the module core
│   │   │   ├── ftrace.h            # Declarations for using `ftrace`
│   │   │   ├── init.h              # Declarations for interceptor initialization
│   │   │   └── menu.h              # Declarations for hooks menu
│   │   ├── array.c                 # Dynamic array management for hooks
│   │   ├── ftrace.c                # ftrace mechanism implementation
│   │   ├── init.c                  # Module init/exit with default files
│   │   └── menu.c                  # Menu to add/remove files to process
│   ├── hooks/
│   │   ├── alterate/               # Alteration module
│   │   │   ├── alterate_api.c/h
│   │   │   ├── alterate.c/h
│   │   ├── forbid/                 # Forbid module
│   │   │   ├── forbid_api.c/h
│   │   │   ├── forbid.c/h
│   │   └── hide/                   # Hiding module
│   │       ├── hide_api.c/h
│   │       ├── hide.c/h
│   └── misc/
│       └── ghost.c                 # Hide the .ko module presence
├── network/                        # Network communication modules
│   ├── core/
│   │   ├── network.c/h             # General network functionalities
│   └── protocols/                  # Protocols supported by the rootkit
│       ├── dns/                    # DNS communication (tunneling, exfiltration)
│       │   ├── dns.c               # Core functions
│       │   └── worker.c            # Thread to handle DNS requests
│       └── tcp/                    # TCP communication
│           ├── socket.c            # TCP sockets creation and management
│           └── worker.c            # Thread to handle TCP requests
├── passwd/                         # Module to interact with rootkit credentials
│   ├── passwd.c
│   └── passwd.h
├── scripts/
│   ├── romance/
│   │   └── initrd.sh               # Script to inject the rootkit into an initrd
│   ├── format.sh                   # File cleanup / formatting
│   └── generate.sh                 # socat.h generation
├── utils/                          # Shared utility functions
│   ├── crypto/                     # Encryption and hash
│   │   ├── aes.c                   # AES implementation
│   │   ├── crypto.h
│   │   └── hash.c                  # Hash functions
│   ├── io/                         # Low-level input/output (read and write, basically)
│   │   ├── io.c
│   │   └── io.h
│   ├── sysinfo/                    # System information retrieval for the attacker UI
│   │   ├── sysinfo.c
│   │   └── sysinfo.h
│   └── ulist/                      # Global linked lists for hooks with persistence
│       ├── ulist.c
│       └── ulist.h
├── cmd.c                           # All commands available on the rootkit side
├── download.c                      # File downloads from the attacker
├── epikeylog.c                     # Keylogger (keyboard capture)
├── main.c                          # Main entry point of the rootkit executable
├── socat                           # `socat` binary
├── socat.c                         # Source code associated with socat usage
├── upload.c                        # File uploads to the remote server
├── userland.c                      # Code executed in userspace (mostly for exec)
├── vanish.c                        # Functions to test virtualization environment
└── Makefile                        # Compilation file
```

## 4. ⚙️ Boot
```bash
boot
├── 1__setup.sh                     # Setup vms, taps, bridge, iptables rules...
├── 2__launch.sh                    # Automatic vm launch
└── 3__clean.sh                     # Environment cleanup
```

## 5. 📄 Docs
```bash
docs
├── css/
│   └── doxygen-awesome.css         # "Doxygen Awesome" theme (modern and readable design)
├── ext/                            # JavaScript extensions for Doxygen
├── img/                            # Documentation images
├── layout/                         # Custom HTML templates for header and footer
├── pages/                          # Documentation pages in Markdown format
│   └── details/                    # Detailed subsections of each project component
├── subject/                        # Project subject documents
└── Doxyfile                        # Doxygen configuration file 
                                    # (generates docs in an html/ folder)
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
| [Overview](01_main.md)    | [Setup](03_install.md)         |
</div>