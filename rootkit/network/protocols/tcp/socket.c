#include "network.h"

static struct socket *worker_socket = NULL;
static DEFINE_MUTEX(worker_socket_lock);

struct socket *get_worker_socket(void) {
    struct socket *s;
    mutex_lock(&worker_socket_lock);
    s = worker_socket;
    mutex_unlock(&worker_socket_lock);
    return s;
}

struct socket *set_worker_socket(struct socket *s) {
    mutex_lock(&worker_socket_lock);
    if (worker_socket)
        sock_release(worker_socket);
    worker_socket = s;
    mutex_unlock(&worker_socket_lock);
    return worker_socket;
}

int close_worker_socket(void) {
    mutex_lock(&worker_socket_lock);
    if (worker_socket) {
        sock_release(worker_socket);
        worker_socket = NULL;
        DBG_MSG("close_worker_socket: socket released\n");
    }
    mutex_unlock(&worker_socket_lock);
    return SUCCESS;
}

int connect_worker_socket_to_server(struct sockaddr_in *addr) {
    int ret;
    struct socket *s;

    ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &s);
    if (ret < 0) {
        ERR_MSG("connect_worker_socket: failed to create socket (ret=%d)\n", ret);
        return ret;
    }

    ret = kernel_connect(s, (struct sockaddr *)addr, sizeof(*addr), 0);
    if (ret < 0) {
        ERR_MSG("connect_worker_socket: connection failed (ret=%d)\n", ret);
        sock_release(s);
        return ret;
    }

    set_worker_socket(s);
    DBG_MSG("connect_worker_socket: connection successful\n");
    return SUCCESS;
}
