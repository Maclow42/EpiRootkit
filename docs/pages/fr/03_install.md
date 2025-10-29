# Mise en place

\tableofcontents

## 1. 📋 Prérequis

- Téléchargement du dépôt Git (sinon, on risque d'être rapidement embêtés...)
- Ordinateur sous **Ubuntu 24.10** (testé et recommandé) avec QEMU/KVM et virtualisation activée
- **Deux VMs QEMU préparées à l'avance** (voir section 2.1)
- Des sucettes Choupa Choups
- Un peu de bonne humeur, ça fait toujours du bien !

### Virtualisation

Voici un petit guide pour installer QEMU/KVM sur Ubuntu 24.10 et activer la virtualisation. Dans un premier temps, autorisez la virtualisation dans votre BIOS. Ensuite mettez à jour la liste des paquets.
```bash
sudo apt update
```

Puis installez QEMU, KVM et Libvirt (optionnel : `virt-manager` pour une GUI) comme montré ci-dessous. 
```bash
sudo apt install -y qemu-kvm libvirt-daemon-system libvirt-clients
sudo apt install -y bridge-utils build-essential linux-headers-$(uname -r)
```

Ajoutez votre utilisateur aux groupes, puis déconnectez-vous/reconnectez-vous pour que la modification prenne effet. Activez ensuite et démarrer le service libvirt.
```bash
sudo usermod -aG libvirt,kvm $USER
sudo systemctl enable --now libvirtd
```

## 2. ⚙️ Mise en place

Commencez par cloner le dépôt Git du projet, disponible à l'adresse suivante : [epita-apprentissage-wlkom-apping-2027-STDBOOL.git](epita-apprentissage-wlkom-apping-2027-STDBOOL.git). Une fois le dépôt cloné, vous trouverez l'arborescence suivante à la racine :
```
epita-apprentissage-wlkom-apping-2027-STDBOOL
├── AUTHORS
├── README
├── TODO
├── boot
├── attacker
├── rootkit
├── docs
└── Makefile
```

<div class="full_width_table">
| Élément      | Description                                                                                   |
|:--------------|:----------------------------------------------------------------------------------------------|
| **AUTHORS**  | Liste des auteurs du projet                                                                  |
| **README**   | Fichier d'explications basiques du projet                                                    |
| **TODO**     | Fichier TODO du projet, contient l'ensemble des tâches effectuées ou prévues                 |
| **boot**     | Dossier contenant les scripts de mise en place des machines virtuelles                       |
| **attacker** | Dossier contenant tout le service web utilisé par l'attaquant                                |
| **rootkit**  | Dossier contenant tout le code du rootkit                                                    |
| **docs**     | Dossier contenant cette documentation au format markdown et HTML                             |
| **Makefile** | Makefile d'installation et d'utilisation du lab                                              |
</div>

Toutes les opérations sont centralisées dans le Makefile. Voici les principales commandes disponibles (à utiliser avec make) :

<div class="full_width_table">
| Commande                | Description                                                                                                         |
|:-------------------------|:---------------------------------------------------------------------------------------------------------------------|
| **prepare**             | Crée toutes les interfaces réseau et règles iptables nécessaires                                                    |
| **start**               | Démarre les deux machines virtuelles du projet (attaquante et victime)                     |
| **update_attacker**     | Téléverse le dossier `attacker` vers la machine d'attaque                                                          |
| **launch_attacker**     | Démarre le service web d'attaque depuis la machine d'attaque                                                       |
| **update_victim**       | Téléverse le dossier `rootkit` vers la machine victime                                                             |
| **launch_victim**       | Compile le code du rootkit sur la machine victime et insère le rootkit avec `insmod`                               |
| **launch_debug_victim** | Même opération que précédemment, mais le rootkit est compilé avec le flag DEBUG                                    |
| **stop_epirootkit**     | Tente de 'rmmod' le rootkit (uniquement si rootkit compilé avec le flag DEBUG)               |
| **doc**                 | Génère la documentation HTML dans le dossier `docs/html`                                                           |
| **clean**               | Nettoie l'ensemble des configurations réseau effectuées par `prepare`                                              |
</div>

### 2.1 Préparation des VMs

#### Création des machines virtuelles

**Important** : Vous devez préparer deux machines virtuelles QEMU à l'avance. Le projet a été testé avec **Ubuntu 24.10**.

**Spécifications minimales des VMs :**
- **OS** : Distribution Linux basée sur le noyau 6 (testé sur Ubuntu 24.10)
- **Format de disque** : QCOW2
- **RAM** : 2GB minimum (4GB recommandé)
- **Taille de disque** : 10GB minimum

**Fichiers requis :**
Vous devez créer deux images disque QEMU et les placer dans le répertoire `boot/vms/` :
- `attacker_disk.qcow2` - Disque de la VM attaquante
- `victim_disk.qcow2` - Disque de la VM victime

**Configuration réseau dans les VMs :**
Les deux VMs doivent être configurées avec des adresses IP statiques :
- **VM Attaquante** :
  - IP : 192.168.100.2/24
  - Passerelle : 192.168.100.1
  - MAC : 52:54:00:AA:BB:CC
- **VM Victime** :
  - IP : 192.168.100.3/24
  - Passerelle : 192.168.100.1
  - MAC : 52:54:00:DD:EE:FF

Pour configurer les IPs statiques sur Ubuntu 24.10, éditez `/etc/netplan/01-netcfg.yaml` :

**VM Attaquante** (`/etc/netplan/01-netcfg.yaml`) :
```yaml
network:
  version: 2
  renderer: networkd
  ethernets:
    ens3:
      addresses:
        - 192.168.100.2/24
      routes:
        - to: default
          via: 192.168.100.1
      nameservers:
        addresses: [8.8.8.8, 8.8.4.4]
```

**VM Victime** (`/etc/netplan/01-netcfg.yaml`) :
```yaml
network:
  version: 2
  renderer: networkd
  ethernets:
    ens3:
      addresses:
        - 192.168.100.3/24
      routes:
        - to: default
          via: 192.168.100.1
      nameservers:
        addresses: [8.8.8.8, 8.8.4.4]
```

Appliquez la configuration avec :
```bash
sudo netplan apply
```

#### Configuration réseau

Une fois vos VMs prêtes et placées dans `boot/vms/`, mettez en place la configuration réseau en lançant :

```bash
make prepare
```

Étant donné que ce script modifie votre configuration réseau, il vous demandera votre mot de passe root. Le script va :
- Vérifier que les images disque des VMs existent dans `boot/vms/`
- Créer un pont Linux (br0) avec l'IP 192.168.100.1/24
- Créer des interfaces TAP (tap0, tap1) pour les deux VMs
- Configurer les règles iptables pour le forwarding réseau

Une fois ceci effectué, vous pouvez effectuer le premier démarrage des machines en lançant :

```bash
make start
```
Le démarrage des deux machines virtuelles peut prendre un peu de temps, mais vous n'avez rien d'autre à faire que d'attendre et de siffloter le thème de *Star Wars* sur l'air de *Jurassic Park*.

### 2.2 Serveur web et rootkit

Une fois les machines démarrées, vous pouvez envoyez le code du serveur web d’attaque à la machine attaquante avec la commande ci-dessous.
```bash
make update_attacker
```

Puis envoyez ensuite le code du rootkit à la machine victime.
```bash
make update_victim
```

### 2.3 Démarrage de l'attaque

Vous pouvez finalement, dans un premier terminal, démarrer le serveur web d'attaque.

```bash
make launch_attacker
```

Celui-ci sera ensuite accessible à l'adresse [http://192.168.100.2:5000](http://192.168.100.2:5000), aussi bien depuis la machine d'attaque que depuis l'hôte. Pour y acceder il suffit donc d'ouvrir un navigateur comme firefox (present par defaut sur la VM d'attaque) et d'indiquer l'adresse mentionnee dans la barre de recherche.

> **Note** : Comme mentionné dans la section [Reverse Shell](#reverse-shell), il est nécessaire d'accéder au service web directement depuis la machine d'attaque si vous souhaitez bénéficier du reverse shell.

Pour ce qui est de la machine victime, vous pouvez utiliser un second terminal afin de compiler et d'insérer le rootkit. Deux modes sont disponibles :
- mode DEBUG : le rootkit produira des logs visibles depuis les journaux de la machine victime et ne sera par défaut pas invisible (il pourra donc être retiré avec `rmmod`). Ce mode se lance avec :

```bash
make launch_debug_victim
``` 

- mode normal : le rootkit ne produit aucun log et est par défaut invisible :

```bash
make launch_victim
```

> **Note** : Si le rootkit est lancé en mode DEBUG, vous pouvez alors le désactiver en lançant `make stop_epirootkit`, ce qui le retirera de la machine victime avec `rmmod`.


## 3. 🔌 Connexion aux machines

Voici l'ensemble des informations relatives aux deux machines virtuelles, notamment leurs identifiants de connexion.

<div class="full_width_table">
|                  | Victim             | Attacker           |
|------------------|:--------------------|:--------------------|
| Username         | `victim`           | `attacker`         |
| Password         | `victim`           | `attacker`         |
| IP Address       | 192.168.100.3      | 192.168.100.2      |
| MAC Address      | 52:54:00:DD:EE:FF  | 52:54:00:AA:BB:CC  |
| TAP              | `tap1`             | `tap0`             |
</div>

Ainsi, la connexion à la machine victime en SSH est par exemple possible en lançant dans un terminal la commande ci-dessous.
```bash
ssh victim@192.168.100.3
```
Un accès SSH peut être utile afin de lancer les commandes du [Makefile]{#Makefile} directement depuis les machines concernées. Voici ci-dessous les équivalents.

### Machine d'attaque {#equivalents-attaque}
<div class="full_width_table">
| Action                        | Commande                                 |
|:-------------------------------|:------------------------------------------|
| Démarrer le serveur web       | `sudo python3 ~/attacker/main.py`        |
</div>

### Machine victime
<div class="full_width_table">
| Action                                 | Commande                                         |
|:-----------------------------------------|:--------------------------------------------------|
| Compiler le rootkit (mode DEBUG)        | `sudo make -f ~/rootkit/Makefile debug`          |
| Insérer le rootkit                      | `sudo insmod ~/rootkit/epirootkit.ko`            |
| Supprimer le module du kernel           | `sudo rmmod epirootkit`                          |
</div>

## 4. 🧹 Nettoyage

Afin de nettoyer l’environnement après utilisation, veuillez `make clean`. 
Ce script vous proposera de supprimer le dossier `boot/vms/` et supprimera également les interfaces TAP ainsi que le pont réseau (bridge).

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
| [Architecture](02_archi.md)            | [Utilisation](04_usage.md)        |
</div>