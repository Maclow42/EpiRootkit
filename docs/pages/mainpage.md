# Overview

\htmlonly
<img 
  src="../img/logo_white.png" 
  style="
    display: block;
    border-radius: 8px; 
    width: 60%;
    overflow: hidden;
  "
/>
\endhtmlonly
![C](https://img.shields.io/badge/c-%2300599C.svg?logo=c&logoColor=white) ![Python](https://img.shields.io/badge/python-3670A0?logo=python&logoColor=ffdd54) ![Flask](https://img.shields.io/badge/flask-%23000.svg?logo=flask&logoColor=white) ![Doxygen](https://img.shields.io/badge/doxygen-2C4AA8?logo=doxygen&logoColor=white) ![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)

## 1. Introduction

Bienvenue dans le projet **EpiRootkit**, un rootkit pédagogique développé dans le cadre de notre cursus à EPITA. Ce rootkit s’insère au niveau noyau pour offrir un canal de commande et de contrôle (C2) hybride, combinant une communication classique par TCP ainsi qu'une communication furtive par requêtes DNS. Les membres du groupes sont **Thibault Colcomb**, **Oleg Krajic** et **Evann Marrel**.

## 2. Architecture

```
.
├── crypto
│   ├── aes.c
│   ├── crypto.h
│   └── hash.c
├── include
│   ├── config.h
│   ├── epirootkit.h
│   ├── rootkit_command.h
│   └── socat.h
├── interceptor
│   ├── core
│   │   ├── include
│   │   │   ├── ftrace.h
│   │   │   ├── init.h
│   │   │   └── menu.h
│   │   ├── array.c
│   │   ├── ftrace.c
│   │   ├── init.c
│   │   └── menu.c
│   ├── hooks
│   │   ├── alterate
│   │   │   ├── alterate.c
│   │   │   ├── alterate.h
│   │   │   └── list.c
│   │   ├── forbid
│   │   │   ├── forbid.c
│   │   │   ├── forbid.h
│   │   │   └── list.c
│   │   └── hide
│   │       ├── hide.c
│   │       ├── hide.h
│   │       └── list.c
│   └── misc
│       └── ghost.c
├── network
│   ├── core
│   │   ├── network.c
│   │   └── network.h
│   └── protocols
│       ├── dns
│       │   ├── dns.c
│       │   └── worker.c
│       └── tcp
│           ├── socket.c
│           └── worker.c
├── scripts
│   ├── format.sh
│   └── generate_socat_h.sh
├── epikeylog.c
├── exec_cmd.c
├── file_ops.c
├── main.c
├── Makefile
├── rootkit_command.c
├── socat
└── socat.c
```