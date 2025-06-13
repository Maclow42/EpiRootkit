\page vm_detect Hypervisor
\tableofcontents

## 1. 🕵️‍♂️ Détection

La détection d’un environnement virtuel par notre rootkit s’appuie sur deux mécanismes complémentaires : l’analyse des fonctionnalités CPU pour identifier la présence d’un hyperviseur, et l’interrogation des informations DMI ou *Desktop Management Interface* pour repérer les signatures matérielles typiques de machines virtuelles. Ces deux approches, réunies dans la fonction `is_running_in_virtual_env`, nous permettent donc une vérification bas-niveau (CPU) et haut-niveau (firmware)...

Par ailleurs, la présence d’un hyperviseur peut être signalée au travers du flag `HYPERVISOR` dans les fonctionnalités CPU, exposé par la CPUID instruction. Notre fonction `check_hypervisor` exploite ce mécanisme. Elle invoque ainsi `boot_cpu_has` avec l’argument `X86_FEATURE_HYPERVISOR`, qui retourne `true` si, lors de l’initialisation du noyau, le flag `HYPERVISOR` a été détecté.
```c
bool check_hypervisor(void) {
    return boot_cpu_has(X86_FEATURE_HYPERVISOR);
}
```

Pour ce qui est du DMI, c'est un standard qui permet au BIOS/UEFI de fournir des informations sur le matériel et le fabricant à l’OS... Ainsi, parmi ces informations se trouve le nom du vendeur (`SYS_VENDOR`) et d’autres identifiants. Nous utilisons donc le tableau `hypervisor_dmi_table` pour vérifier ce nom. Chaque entrée compare la chaîne renvoyée par le BIOS (`DMI_SYS_VENDOR`) avec un nom de fournisseur connu pour agir comme un hyperviseur
```c
bool check_dmi(void) {
    static const struct dmi_system_id hypervisor_dmi_table[] = {
        { .ident = "VMware", .matches = { DMI_MATCH(DMI_SYS_VENDOR, "VMware") } },
        { .ident = "VirtualBox",
          .matches = { DMI_MATCH(DMI_SYS_VENDOR, "innotek GmbH") } },
        { .ident = "QEMU", .matches = { DMI_MATCH(DMI_SYS_VENDOR, "QEMU") } },
        { .ident = "DigitalOcean",
          .matches = { DMI_MATCH(DMI_SYS_VENDOR, "DigitalOcean") } },
        { .ident = "OpenStack",
          .matches = { DMI_MATCH(DMI_SYS_VENDOR, "OpenStack") } },
        { .ident = "Scaleway", .matches = { DMI_MATCH(DMI_SYS_VENDOR, "Scaleway") } },
        {}
    };

    return dmi_check_system(hypervisor_dmi_table) > 0;
}
```

La fonction principale `is_running_in_virtual_env` urilise finalement ces deux contrôles afin de retourner `true` dès que l’un ou l’autre des mécanismes signale un environnement virtualisé.
```c
bool is_running_in_virtual_env(void) {
    if (check_hypervisor()) {
        ERR_MSG("vanish: hypervisor detected...");
        return true;
    }

    if (check_dmi()) {
        ERR_MSG("vanish: virtual environment detected...");
        return true;
    }

    return false;
}
```

C’est certes une approche assez simpliste, mais d’après nos tests, elle permet de détecter l’environnement virtuel dans la plupart des cas. Pour ce qui est de la liste des vendeurs, elle est bien sûr non exhaustive, et dans une utilisation réelle, il faudrait étudier plus en profondeur les noms des autres fournisseurs. De plus, notre rootkit n’utilise pour le moment cette fonctionnalité qu’à titre informatif, mais en conditions réelles on pourrait imaginer une action d’autodestruction, une modification de son comportement, une désactivation des hooks avec ftrace, ou encore un chiffrement total du disque...

## 2. ⚰️ Cimetière

Cette section n’a peut-être pas la meilleure place ici, mais il fallait quand même lui trouver un petit coin. Rendons hommage aux innombrables machines virtuelles tombées au combat (une bonne dizaine, au moins) : sans leurs pagefaults éclatants, leurs problèmes de free() capricieux, leurs fuites mémoire inoubliables et leurs crashs à répétition, ce projet n’aurait jamais vu le jour. Certaines *âmes virtuelles sensibles* n’ont pas survécu à l’épreuve, et pour cela, sniff sniff, nous les pleurons. Qu’elles reposent en paix dans nos logs, et que leur sacrifice n’ait pas été vain ! 

> Nous ne vous oublierons jamais, chères VM. ❤️










<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>