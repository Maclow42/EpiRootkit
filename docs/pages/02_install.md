# Mise en place

\tableofcontents

## 1. 📋 Prérequis

- Téléchargement du dépôt Git
-
-

## 2. 📁 Dossier
```bash
cd boot/
```

Après avoir téléchargé le dépôt Git, déplacez-vous dans le dossier **boot/**. Ce dossier contient trois scripts : **1__setup.sh**, **2__launch.sh** et **3__clean.sh**. Ils facilitent la mise en place des machines virtuelles et de l’environnement nécessaire à leur bon fonctionnement.

## 3. ⚙️ Installation
```bash
sudo ./1__setup.sh
```
Exécutez **1__setup.sh** avec sudo. Ce premier script crée le dossier `boot/vms/` et télécharge depuis un serveur distant les deux machines virtuelles. Prévoyez environ quarante minutes pour le téléchargement et l'installation. De plus, deux interfaces TAP (`tap0` et `tap1`) sont créées et reliées à un bridge (`br0`) afin de faciliter la communication entre les VMs. Les interfaces TAP jouent le rôle de cartes réseau virtuelles. Par ailleurs, une série de commandes `iptables` autorise le trafic TCP entrant sur le port 4242 via l'interface `br0`, ainsi que le trafic UDP entrant et sortant sur le port 53, utilisé principalement pour le DNS.

## 4. 🚀 Lancement
```bash
./2__launch.sh
```
Exécutez **2__launch.sh**. Ce script vérifie que tout est correctement installé, puis lance les deux machines virtuelles avec QEMU. Chacune dispose de 4096 Mo de mémoire RAM. L'attaquant est relié à `tap0` et la victime à `tap1`.

## 5. 🔌 Connexion
```bash 
attacker@attacker$ cd /home/attacker/Documents/server/
```
Vous trouverez ci-dessous des informations relatives aux deux machines virtuelles, notamment les identifiants de connexion. Sur la VM victime, le rootkit est préinstallé et se lance automatiquement au démarrage. Sur la VM attaquante, rendez-vous dans le répertoire ci dessus et exécutez `sudo python main.py`. Ensuite, ouvrez Firefox et entrez l’adresse `http://192.168.249.59:5000`.

<div class="full_width_table">
|                  | Victim             | Attacker           |
|------------------|:--------------------|:--------------------|
| Username         | `victim`           | `attacker`         |
| Password         | `victim`           | `attacker`         |
| IP Address       | 192.168.100.3      | 192.168.100.1      |
| MAC Address      | 52:54:00:DD:EE:FF  | 52:54:00:AA:BB:CC  |
| TAP              | `tap1`             | `tap0`             |
</div>

## 6. 🛠️ Utilisation
Pour l’utilisation, veuillez vous référer à la section [Utilisation](04_usage.md).

## 7. 🧹 Nettoyage
```bash 
sudo ./3__clean.sh
```
Afin de nettoyer l’environnement après utilisation, veuillez exécuter le script **3__clean.sh** avec sudo. Ce script vous proposera de supprimer le dossier `vms/` et supprimera également les interfaces TAP ainsi que le pont réseau (bridge).

<img 
  src="../img/logo_no_text.png" 
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
| [Overview](01_main.md)            | [Architecture](03_archi.md)        |
</div>