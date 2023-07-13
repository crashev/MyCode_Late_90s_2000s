// Sms2irc
// By crashev <crashev@ebox.pl>
//            <crashev@crashev.net>
//
// version 4.0
//	  + kick reaction
//        + phone book
//        + eragsm sms module
// todo:
//   protection 
//   commands !users,!kick others :>

#define UNIX_PATH_MAX 107

#include "config.h"
#include <netinet/in.h> // sockaddr_in ip(7)
#include <sys/time.h>
#include <netdb.h>      // resolv 
#include <stdarg.h>     // *fmt ,va_start/end
#include <string.h>
#include <signal.h>

void breakup(int dummy)
{
    printf("Closing \n");
    unlink(SOCK_PATH);
    exit(0);
}


void say(int gdzie,char *fmt,...){
    char buf[4048];
    va_list ap;

    memset(buf,0x0,sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf,4040,fmt,ap);
    va_end(ap);
    write(gdzie,buf,strlen(buf));
    usleep(200);
}

int otworz_gniazdo_lokalne(char *path)
{
    	struct sockaddr_un sck;
        int pip;

        if (strlen(path)>UNIX_PATH_MAX) return -1;
	    sck.sun_family=AF_UNIX;
	    strcpy(sck.sun_path,path);	
        pip = socket(PF_LOCAL,SOCK_STREAM,0);	
	if (!pip) 
	    {
	    	perror("socket()-local");
	        return -1;
            }
	if (bind(pip,(struct sockaddr *)&sck,sizeof(sck))<0) 
	    {
	        perror("  - bind()-lokal ");
	        return -1;
            }
	listen(pip,10);
return pip;
}

int otworz_irc_polaczenie(char *nick,char *user,char *serwer,int port,char *chan)
{
    struct sockaddr_in sck;
    char buf[2048];
    struct hostent *hp;
    long ia;   
    int ret; 
    int ircek = socket(PF_INET,SOCK_STREAM,0);

    if (!ircek) 
    {
	perror("socket()-irc");
	return -1;
    }
    sck.sin_family=PF_INET;
    sck.sin_port=htons(port);
        hp=gethostbyname(serwer);
    if (hp) memcpy(&ia,hp->h_addr,4); else return -1;
    sck.sin_addr.s_addr=ia;
    if (connect(ircek,(struct sockaddr *)&sck,sizeof(sck))<0)
    {
        perror("  - connect()-irc");
	return -1;
    }
    printf(" + Connected \n");
    say(ircek,"USER user3214 host.jakis.pl dokic :%s\nNICK %s\n",user,nick);
    usleep(200);
    memset(buf,0x0,sizeof(buf));read(ircek,buf,sizeof(buf)-1);
    if (strstr(buf,"433")||(strstr(buf,"437")))
        {
	    	printf("Error: %s \n",buf);
		breakup(0);
	}
    if (strstr(buf,"ERROR :")) 
    {
    	printf("Error: %s ",strstr(buf,"ERROR :"));
	close(ircek);
	return -1;
    }
    printf(" = Registering connection\n");
    while (!strstr(buf,"376")) 
    {
        memset(buf,0x0,sizeof(buf));ret=read(ircek,buf,sizeof(buf)-1);
	if (strstr(buf,"376")) break;
	if (ret==0) { printf("last buf: %s \n",buf);break;}
    }
    printf(" + Connection registered\n");
    say(ircek,"JOIN #%s \n",chan);
    memset(buf,0x0,sizeof(buf));read(ircek,buf,sizeof(buf)-1);
// +b,+l,+i,+k,tempor
    if ((strstr(buf,"474"))||(strstr(buf,"473"))||(strstr(buf,"475"))||(strstr(buf,"471"))||(strstr(buf,"437")))
    {     
    	    	printf("Error: %s \n",buf);
		breakup(0);
	}

    return ircek;    
}
int irc_kick_reakcja(int ir,char *chan,char *who) 
{
    char tmp[1024];
    int t=1,ret;
    printf(" - I was kicked out of the channel by %s..\n",who);
    printf(" + Rejoining\n");
    say(ir,"JOIN #%s\n",chan);
    sleep(2); // Ravi,phantom i takie tam ;P
    bzero(tmp,sizeof(tmp));
    read(ir,tmp,sizeof(tmp)-1);
    if (strstr(tmp,"353")) return 0;
// +b,+l,+i,+k,tempor
while (1) 
    {
        t++;
  if ((strstr(tmp,"474"))||(strstr(tmp,"473"))||(strstr(tmp,"475"))||(strstr(tmp,"471"))||(strstr(tmp,"437")))
      {
        printf(" + [%d] Trying again - #%s",t,strstr(tmp,chan));fflush(stdout);
        say(ir,"JOIN #%s\n",chan);
	sleep(2);
    	bzero(tmp,sizeof(tmp));
        ret=read(ir,tmp,sizeof(tmp)-1);
      }
     if (strstr(tmp,"353")) return 0;      
    }    
 printf("FinalErrorBuf: %s \n",tmp);
return -1;
}

