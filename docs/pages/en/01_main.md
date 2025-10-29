# Overview

\tableofcontents

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
![C](https://img.shields.io/badge/c-%2300599C.svg?logo=c&logoColor=white) ![Python](https://img.shields.io/badge/python-3670A0?logo=python&logoColor=ffdd54) ![Flask](https://img.shields.io/badge/flask-%23000.svg?logo=flask&logoColor=white) ![Doxygen](https://img.shields.io/badge/doxygen-2C4AA8?logo=doxygen&logoColor=white) ![Manjaro](https://img.shields.io/badge/manjaro-35BF5C?style=flat&logo=manjaro&logoColor=white) ![Ubuntu](https://img.shields.io/badge/Ubuntu-E95420?logo=Ubuntu&logoColor=white) 
## ✨ Introduction

Welcome to the **EpiRootkit** project, an educational rootkit developed as part of our curriculum at EPITA. This rootkit operates at the kernel level to provide a hybrid command and control (C2) channel, combining classic TCP communication with stealthy DNS-based communication. The team members are **Thibault Colcomb**, **Oleg Krajic**, and **Evann Marrel**.

## 🚀 Features

- 🌐 TCP and/or DNS channel communication
- 🖥️ Remote command execution
- 🐚 Reverse shell
- 🙈 Hide directories and files (dynamic)
- 🚫 Block access to directories or files (dynamic)
- ✏️ Modify the display of targeted files (dynamic)
- 🕵️ Hide the module from the modules list
- 🔐 AES encryption for communications (TCP and DNS)
- 🔑 Remote authentication
- 🕸️ Remote control web interface
- 🚪 Hide network ports
- ⌨️ Keylogger
- 🔄 Persistence through reboot
- 🖼️ ASCII art
- 📁 File upload and download
- 🛡️ Virtual environment detection
- 🔍 Remote file explorer

## 🏢 Organization

This documentation was generated with Doxygen. It is organized into several sections listed below. What is not mentioned here corresponds to the automatically generated source code documentation.
- [**Overview**](01_main.md) - General introduction to the documentation.
- [**Architecture**](02_archi.md) - Git repository structure.
- [**Setup**](03_install.md) - Virtualization configuration and project setup.
- [**Usage**](04_usage.md) - Available commands, web interface operation.
- [**Environment**](05_env.md) - Communication setup between machines and OS information.
- [**Details**](dd/dab/details.html) - Technical information about rootkit feature implementation.


<div class="section_buttons">

| Previous                          | Next                               |
|:----------------------------------|-----------------------------------:|
|                                   | [Architecture](02_archi.md)      |
</div>
