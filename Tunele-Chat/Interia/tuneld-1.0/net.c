#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

extern FILE *log;
extern void finish( int sig );
extern void debug(FILE *gdzie,char *fmt,...);


void say(int gdzie,char *fmt,...){
    char buf[4048];
    va_list ap;

    memset(buf,0x0,sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf,4040,fmt,ap);
    va_end(ap);
    write(gdzie,buf,strlen(buf));
}

void setsock(int sock) {
    int opt;
    opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR, &opt , sizeof(opt));
    if (fcntl(sock, F_SETFL, O_NONBLOCK)<0) {
	debug(log,"[-] fcntl-setnonblock error ");
	finish(-1);
    }

}


int open_listen_socket(int port) {
    struct sockaddr_in sin;
    int sock;
    
    sin.sin_family      = PF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port 	= htons(port);
    
    if ( (sock = socket(PF_INET, SOCK_STREAM, 0)) <0 ) {
	perror("[-] Socket error ");
	finish(-1);
    }
    setsock(sock);
    if ( (bind(sock, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
	perror("[-] Bind error ");
	finish(-1);
    }
    listen(sock, 10);    
    
    return sock;
}




int open_czat_socket(int port,char *interia_server) {
    struct sockaddr_in sin;
    int sock;
    
    if ( (sock = socket(PF_INET, SOCK_STREAM, 0))<0 ) {
//	perror("[ToCzat-Socket()] ");
	return -1;
    }
    sin.sin_family      = PF_INET;
    sin.sin_port        = htons(port);
    sin.sin_addr.s_addr = inet_addr(interia_server);
    
    if ( (connect(sock, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
//	perror("[ToCzat-Connect()] ");
	return -1;
    }
    
    return sock;
}

int set_socket_keepalive(int sock ) {
 int flag=1;
 struct linger linger;

 linger.l_onoff=0; linger.l_linger=0;
 setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
 setsockopt(sock,SOL_SOCKET,SO_LINGER,&linger,sizeof(linger));
 setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,&flag,sizeof(flag));
 return 0;
}