void irc_reakcja(int ir,char *buff,char *nick,char *kanal){
 char kto[50];
 char *ptr=NULL;

      ptr=strtok(buff,"\n");
      if (strstr(buff,"KICK")) 
      {
        sscanf(strstr(buff,"KICK"),"KICK %*s %s %*s",kto);
        if (!strcmp(kto,nick)) 
	       {
	    	  if (irc_kick_reakcja(ir,kanal,strtok(ptr," "))==-1) exit(0);
		  else
		  printf("  + Rejoined\n");
		}

      }

    while ((ptr=strtok(NULL,"\n")))
      {
      if (strstr(buff,"KICK")) 
      {
            sscanf(strstr(buff,"KICK"),"KICK %*s %s %*s",kto);
    	    if (!strcmp(kto,nick)) 
	        {
	    	  if (irc_kick_reakcja(ir,kanal,strtok(ptr," "))==-1) exit(0); else
		  printf("  + Rejoined\n");
		}
       }

      }
 
}


int main(int argc,char *argv[]) {
	char buf[2048];
	int lok,kli,irc;
	int len,i;
	int linia=0;
    	struct sockaddr_un sck;
        char *ptr;
	fd_set fds;
if (argc<4) 
    {
	printf("Usage: %s [nick] [\"RealName\"] [irc-chan]\n",argv[0]);
	return -1;
}
    signal(SIGINT,breakup);
printf("\nStarting....\n");
printf("  + Nickname  : %s \n",argv[1]);
printf("  + RealName  : %s \n",argv[2]);
printf("  + Irc-Chan  : #%s \n\n",argv[3]);	      
if ((strlen(argv[1])>9)||(strlen(argv[3])>20)) 
    {
	printf("  - Nickname or channel name too long\n");
	breakup(0);
    }
    lok=otworz_gniazdo_lokalne(SOCK_PATH);
	if (lok==-1) breakup(0);
reconn:
i=0;
while (ircserver[i].name!=NULL)
{
    printf("Trying: %s : %d\n",ircserver[i].name,ircserver[i].port);
    irc=otworz_irc_polaczenie(argv[1],argv[2],ircserver[i].name,ircserver[i].port,argv[3]);
    if ( irc>0 ) break; else printf("  - Failed to connect to : %s : %d\n",ircserver[i].name,ircserver[i].port);
    i++;
}
    if (irc==-1) breakup(0);

    FD_ZERO(&fds);
    FD_SET(irc,&fds);
    FD_SET(lok,&fds);

while (1) 
{
  select(irc + 1,&fds,NULL,NULL,NULL);
if (FD_ISSET(lok,&fds)) 
{
      kli = accept(lok,&sck,&len);
      bzero(buf,sizeof(buf));
      read(kli,buf,sizeof(buf)-1);
      ptr=strtok(buf,"\n");
      printf("Zapis : [%d]%s \n",strlen(ptr),ptr);
      say(irc,"PRIVMSG #%s :%s\n",argv[3],ptr);
      linia++;
    while ((ptr=strtok(NULL,"\n")))
      {
	  printf("Zapis : %d) [%d]%s \n",linia,strlen(ptr),ptr);
	  say(irc,"PRIVMSG #%s :%s\n",argv[3],ptr);linia++;
	  if (linia>MAXLINES) break;
      }
      say(kli,"OK #%s",argv[3]);
      close(kli);
}
if (FD_ISSET(irc,&fds)) 
    {
      bzero(buf,sizeof(buf));
      len=read(irc,buf,sizeof(buf)-1);
      if (len<=0) 
      {
       printf("  - Connection Lost!\n");
       printf("  = Reconnecting....\n");
       goto reconn;
      }
      if (strstr(buf,"PING"))
      {
          say(irc,"PING :%s\n",ircserver[i].name);
	  sleep(2);
      }
    irc_reakcja(irc,buf,argv[1],argv[3]);
    }
    FD_ZERO(&fds);
    FD_SET(irc,&fds);
    FD_SET(lok,&fds);
}    
    return 0;
}   