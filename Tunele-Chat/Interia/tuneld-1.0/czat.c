/*
    Ksywki tez moga miec w interia poczatek '#' wiec jesli tak bedzie to 
    bec PRIVMSG nie zadziala
*/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#include "dumpchannels/kanaly.h"
#include "misc.h"


extern FILE *log;
extern void debug(FILE *gdzie,char *fmt,...);
extern int open_czat_socket(int port,char *host);
extern void close_conn(int i);


void space2czat(char *p) {
    int x;
    for (x=0;x<strlen(p);x++) if (p[x]=='_') p[x]=' ';
}

void *xalloc(size_t size ) {
    void *p;

    if (size < 0) {
    	    debug(log,"Malloc ERROR!(negative value: %d) \n",size);
	    return NULL;
    }
    p = malloc(size);
    if (!p) { 
	    debug(log,"Malloc ERROR!(size: %d) %s\n",size,strerror(errno));
	    exit(-1);
	}

    memset(p,0x0,size);

    return p;
}


void xfree(void *ptr) {
    if (ptr) 
	   free(ptr);
}

/* Zamienia dlugosc wysylki na format czatu - 2 bajty */

char *len2czat(int len) {
	static char l[2];
        l[0] = (char)(len & 0xff);
        l[1] = (char)(len >> 8 & 0xff);

	return l;
}

/* Zamienia dlugosc wysylki z czatu - z 2 bajtow na int */

int czat2len(char *p) {
    int pre;
    pre = ((p[1]<<8)&0xffff) | (p[0]&0xff);

    return pre;
}


int czat_join(struct client_struct *cli,char *kanal,int k,char *host) {
    char buf[255];
    char czan[255];
    int x=0;
    char *l,*r;
    int port;
    int len = strlen(cli->nick)+strlen(kanal)+5;

    // Konwersja _ do spacji
    bzero(czan,sizeof(czan));
    strncpy(czan,kanal,sizeof(czan)-1);
    space2czat(czan);

    /* 1 Sprawdzamy czy kanal jest na naszej liscie */
    do {
	bzero(buf,sizeof(buf));
	strcpy(buf,kanaly[x]);
	r = rindex(buf,':')+1;
	l = strtok(buf,":");
	if (r && l)
	    if (!strncmp(l,czan,strlen(l))) {
	        port = atoi(r);
if (cli->port!=port) {
		close(cli->toczat);
		if ( (cli->toczat = open_czat_socket(port,host))==-1) {
			close_conn(k);  
			return -1;
		    }
		cli->port = port;
		}
	        bzero(buf,sizeof(buf));
		debug(log,"[JOIN-CZAT]: %s \n",czan);
	        sprintf(buf,"%c%c%c%s%c%s%c",len&0xff,0x00,0x00,cli->nick,0x00,czan,0x00);
	        write(cli->toczat,buf,len);
		return 1;
	}
    } while (kanaly[++x]);
    return 0;

}

void czat_privmsg_kanal(struct client_struct *cli, char *msg) {
    int len = strlen(msg)+3;
    char *buf;
    debug(log,"[xalloc] czat_privmsg_kanal xalloc	\n");
    if ( !(buf=xalloc(len+1)) ) { debug(log,"[xalloc] null xalloc in czat_privmsg_kanal\n"); return; }
    sprintf(buf,"%c%c%c%s",len,0x00,0x01,msg);
    write(cli->toczat,buf,len);    
    xfree(buf);
}

char *czat_privmsg_crypt(char *nick, char *p, int length ) {
    char *tmp;
    int klucz=strlen(nick);
    int x,i;

    debug(log,"[xalloc] czat_privmsg_crypt xalloc	\n");
    debug(log,"[crypt-length]: %d \n",length);
    debug(log,"OD NICKA: %s \n",nick);
//    length = strlen(p);
//    if ( !(tmp = xalloc(strlen(p)+1)) )
    if ( !(tmp = xalloc(length+1)) )
       { debug(log,"[xalloc] null xalloc in czat_privmsg_crypt\n"); return NULL; }
    x=0;
    for (i=0;i<length;i++) {
	    	tmp[i]=p[i]^(nick[x]^88);
		if (x++==klucz-1) x=0;
    }

    return tmp;
}

void czat_privmsg_osoba(struct client_struct *cli,char *nick,char *wiadomosc) {
        char *buf,*ptr;
        int len;

        len = strlen(nick)+strlen(wiadomosc)+4;
//        len = strlen(nick)+4;
        debug(log,"[xalloc] czat_privmsg_osoba \n");
        if ( !(buf = xalloc(len+1)) )
	 { debug(log,"[xalloc] null xalloc in czat_privmsg_osoba\n"); return; }
       debug(log,"[PRIVMSG] Wysylamy wiadomosc : %s[%d] do %s\n",wiadomosc,strlen(wiadomosc),nick);
        ptr = czat_privmsg_crypt(nick,wiadomosc,strlen(wiadomosc));

//      sprintf(buf,"%c%c%c%s%c%s",len,0x00,0x03,nick,0x00,ptr);
        sprintf(buf,"%c%c%c%s%c",len,0x00,0x03,nick,0x00);
        write(cli->toczat,buf,len-strlen(wiadomosc));
	for (len=0;len<strlen(wiadomosc);len++) 
			write(cli->toczat,&ptr[len],1);
        xfree(ptr);
        xfree(buf);
}

char *czat_parse_priv(char *ptr, int len) {
	   char *tmpbuf; 
	   int i,x;
	   
	   debug(log,"[xalloc] czat_parse_priv \n");
	   if ( !(tmpbuf = xalloc(len)) )
	    { debug(log,"[xalloc] null xalloc in czat_parse_priv\n"); return NULL; }
	   for (i=0,x=0;i<len;i++) {
	    	switch (ptr[i] & 0xff) {
		    case 0x00: break;
		    case 0x01: i++;break;
		    case 0x02: i++;break;
		    default: tmpbuf[x++]=ptr[i];
	      }
	   }
	   
	   return tmpbuf;
}

