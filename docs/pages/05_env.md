# Environnement

\tableofcontents

## 1. 🖥️ Machines virtuelles

La configuration, à la fois pédagogique et destinée à faciliter les tests, est un peu particulière : les deux machines virtuelles (« rooktit » et « serveur attaquant ») tournent sur un même hôte et doivent communiquer entre elles, avec l’hôte lui-même, et avec Internet. Pour cela, nous avons mis en place un dispositif de réseau virtuel, résumé sur le schéma ci-dessous.

<img 
  src="diag.svg" 
  style="
    display: block;
    margin: 50px auto;
    overflow: hidden;
  "
/>

Chaque machine virtuelle dispose d’une adresse MAC unique. Elles sont chacune raccordées à une interface TAP, qui joue le rôle de carte réseau virtuelle. Une interface TAP est un dispositif logiciel de niveau 2 fourni par le pilote tun/tap du noyau Linux. Il permet d’injecter et de récupérer, depuis l’espace utilisateur, des trames Ethernet comme si c’était un vrai périphérique réseau. Par ailleurs, les deux TAP sont reliées à un pont réseau (ou bridge) hébergé par l’hôte. Il fait office de commutateur Ethernet virtuel. Il reçoit les trames sur ses interfaces, apprend les adresses MAC source, et les relaie uniquement sur les interfaces où se trouve la destination, exactement comme un switch matériel. On peut y connecter aussi bien des TAP que l’interface physique de l’hôte pour donner accès à Internet ou à d’autres réseaux.

Grâce à ce montage, les deux machines virtuelles communiquent entre elles en échangeant directement des trames Ethernet via le bridge. Elle peuvent atteindre la machine hôte et accéder à Internet via l’interface physique ou toute autre passerelle configurée sur le bridge.

## 2. 🧠 Systèmes d'exploitation

Ce projet vise à développer un rootkit pédagogique capable de communiquer avec un serveur d’attaque via deux canaux (TCP et DNS), d’injecter et de cacher du code au sein d’un noyau Linux, puis de démontrer ses fonctionnalités sur une machine victime virtuelle. Pour cela, nous avons choisi deux environnements distincts.
- **Victime** : `Ubuntu Server 22.04 LTS (noyau 6.8.0-58-generic)`
- **Attaquant** : `Manjaro Linux (noyau 6.12.28-1-MANJARO)`

### Victime

Concernant le système d’exploitation de la victime, le noyau `6.8.0-58-generic`, largement déployé sur les environnements serveurs modernes, prend en charge toutes les API de modules standard (`ftrace`, hooking des syscalls, API réseau, etc.) utilisées par notre rootkit. Toutefois, nous avons dû mettre en œuvre quelques adaptations, notamment pour la technique de `hook` des appels système via `ftrace`. En effet, dans les versions récentes de Linux, la fonction `kallsyms_lookup_name()` n’étant plus exportée, il est nécessaire de recourir à un `kprobe` pour en récupérer l’adresse. Un kprobe est un mécanisme du noyau Linux permettant d’insérer dynamiquement un point d’observation (sonde) dans le code du noyau sans recompiler ni redémarrer la machine.

Par ailleurs, une installation *Server* sans interface graphique limite le bruit (services non essentiels) et facilite l’observation des effets du rootkit (logs, appels système, journald, etc). APT fournit des outils simples pour installer *build-essential*, *linux-headers* et les autres dépendances nécessaires au développement de modules. Enfin, l’utilisation d’une machine virtuelle `Ubuntu` simplifie la préparation de l’environnement d’expérimentation (création de volumes, snapshots, débogage, etc). L’ISO est disponible au téléchargement à l’adresse suivante : https://releases.ubuntu.com/jammy/

### Attaquant

Manjaro suit un modèle “rolling release” avec un noyau plus récent (ici 6.12.28-1) que les distributions LTS. Les versions récentes des bibliothèques Python (flask, dnslib, etc) sont disponibles directement ou via l’AUR, simplifiant le développement de l’interface Web de l’attaquant. Le choix du système d’exploitation est en réalité assez arbitraire, puisque sa seule fonction est d’héberger, dans un navigateur web, le serveur Python de l’attaquant. Tout autre système récent, doté des bibliothèques Python requises et d’un navigateur tel que Firefox, conviendrait tout aussi bien. L’ISO est disponible au téléchargement à l’adresse suivante : https://manjaro.org/products/download/x86.

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
| [Utilisation](04_usage.md)        | [Réseau](d5/dc4/network.html)      |
</div>