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

## 3. 📦 Packages supplémentaire

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
| [Utilisation](04_usage.md)        |                                    |
</div>