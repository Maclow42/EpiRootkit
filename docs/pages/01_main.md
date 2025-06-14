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

Bienvenue dans le projet **EpiRootkit**, un rootkit pédagogique développé dans le cadre de notre cursus à EPITA. Ce rootkit s’insère au niveau noyau pour offrir un canal de commande et de contrôle (C2) hybride, combinant une communication classique par TCP ainsi qu'une communication furtive par requêtes DNS. Les membres du groupes sont **Thibault Colcomb**, **Oleg Krajic** et **Evann Marrel**.

## 🚀 Fonctionnalités

- 🌐 Communication par canal TCP et/ou DNS
- 🖥️ Exécution de commandes à distance
- 🐚 Reverse shell
- 🙈 Cacher des répertoires et des fichiers (dynamique)
- 🚫 Interdire l’accès à des répertoires ou fichiers (dynamique)
- ✏️ Modifier l’affichage de certains fichiers ciblés (dynamique)
- 🕵️ Cacher le module dans la liste des modules
- 🔐 Chiffrement AES des communications (TCP et DNS)
- 🔑 Authentification à distance
- 🕸️ Interface Web de contrôle distant
- 🚪 Cacher des ports réseau
- ⌨️ Keylogger
- 🔄 Persistance au reboot
- 🖼️ ASCII art
- 📁 Envoi et réception de fichiers
- 🛡️ Détection d’environnement virtuel
- 🔍 Explorateur distant de fichiers

## 🏢 Organisation

Cette documentation a été générée avec Doxygen. Elle est organisée en plusieurs sections répertoriées ci-dessous. Ce qui n’est pas mentionné ici correspond à la documentation du code source générée automatiquement.
- [**Overview**](01_main.md) - Introduction générale à la documentation.
- [**Architecture**](02_archi.md) - Arborescence du dépôt Git.
- [**Mise en place**](03_install.md) - Configuration de la virtualisation et mise en place du projet.
- [**Utilisation**](04_usage.md) - Commandes disponibles, fonctionnement de l’interface web.
- [**Environnement**](05_env.md) - Dispositif de communication entre machines, et informations sur les OS.
- [**Détails**](dd/dab/details.html) - Informations techniques concernant l’implémentation des fonctionnalités du rootkit.


<div class="section_buttons">

| Previous                          | Next                               |
|:----------------------------------|-----------------------------------:|
|                                   | [Architecture](02_archi.md)      |
</div>