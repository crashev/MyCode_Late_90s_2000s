#ifndef _server_h
#define _server_h

#define ALLOW	1
#define DENY	0

#define YES 	1
#define NO	0

#define KEYFILE "server.pem"
#define PASSWORD "password"
#define DHFILE "dh1024.pem"

#define LOGFILE "debug/logfile.log"
#define MAXCLIENTS 50
#define SERWER_PORT 9000
#define ADMIN_SOCKET "admin/admin.sock"

/* PING/PONG */
#define PINGTIMEOUT	40	// Czas sprawdzic czy klient zyje - co 30 sekund
#define PONGTIMEOUT	80	// Zamkniecie polaczenia po uplywie 40 sekund
#define PINGPONGTIMEOUT 20	// Wysylka co minimum 10 sekund jak nie odpowiada w celu sprawdzenia

/* Directory with config files */
#define CONFIGS_DIR	"configs"
#define STATS_DIR	"/home/services/apcenterd/stats"
#define LOGS_DIR	"logs"
#define APCLIENT_DIR	"UPclientd" 	// for APclientd updates

#define SHOULDLOG	YES


/* Access list */
#define ACCESS_LIST "configs/access.list"
#define ACCESS_LIST_DEFAULTPOLICY DENY

/* Settime - synchronizacja */
#define SETTIME	NO

struct client_struct {
    char name[50]; 	// name of access point
    char host[255];	// host name - IP address
    char cversion[20];  // APclient version
    int fd;		// sock descriptor
    int stat;		// 0 - free, 1 - connected during SSL accept, 2 - connected, accepted
    int sstat;  	// sock stat , 0 - nothing, 1 - reading/write blocked, 2 - writing/read blocked
    SSL *ssl;		
    BIO *io;
    time_t ppong;	// PING/PONG mechanism
    time_t ppongt;	// PING/PONG flood check
};

/* SSL Stuff */

extern void load_dh_params(SSL_CTX *ctx, char *pcFile);
//void generate_eph_rsa_key(SSL_CTX *ctx);

/* Network stuff */
extern void set_socket_keepalive(int iSock);
extern int set_socket_nonblock(int iSock);
extern int open_listen_socket(int iPort);
extern int access_list(struct sockaddr_in *tu);

/* Local administravia stuff */
extern int open_admin_socket(char *pcLocalSock);

extern int settime(BIO *bio);

#endif

