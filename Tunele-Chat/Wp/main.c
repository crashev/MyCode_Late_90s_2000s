/* WP Gate v0.4 */
/* by Sebastian Rosenkiewicz <rawsock@ds.pg.gda.pl> */
/*
    Poprawki: Pawel Furtak <crashev@ebox.pl>
    - get_magic()
    - przekierowanie na IP zawarte w REALNAME + validip()
    - poprawnie kluczy wyliczajacych token
    - reads zmienione
    - random_ip() - porzucone
    - w idencie lokalne IP lub IP redirecta z realname

*/

#include "main.h"
#include "conf.h"
#include "magic.h"
#include <time.h>

#include <arpa/inet.h>

int NALLOW;
char MAGIC[32]="",NICK[256]="",TMPP[32]="";
struct config conf;
char *allow[MAXALLOW];

char *getlocalip() {
    static char ip[255];
    char buf[255];
    struct in_addr ina;
    struct hostent *hp;
    bzero(buf,sizeof(buf));
    gethostname(buf,sizeof(buf)-1);
    if ( (hp=gethostbyname2(buf,AF_INET)) ) {
	bcopy(hp->h_addr,(void *)&ina.s_addr,hp->h_length);
	strcpy(ip,inet_ntoa(ina));
	return (char *)&ip;
    }
}

void finish(int sig) {
 switch(sig) {
    case SIGPIPE: return;
    case SIGSEGV: puts("Oops! Crashing!");
		  exit(sig);
    case SIGCHLD: while(waitpid(-1,NULL,WNOHANG)>0);
		  return;
    default: exit(sig);
 }
}

char *random_ip() {
    static char buf[20];
    int i1=0,i2=0,i3=0,i4=0;
    i1 = (int) (255.0*rand()/(RAND_MAX+1.0));
    i2 = (int) (255.0*rand()/(RAND_MAX+1.0));
    i3 = (int) (255.0*rand()/(RAND_MAX+1.0));
    i4 = (int) (255.0*rand()/(RAND_MAX+1.0));
    bzero(buf,sizeof(buf));
    sprintf(buf,"%d.%d.%d.%d",i1,i2,i3,i4);
    return (char *)buf;
}

int validip(char *p) {
    int a=-1,b=-1,c=-1,d=-1;
    sscanf(p,"%d.%d.%d.%d",&a,&b,&c,&d);
    if ((a<0)||(b<0)||(c<0)||(d<0)) return 0;
    return 1;
}

char *make_message(const char *fmt,va_list ap) {
 int n,size=2048;
 char *p;

 if ((p=malloc(size))==NULL) return NULL;
 while (1) {
    n=vsnprintf(p,size,fmt,ap);
    if (n>-1&&n<size) return p;
    if (n>-1) size=n+1;
    else size*=2;
    if ((p=realloc(p,size))==NULL) return NULL;
 }
}

void my_dprintf2(int *sock,const char *fmt,...) {
 char *str;
 va_list args;

 va_start(args,fmt);
 str=(char *)make_message(fmt,args);
 if (!str) return;
 va_end(args);
 send(*sock,str,strlen(str),0);
 printf("[MY]: %s \n",str);
 free(str);
}

void my_dprintf(int *gdzie,char *fmt,...){
    char buf[4048];
    va_list ap;

    memset(buf,0x0,sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf,4040,fmt,ap);
    va_end(ap);
    write(*gdzie,buf,strlen(buf));
}


