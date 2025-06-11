#include "vanish.h"

#include <asm/cpufeature.h>
#include <linux/dmi.h>

#include "config.h"

/**
 * @brief Checks if the system is running under a hypervisor.
 *
 * This function uses the CPU feature flags to determine if the system
 * is running under a hypervisor.
 *
 * @return `true` if a hypervisor is detected, `false` otherwise.
 */
bool check_hypervisor(void) {
    return boot_cpu_has(X86_FEATURE_HYPERVISOR);
}

/**
 * @brief Checks if the system is running in a known virtualized environment.
 *
 * This function uses DMI (Desktop Management Interface) system information
 * to check for known virtual machine vendors such as VMware, VirtualBox,
 * QEMU, and others lol.
 *
 * @return `true` if a virtualized environment is detected, `false` otherwise.
 */
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

/**
 * @brief Determines if the system is running in a virtualized environment.
 *
 * This function combines the results of `check_hypervisor` and `check_dmi`
 * to determine if the system is running in a virtualized environment.
 *
 * @return `true` if the system is running in a virtualized environment, `false`
 * otherwise.
 */
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
