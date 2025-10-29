# Setup

\tableofcontents

## 1. 📋 Prerequisites

- Download the Git repository (otherwise, we'll quickly run into issues...)
- Computer running **Ubuntu 24.10** (tested and recommended) with QEMU/KVM and virtualization enabled
- **Two QEMU VMs prepared in advance** (see section 2.1)
- Choupa Chups lollipops
- A bit of good mood, it always does good!

### Virtualization

Here's a quick guide to install QEMU/KVM on Ubuntu 24.10 and enable virtualization. First, enable virtualization in your BIOS. Then update the package list.
```bash
sudo apt update
```

Then install QEMU, KVM, and Libvirt (optional: `virt-manager` for a GUI) as shown below. 
```bash
sudo apt install -y qemu-kvm libvirt-daemon-system libvirt-clients
sudo apt install -y bridge-utils build-essential linux-headers-$(uname -r)
```

Add your user to the groups, then log out/log in for the change to take effect. Then enable and start the libvirt service.
```bash
sudo usermod -aG libvirt,kvm $USER
sudo systemctl enable --now libvirtd
```

## 2. ⚙️ Setup

Start by cloning the project's Git repository, available at the following address: [epita-apprentissage-wlkom-apping-2027-STDBOOL.git](epita-apprentissage-wlkom-apping-2027-STDBOOL.git). Once the repository is cloned, you'll find the following structure at the root:
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
| Element      | Description                                                                                   |
|:--------------|:----------------------------------------------------------------------------------------------|
| **AUTHORS**  | List of project authors                                                                       |
| **README**   | Basic project explanation file                                                                |
| **TODO**     | Project TODO file, contains all completed or planned tasks                                    |
| **boot**     | Folder containing virtual machine setup scripts                                               |
| **attacker** | Folder containing all web service used by the attacker                                        |
| **rootkit**  | Folder containing all rootkit code                                                            |
| **docs**     | Folder containing this documentation in markdown and HTML format                              |
| **Makefile** | Lab installation and usage Makefile                                                           |
</div>

All operations are centralized in the Makefile. Here are the main available commands (to use with make):

<div class="full_width_table">
| Command                | Description                                                                                                         |
|:-------------------------|:---------------------------------------------------------------------------------------------------------------------|
| **prepare**             | Creates all necessary network interfaces and iptables rules                                                          |
| **start**               | Starts the two project virtual machines (attacker and victim)                                                        |
| **update_attacker**     | Uploads the `attacker` folder to the attack machine                                                                  |
| **launch_attacker**     | Starts the attack web service from the attack machine                                                                |
| **update_victim**       | Uploads the `rootkit` folder to the victim machine                                                                   |
| **launch_victim**       | Compiles the rootkit code on the victim machine and inserts the rootkit with `insmod`                               |
| **launch_debug_victim** | Same operation as previous, but rootkit is compiled with the DEBUG flag                                              |
| **stop_epirootkit**     | Attempts to 'rmmod' the rootkit (only if rootkit compiled with DEBUG flag)                                          |
| **doc**                 | Generates HTML documentation in the `docs/html` folder                                                              |
| **clean**               | Cleans all network configurations made by `prepare`                                                                 |
</div>

### 2.1 VM Preparation

#### Creating Virtual Machines

**Important**: You must prepare two QEMU virtual machines in advance. The project has been tested with **Ubuntu 24.10**.

**Minimum VM specifications:**
- **OS**: A Linux 6 kernel based distribution (tested on Ubuntu 24.10)
- **Disk format**: QCOW2
- **RAM**: 2GB minimum (4GB recommended)
- **Disk size**: 10GB minimum

**Required VM files:**
You need to create two QEMU disk images and place them in the `boot/vms/` directory:
- `attacker_disk.qcow2` - Attacker VM disk
- `victim_disk.qcow2` - Victim VM disk

**Network configuration inside VMs:**
Both VMs must be configured with static IP addresses:
- **Attacker VM**: 
  - IP: 192.168.100.2/24
  - Gateway: 192.168.100.1
  - MAC: 52:54:00:AA:BB:CC
- **Victim VM**:
  - IP: 192.168.100.3/24
  - Gateway: 192.168.100.1
  - MAC: 52:54:00:DD:EE:FF

To configure static IPs on Ubuntu 24.10, edit `/etc/netplan/01-netcfg.yaml`:

**Attacker VM** (`/etc/netplan/01-netcfg.yaml`):
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

**Victim VM** (`/etc/netplan/01-netcfg.yaml`):
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

Apply the configuration with:
```bash
sudo netplan apply
```

#### Network Setup

Once your VMs are ready and placed in `boot/vms/`, set up the network configuration by running:

```bash
make prepare
```

Since this script modifies your network configuration, it will ask for your root password. The script will:
- Verify that VM disk images exist in `boot/vms/`
- Create a Linux bridge (br0) with IP 192.168.100.1/24
- Create TAP interfaces (tap0, tap1) for both VMs
- Configure iptables rules for network forwarding

Once this is done, you can perform the first machine boot by running:

```bash
make start
```
Starting both virtual machines may take some time, but you don't have to do anything other than wait and whistle the *Star Wars* theme to the tune of *Jurassic Park*.

### 2.2 Web Server and Rootkit

Once the machines are started, you can send the attack web server code to the attacking machine with the command below.
```bash
make update_attacker
```

Then send the rootkit code to the victim machine.
```bash
make update_victim
```

### 2.3 Starting the Attack

You can finally, in a first terminal, start the attack web server.

```bash
make launch_attacker
```

It will then be accessible at [http://192.168.100.2:5000](http://192.168.100.2:5000), both from the attack machine and from the host. To access it, simply open a browser like firefox (present by default on the attack VM) and enter the mentioned address in the search bar.

> **Note**: As mentioned in the [Reverse Shell](#reverse-shell) section, it is necessary to access the web service directly from the attack machine if you want to benefit from the reverse shell.

For the victim machine, you can use a second terminal to compile and insert the rootkit. Two modes are available:
- DEBUG mode: the rootkit will produce logs visible from the victim machine's journals and will not be invisible by default (it can therefore be removed with `rmmod`). This mode is launched with:

```bash
make launch_debug_victim
``` 

- normal mode: the rootkit produces no logs and is invisible by default:

```bash
make launch_victim
```

> **Note**: If the rootkit is launched in DEBUG mode, you can then deactivate it by running `make stop_epirootkit`, which will remove it from the victim machine with `rmmod`.


## 3. 🔌 Connecting to Machines

Here is all the information related to both virtual machines, including their login credentials.

<div class="full_width_table">
|                  | Victim             | Attacker           |
|------------------|:--------------------|:--------------------|
| Username         | `victim`           | `attacker`         |
| Password         | `victim`           | `attacker`         |
| IP Address       | 192.168.100.3      | 192.168.100.2      |
| MAC Address      | 52:54:00:DD:EE:FF  | 52:54:00:AA:BB:CC  |
| TAP              | `tap1`             | `tap0`             |
</div>

Thus, SSH connection to the victim machine is for example possible by running in a terminal the command below.
```bash
ssh victim@192.168.100.3
```
SSH access can be useful to run [Makefile]{#Makefile} commands directly from the concerned machines. Here are the equivalents below.

### Attack Machine {#equivalents-attack}
<div class="full_width_table">
| Action                        | Command                                  |
|:-------------------------------|:------------------------------------------|
| Start web server              | `sudo python3 ~/attacker/main.py`        |
</div>

### Victim Machine
<div class="full_width_table">
| Action                                 | Command                                         |
|:-----------------------------------------|:--------------------------------------------------|
| Compile rootkit (DEBUG mode)           | `sudo make -f ~/rootkit/Makefile debug`          |
| Insert rootkit                         | `sudo insmod ~/rootkit/epirootkit.ko`            |
| Remove module from kernel              | `sudo rmmod epirootkit`                          |
</div>

## 4. 🧹 Cleanup

To clean the environment after use, please `make clean`. 
This script will offer to delete the `boot/vms/` folder and will also remove TAP interfaces and the network bridge.

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
| [Architecture](02_archi.md)            | [Usage](04_usage.md)        |
</div>