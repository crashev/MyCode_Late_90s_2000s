#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define BUFSIZE 4096
#define MAXALLOW 128
#define CONFIG "wpg.conf"
#define ALLOW "wpg.allow"

/* Filled with config file data. */
struct config {
 unsigned short int listen_port;
 char *http_proxy_host;
 char *http_proxy_port;
 char *target_host;
 char *target_port;
 char registered;
};

void finish(int sig);
