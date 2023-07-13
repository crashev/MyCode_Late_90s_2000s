#define INTERIA_CZAT "217.74.65.63"

extern void say(int gdzie,char *fmt,...);
extern void setsock(int sock);
extern int open_listen_socket(int port);
extern int open_czat_socket(int port,char *host);
