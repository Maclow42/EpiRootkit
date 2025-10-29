# Environment

\tableofcontents

## 1. 🖥️ Virtual Machines {#virtual-machines}

The configuration, both educational and designed to facilitate testing, is somewhat special: the two virtual machines ("rootkit" and "attacking server") run on the same host and must communicate with each other, with the host itself, and with the Internet. For this, we have set up a virtual network setup, summarized in the diagram below.

<img 
  src="diag.svg" 
  style="
    display: block;
    margin: 50px auto;
    overflow: hidden;
  "
/>

Each virtual machine has a unique MAC address. They are each connected to a TAP interface, which acts as a virtual network card. A TAP interface is a layer-2 software device provided by the Linux kernel's tun/tap driver. It allows injecting and retrieving, from userspace, Ethernet frames as if it were a real network device. Additionally, both TAPs are connected to a network bridge hosted by the host. It acts as a virtual Ethernet switch. It receives frames on its interfaces, learns source MAC addresses, and relays them only on interfaces where the destination is located, exactly like a hardware switch. You can connect both TAPs and the host's physical interface to provide Internet access or access to other networks.

> Thanks to this setup, the two virtual machines communicate with each other by directly exchanging Ethernet frames via the bridge. They can reach the host machine and access the Internet via the physical interface or any other gateway configured on the bridge.

## 2. 🧠 Operating Systems

This project aims to develop an educational rootkit capable of communicating with an attack server via two channels (TCP and DNS), injecting and hiding code within a Linux kernel, and then demonstrating its features on a virtual victim machine. For this, we chose two distinct environments.
- **Victim**: `Ubuntu Server 22.04 LTS (kernel 6.8.0-58-generic)`
- **Attacker**: `Manjaro Linux (kernel 6.12.28-1-MANJARO)`

### Victim

Regarding the victim's operating system, the `6.8.0-58-generic` kernel, widely deployed on modern server environments, supports all standard module APIs (`ftrace`, syscall hooking, network API, etc.) used by our rootkit. However, we had to implement some adaptations, particularly for the syscall hooking technique via `ftrace`. Indeed, in recent Linux versions, since the `kallsyms_lookup_name` function is no longer exported, it is necessary to use a `kprobe` to retrieve its address. A kprobe is a Linux kernel mechanism allowing dynamic insertion of an observation point (probe) in the kernel code without recompiling or restarting the machine.

Moreover, a *Server* installation without a graphical interface limits noise (non-essential services) and facilitates observation of rootkit effects (logs, system calls, journald, etc.). APT provides simple tools to install *build-essential*, *linux-headers*, and other dependencies needed for module development. Finally, using an `Ubuntu` virtual machine simplifies the preparation of the experimentation environment (volume creation, snapshots, debugging, etc.). The ISO is available for download at: https://releases.ubuntu.com/jammy/

### Attacker

Manjaro follows a "rolling release" model with a more recent kernel (here 6.12.28-1) than LTS distributions. Recent versions of Python libraries (flask, dnslib, etc.) are directly available or via the AUR, simplifying the development of the attacker's Web interface. The choice of operating system is actually quite arbitrary, since its only function is to host, in a web browser, the attacker's Python server. Any other recent system with the required Python libraries and a browser such as Firefox would work just as well. The ISO is available for download at: https://manjaro.org/products/download/x86.

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
| [Usage](04_usage.md)              | [Network](d5/dc4/network.html)     |
</div>
