# Mise en place

\tableofcontents

## 1. 📋 Prérequis

- Téléchargement du dépôt Git (sinon, on risque d'être rapidement embêtés...)
- Ordinateur sous Ubuntu 24.10 avec QEMU/KVM et virtualisation activée
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

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/logscreen-victim.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the victim's login screen.
  </figcaption>
</figure>
\endhtmlonly

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/logscreen-attacker.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the attacker's login screen.
  </figcaption>
</figure>
\endhtmlonly

Si le programme s’exécute sans problème et sans aucune erreur, la sortie affichée dans la console devrait ressembler à la figure ci-dessous.

```bash
 ./2__launch.sh
=================================================
                  Launching VMs                  
=================================================
[DEBUG] Launching Attacker VM...
[DEBUG] Launching Victim VM...
=================================================
                   Information                   
=================================================
  Project Directory       : vms
  Attacker Disk           : attacker_disk.qcow2
  Victim Disk             : victim_disk.qcow2
-------------------------------------------------
  Attacker VM PID         : 243473        
  Attacker MAC Address    : 52:54:00:AA:BB:CC    
  Attacker IP             : 192.168.100.2        
  Attacker TAP Interface  : tap0                 
  Attacker Username       : attacker             
  Attacker Password       : attacker             
-------------------------------------------------
  Victim VM PID           : 243505          
  Victim MAC Address      : 52:54:00:DD:EE:FF    
  Victim IP               : 192.168.100.3        
  Victim TAP Interface    : tap1                 
  Victim Username         : victim               
  Victim Password         : victim               
-------------------------------------------------
  Bridge Name             : br0                  
  Bridge IP               : 192.168.100.1        
=================================================
             Completed Launching VMs             
=================================================
```


## 5. 🔌 Connexion
```bash 
attacker@attacker$ cd /home/attacker/Documents/server/
```
Vous trouverez ci-dessous des informations relatives aux deux machines virtuelles, notamment les identifiants de connexion. Sur la VM victime, le rootkit est préinstallé et se lance automatiquement au démarrage. Sur la VM attaquante, rendez-vous dans le répertoire ci dessus et exécutez `sudo python main.py`. Ensuite, choisissez l'option **2**, ouvrez Firefox et entrez l’adresse indiquée dans la console (*Running on http://x.x.x.x:5000...*) pour accéder à l’interface graphique. Une interaction en CLI est également possible via l’option **1**.

<div class="full_width_table">
|                  | Victim             | Attacker           |
|------------------|:--------------------|:--------------------|
| Username         | `victim`           | `attacker`         |
| Password         | `victim`           | `attacker`         |
| IP Address       | 192.168.100.3      | 192.168.100.2      |
| MAC Address      | 52:54:00:DD:EE:FF  | 52:54:00:AA:BB:CC  |
| TAP              | `tap1`             | `tap0`             |
</div>

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/logscreen-connected-victim.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the victim's login screen after connection.
  </figcaption>
</figure>
\endhtmlonly

\htmlonly
<figure style="text-align: center;">
  <img 
    src="../../../img/logscreen-connected-attacker.png" 
    style="
      margin: 30px 0px 0px;
      border-radius: 8px; 
      width: 100%;
    "
  />
  <figcaption style="margin-top: 0.5em; font-style: italic;">
    Figure: Screenshot of the attacker's login screen after connection.
  </figcaption>
</figure>
\endhtmlonly

## 6. 🛠️ Utilisation
Pour l’utilisation, veuillez vous référer à la section [Utilisation](04_usage.md). Pour ce qui est du mot de passe par défaut utilisé par l’attaquant pour s’authentifier auprès du rootkit sur la machine victime, il suffit de cliquer sur `Authentification` dans l’interface web et d’entrer `evannounet`. Si vous passez par la ligne de commande, entrez simplement la commande indiquée ci-dessous. Vous pourrez changer le mot de passe ultérieurement.
```bash
connect evannounet
```

## 7. 🧹 Nettoyage
```bash 
sudo ./3__clean.sh
```
Afin de nettoyer l’environnement après utilisation, veuillez exécuter le script **3__clean.sh** avec sudo. Ce script vous proposera de supprimer le dossier `vms/` et supprimera également les interfaces TAP ainsi que le pont réseau (bridge).

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
| [Overview](01_main.md)            | [Architecture](03_archi.md)        |
</div>