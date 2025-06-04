\page network Réseau
\tableofcontents

## 1. 🌐 Introduction

## 2. 🤝 TCP

## 3. 🧭 DNS

Dans le cadre de ce projet, la communication principale utilisée pour l’échange de paquets entre les deux machines virtuelles repose naturellement sur le protocole TCP. Cependant, nous avons choisi de mettre en œuvre une méthode de communication alternative afin d’introduire un aspect furtif aux échanges. L’objectif est de démontrer, de manière pédagogique, comment envoyer des commandes à une machine cible via des requêtes DNS de type **TXT**, puis d’exfiltrer les résultats de ces commandes en les encapsulant dans des requêtes DNS de type **A**. La partie **C** (côté victime) construit manuellement les paquets DNS, les envoie, puis lit les réponses. Du côté attaquant, un programme Python écoute sur le port **53**, intercepte les requêtes DNS entrantes et y répond en renvoyant les données attendues.

### 3.1 Header

<img 
  src="dns_packet.svg" 
  style="
    display: block;
    margin: 50px auto;
    overflow: hidden;
  "
/>



## 4. 🔒 Chiffrement

## 5.  Améliorations

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>