void setsockopts(int *sock) {
 int flag=1;
 struct linger linger;

 linger.l_onoff=0; linger.l_linger=0;
 setsockopt(*sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
 setsockopt(*sock,SOL_SOCKET,SO_LINGER,&linger,sizeof(linger));
 setsockopt(*sock,SOL_SOCKET,SO_KEEPALIVE,&flag,sizeof(flag));
}

int connect_tcp(int *sock,char *host,char *port,char proto) {
 int error;
 struct addrinfo hints;
 struct addrinfo *res;

 bzero(&hints,sizeof(hints));
 switch(proto) {
    case 1: hints.ai_family=PF_INET; // force IPv4
	    break;
    case 2: hints.ai_family=PF_INET6; // force IPv6
	    break;
    default: hints.ai_family=PF_UNSPEC; // auto
 }
 hints.ai_socktype=SOCK_STREAM;
 error=getaddrinfo(host,port,&hints,&res);
 if (error) { perror(gai_strerror(error)); finish(0); }
 if ((*sock=socket(res->ai_family,res->ai_socktype,res->ai_protocol))==-1)
    { perror("socket"); finish(0); }
 error=connect(*sock,res->ai_addr,res->ai_addrlen);
 freeaddrinfo(res);

 return error;
}

int reads(int *sock,char *buf) {
 int c,count=0;

 do c=read(*sock,&buf[count],1); while ((c>0)&&(buf[count++]!='\n'));
 buf[count]='\0';
 return (c<=0)?c:count;
}

void randomize() {
 FILE *rdev;
 unsigned int seed;
 if ((rdev=fopen("/dev/urandom","r"))!=NULL) { seed=fgetc(rdev); fclose(rdev); }
 seed+=getpid()+time(NULL);
 srandom(seed);
}

void rnick(char *nick) {
 const char fill1[]="aeuioy";
 const char fill2[]="qwrtpsdfghjklzxcvbnm";
 int x,y;
 y=3+((random()%100)/25);
 for (x=0;x<y;x++)
 {
  if ((x==1)||(x==3)||(x==5)||(x==7)) nick[x]=fill1[((random()%10)+1)/2];
  else nick[x]=fill2[(random()%100)/5];
 }
 nick[y]=0;
}

void client_to_server(int *sockin,int *sockout) {
 int count;
 char buf[BUFSIZE],*p;

 count=reads(sockin,buf);
 if (count<=0) finish(0);
 if (!strncasecmp(buf,"JOIN",4)) {
    p=strtok(buf," ");
    p=strtok(NULL,"\r\n");
    if (!p) return;
    if (!*p) return;
    my_dprintf(sockout,"WPJOIN %s\r\n",p);
 } else send(*sockout,buf,count,0);
}

void server_to_client(int *sockin,int *sockout) {
 int count,i;
 char buf[BUFSIZE],tmp[BUFSIZE],*p,*cmd,*par;

 count=reads(sockin,buf);
 if (count<=0) {
        close(*sockin);
	close(*sockout);
	finish(0);
    }
 if (buf[0]!=':') return;

 strcpy(tmp,buf);
 p=strtok(tmp," ");
 cmd=strtok(NULL," ");
 if (cmd) if (*cmd) {
    if (!strcasecmp(cmd,"WPJOIN")) {
        par=strtok(NULL,"\r\n");
        if (!par) return;
        if (!*par) return;
        my_dprintf(sockout,"%s JOIN %s\r\n",p,par);
        return;
    }
    if (!strcasecmp(cmd,"MAGIC")) {
	return;
    }
    if (!strcasecmp(cmd,"MAGICU")) {
	return;
    }
 }

 /* rip out HTML garbage */
 if (strchr(buf,'<')&&strchr(buf,'>')) {
    count=0;
    i=0;
    while (buf[count]) {
	while(buf[count]&&(buf[count]!='<')) tmp[i++]=buf[count++];
	while(buf[count]&&(buf[count]!='>')) count++;
	if (buf[count]=='>') count++;
    }
    tmp[i]='\0';
 } else strcpy(tmp,buf);

 /* rip out gfx garbage */
 if (strchr(buf,'{')&&strchr(buf,'}')) {
    count=0;
    i=0;
    while (tmp[count]) {
	while(tmp[count]&&(tmp[count]!='{')) buf[i++]=tmp[count++];
	while(tmp[count]&&(tmp[count]!='}')) count++;
	if (tmp[count]=='}') count++;
    }
    buf[i]='\0';
 } else strcpy(buf,tmp);

 send(*sockout,buf,strlen(buf),0);
}

void get_magic(int *clientsock,const char *nick) {
 int serversock;
 char buf[BUFSIZE],*p;
 char ticket[500];
 char location[500];
/* faza 1 - polaczenie do profil.wp.pl ,zebranie ticketu */

  if (connect_tcp(&serversock,"profil.wp.pl","80",0)==-1) {   perror("connect"); finish(0);}
    my_dprintf(&serversock,"GET /ticket?ver=1.0&backUrl=http%%3A%%2F%%2Fczat.wp.pl%%2Fchat.html%%3Fnoframeset%%3D1%%26%%2526npklr%%253Dp%%3D%%26i%%3D29074%%26allOpt%%3Dnie%%26czatname%%3DKoszalin%%26nick%%3D%s%%26x%%3D46%%26y%%3D11&service=czat.wp.pl&uD=1813787902 HTTP/1.1\r\n",NICK);
    my_dprintf(&serversock,"Host: profil.wp.pl\r\n\r\n");

 bzero(location,sizeof(location));
 bzero(ticket,sizeof(ticket));
 while (reads(&serversock,buf)>0) {
    if (strstr(buf,"wpsticket")) {
	if ( (p=strchr(buf,'=')) ) {
	    p++;
	    strtok(p,";");
	    strncpy(ticket,p,sizeof(ticket)-1);
//	    printf("ticket: %s \n",ticket);
	}
    }
    if (strstr(buf,"Location:")) {
	if ( (p=strchr(buf,':')) ) {
	    p++;
	    strtok(p,"\r\n");
	    strncpy(location,p,sizeof(location)-1);
//	    printf("Location: %s \n",location);
	}
    }

}
    close(serversock);

  if (connect_tcp(&serversock,"profil.wp.pl","80",0)==-1) {   perror("connect"); finish(0);}
    my_dprintf(&serversock,"GET %s HTTP/1.1\r\n",location);
    my_dprintf(&serversock,"Host: profil.wp.pl\r\n");
    my_dprintf(&serversock,"Cookie: wpsticket=%s;\r\n\r\n",ticket);

 bzero(location,sizeof(location));
 while (reads(&serversock,buf)>0) {
    if (strstr(buf,"Location:")) {
	if ( (p=strchr(buf,':')) ) {
	    p++;
	    strtok(p,"\r\n");
	    strncpy(location,p,sizeof(location)-1);
//	    printf("Location: %s \n",location);
	}
    }
}
//printf("Final Location: %s \n",location);
    close(serversock);
if (connect_tcp(&serversock,"czat.wp.pl","80",0)==-1) {   perror("connect"); finish(0);}
    my_dprintf(&serversock,"GET %s HTTP/1.1\r\n",location);
    my_dprintf(&serversock,"Host: czat.wp.pl\r\n\r\n");


 while (reads(&serversock,buf)>0) {
printf("[LINIA]: %s \n",buf);
//    if (  (strstr(buf,"PARAM"))&&(strstr(buf,"name=\"magic\""))    )
    if (  (strstr(buf,"param"))&&(strstr(buf,"name=\"magic\""))    )
	if ((p=strstr(buf,"value=\""))) {
	    p+=7;
	    strtok(p,"\"");
	    if (strlen(p)!=sizeof(MAGIC)) continue;
	    memcpy(MAGIC,p,sizeof(MAGIC));
	    printf("got magic: %s \n",MAGIC);
//	    break;
	}
 }

 if (!*MAGIC) { my_dprintf(clientsock,":wp.gate No MAGIC number received from HTTP server\n"); finish(0); }

 close(serversock);
}

void tunnel(int *clientsock) {
 int serversock,tmpsock,maxfd,i;
 char buf[BUFSIZE],tmp[BUFSIZE],tmpnick[64],pad[64]="00",*p,*uname=NULL,*umode=NULL,*urealname=NULL;
 fd_set sockfdset,selectfdset;

 srand(time(NULL));
 maxfd=getdtablesize();

 /* get NICK and USER from client */
 do {
    if (reads(clientsock,buf)<=0) finish(0);
    if (!strncasecmp(buf,"NICK",4)) {
	strtok(buf," :");
        p=strtok(NULL," :\r\n");
	if (!p) finish(0);
        if (!*p) finish(0);
	strcpy(NICK,p);
    }
    if (!strncasecmp(buf,"USER",4)) {
	strcpy(tmp,buf);
	p=strtok(tmp," ");
        uname=strtok(NULL," ");
	if (!uname) finish(0);
        if (!*uname) finish(0);
        umode=strtok(NULL," ");
	if (!umode) finish(0);
        if (!*umode) finish(0);
        p=strtok(NULL," ");
	if (!p) finish(0);
        if (!*p) finish(0);
        urealname=strtok(NULL,":\r\n");
	if (!urealname) finish(0);
        if (!*urealname) finish(0);
	if (validip(urealname)) {
	if (conf.target_host) free(conf.target_host);
		conf.target_host = strdup(urealname);
	}

    }
 } while ((!*NICK)||(!uname));

 /* register tmp user */
 if (conf.registered) {
    randomize();
    rnick(tmpnick);
    get_magic(clientsock,tmpnick);
    if (connect_tcp(&tmpsock,conf.target_host,conf.target_port,0)==-1) {
	perror("connect"); finish(0);
    }
    for (i=0;i<(strlen(tmpnick)-1)/4;i++) strcat(pad,"00");
    my_dprintf(&tmpsock,"NICK a%s|%s\r\n",tmpnick,pad);
    printf("NICK: a%s|%s \n",tmpnick,pad);
    if (reads(&tmpsock,buf)<=0) finish(0);
    p=strchr(buf+1,':');
	    if (!p) { 
			my_dprintf(clientsock,"%s\n",buf); 
			finish(0);
		    }
    p++;
    strtok(p,"\r\n");
    if (strlen(p)!=8) { my_dprintf(clientsock,"%s\n",buf); finish(0); }
//    my_dprintf(&tmpsock,"USER %s %s %s :%s\r\n",random_ip(),umode,do_magic(p,MAGIC),urealname);
    if (validip(urealname))
	my_dprintf(&tmpsock,"USER %s %s %s :Czat-Applet\r\n",conf.target_host,umode,do_magic(p,MAGIC));
    else
        my_dprintf(&tmpsock,"USER %s %s %s :Czat-Applet\r\n",getlocalip(),umode,do_magic(p,MAGIC));
    if (reads(&tmpsock,buf)<=0) finish(0);
 }

 /* register user */
 if (conf.registered) get_magic(clientsock,tmpnick);
    else get_magic(clientsock,NICK);
 if (connect_tcp(&serversock,conf.target_host,conf.target_port,0)==-1) {
    perror("connect"); finish(0);
 }
 if (conf.registered) my_dprintf(&serversock,"NICK a%s|%s\r\n",tmpnick,pad);
 strcpy(pad,"00");
 for (i=0;i<(strlen(NICK)-1)/4;i++) strcat(pad,"00");
 if (!conf.registered)  
	my_dprintf(&serversock,"NICK a%s|%s\r\n",NICK,pad);
    

 if (reads(&serversock,buf)<=0) finish(0);
 p=strchr(buf+1,':');
 if (!p) { my_dprintf(clientsock,"%s\n",buf); finish(0); }
 p++;
 strtok(p,"\r\n");
 if (strlen(p)!=8) { my_dprintf(clientsock,"%s\n",buf); finish(0); }
// strcpy(TMPP,p);

 /* read magicu */
/* if (reads(&serversock,buf)<=0) finish(0);
     if (strchr(buf,' ')) strcpy(MAGIC,strchr(buf,' ')+1);
	     else
	 printf("[MAGIC FAILED TO FOUND \n");
// printf("P: %s AND MAGICU: %s \n",TMPP,MAGIC);
*/
    if (validip(urealname))
       my_dprintf(&serversock,"USER %s %s %s :Czat-Applet\r\n",conf.target_host,umode,do_magic(p,MAGIC));
   else
       my_dprintf(&serversock,"USER %s %s %s :Czat-Applet\r\n",getlocalip(),umode,do_magic(p,MAGIC));
 if (conf.registered) {
    if (reads(&serversock,buf)<=0) finish(0); /* suppress nick-already-in-use */
    my_dprintf(&serversock,"NICK b%s|%s\r\n",NICK,pad);
    close(tmpsock);
 }
 /* data transfer */
 setsockopts(clientsock);
 setsockopts(&serversock);
 FD_ZERO(&sockfdset);
 FD_SET(*clientsock,&sockfdset);
 FD_SET(serversock,&sockfdset);
 while(1) {
    bcopy(&sockfdset,&selectfdset,sizeof(selectfdset));
    if (select(maxfd,&selectfdset,NULL,NULL,NULL)<0) {
	if (errno!=EINTR) perror("select");
	continue;
    }
    if (FD_ISSET(*clientsock,&selectfdset)) client_to_server(clientsock,&serversock);
    if (FD_ISSET(serversock,&selectfdset)) server_to_client(&serversock,clientsock);
 }
}

int main(int argc,char *argv[]) {
 int new_sock,listen_sock,remote_size,count;
 char *remoteip;
 struct sockaddr_in remote;
 struct sockaddr_in local;

 signal(SIGPIPE,finish);
 signal(SIGINT,finish);
 signal(SIGTERM,finish);
 signal(SIGSEGV,finish);
 signal(SIGCHLD,finish);
 bzero(&conf,sizeof(struct config));

 parse_config();
 parse_allow();

 if ((listen_sock=socket(AF_INET,SOCK_STREAM,0))==-1) { perror("socket"); finish(0); }
 setsockopts(&listen_sock);
 local.sin_family=AF_INET;
 local.sin_port=htons(conf.listen_port);
 local.sin_addr.s_addr=INADDR_ANY;
 bzero(&(local.sin_zero),8);
 if (bind(listen_sock,(struct sockaddr *)&local,sizeof(struct sockaddr))<0) { perror("bind"); finish(0); }
 if (listen(listen_sock,5)<0) { perror("listen"); finish(0); }
// if (daemon(0,0)<0) { perror("daemon"); finish(0); }
 while(1) {
    remote_size=sizeof(struct sockaddr_in);
    new_sock=accept(listen_sock,(struct sockaddr *)&remote,&remote_size);
    if (new_sock<0) {
	if (errno!=EINTR) { perror("accept"); finish(0); } else continue;
    }
    if (NALLOW) {
	remoteip=inet_ntoa(remote.sin_addr);
	for(count=0;count<NALLOW;count++)
	if (!strcmp(allow[count],remoteip)) break;
	if (count>=NALLOW) { close(new_sock); continue; }
    }
    switch(fork()) {
	case -1: perror("fork"); finish(0);
	case  0: close(listen_sock); tunnel(&new_sock);
	default: close(new_sock);
    }
 }
 finish(0);
 return 0;
}
