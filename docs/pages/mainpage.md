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

## 1. Introduction

FIXME

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