/*

    Interia Czat to IRC tunnel
    Version 1.0 Beta
    Pawel Furtak - All rights reserved
    
    TODO: - timeout przy connect to czat
	  - poprawka w dlugosci wysylania - uzywac 2 bajtow
	  - ! i @ sprawiaja problemy w parsowaniu ksywek
*/


#include <stdio.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdarg.h>
#include <string.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include "net.h"
#include "misc.h"
#include "dec.h"

#define PORT 6667
#define MAXCLIENTS 50
#define SERVER "pawq.eu.org"
#define LOG "debug.log"


int count = 0;
const char *komendy[]={"MODE ","USERHOST ","NICK ","PING","JOIN","PRIVMSG ","LIST",NULL};
struct client_struct clients[MAXCLIENTS];

extern const char *kanaly[];
extern int czat_join(struct client_struct *cli,char *kanal,int k,char *host);
extern void czat_privmsg_kanal(struct client_struct *cli, char *msg);
extern void czat_privmsg_osoba(struct client_struct *cli,char *nick,char *wiadomosc);
extern char *czat_parse_priv(char *ptr, int len);
extern char *czat_privmsg_crypt(char *nick, char *p, int length );
extern void *xalloc(size_t size );
extern void xfree(void *ptr);
extern void debug(FILE *gdzie,char *fmt,...);
extern int set_socket_keepalive(int sock );



FILE *log;


int validip(char *p) {
    int a=-1,b=-1,c=-1,d=-1;
    sscanf(p,"%d.%d.%d.%d",&a,&b,&c,&d);
    if ((a<0)||(b<0)||(c<0)||(d<0)) return 0;
    return 1;
}


int reads(int *sock,char *buf) {
 int c,count=0;
 do c=read(*sock,&buf[count],1); while ((c>0)&&(buf[count++]!='\n'));
 buf[count]='\0';
 return (c<=0)?c:count;
}


int find_free() {
    int c;
    for (c=0; c<MAXCLIENTS; c++) if (clients[c].stat == 0) return c;
    return -1; // no free availble
}


void send_motd(int b,char *nick) {
    say(b,":%s 001 %s :Welcome to the Internet Relay Network %s!pawq@czat.interia.pl\n",SERVER,nick,nick);
    say(b,":%s 002 %s :Your host is %s, running version 1.0beta\n",SERVER,nick,SERVER);
    say(b,":%s 003 %s :This server was created XXX XXX XX XXXX XX XX:XX:XX CEST\n",SERVER,nick);
    say(b,":%s 004 %s %s 1.0beta aoOirw abeiIklmnoOpqrstv\n",SERVER,nick,SERVER);
    say(b,":%s 376 %s :End of MOTD command.\n",SERVER,nick);
    say(b,":%s MODE %s :+ir\n",nick,nick);
}


void close_conn(int i) {  
	        debug(log,"[--] Connection closed by %d \n",i);
                debug(log,"[--] Closing %d %d \n",clients[i].fd,clients[i].toczat);
                if (clients[i].fd>0) close(clients[i].fd);
                if (clients[i].toczat>0) close(clients[i].toczat);
		fflush(0);
	        clients[i].stat = 0;
		bzero(&clients[i],sizeof(clients[i]));
                count--;
}


void finish( int sig ) {
switch (sig) {
    case SIGPIPE: return;
    case SIGSEGV: debug(log,"SIGSEGV!!! crashing... \n");
		  exit(sig);
    case SIGCHLD: while(waitpid(-1,NULL,WNOHANG)>0);
		  return;
    default: exit(sig);
    }
}

void parse_irc_cmd(char *buf,int k) {
    char *ptr;
    int x,b=clients[k].fd;
    int ret;

// Znamy komende? 

        x=0;
	do {
	    if (!strncasecmp(buf,komendy[x],strlen(komendy[x]))) 
    		goto kom;
	} while ( komendy[++x] );

        strtok(buf," ");

    if (clients[k].stat==10)
          say(b,":%s 421 %s %s :Unknow command\r\n",SERVER,clients[k].nick,buf);

        return;

       kom:
       if (clients[k].stat==5) return;
    
        if (!strncasecmp(buf,"USERHOST ",9)) { 
	    say(b,":%s 302 %s :%s=+%s@czat.interia.pl\r\n",SERVER,clients[k].nick,buf+9,buf+9);
        } 
        if (!strncasecmp(buf,"PRIVMSG ",8)) { 
	switch (buf[8]) {
		case '#': if (strchr(buf,':')) 
		    czat_privmsg_kanal(&clients[k],strchr(buf,':')+1);
		    break;
    	default:  if (strchr(buf,':')) 
		    ptr=strdup(buf+8);
		    if (!ptr) { debug(log,"!!! Error - strdup() : %s \n",strerror(errno));return;}
		    strtok(ptr," :");
		    if (ptr==NULL) return;
	/* 
	    amortyzacja polaczona z bledem 0x05(ostrzezenie) 
	*/
	czat_privmsg_osoba(&clients[k],ptr,strchr(buf,':')+1);
		    free(ptr);
		    break;
	    }
        } 

	// MODE pawq128 +i -> :pawq128 MODE pawq128 :+i

        if (!strncasecmp(buf, "MODE ", 5)) {
//            ptr = strchr(buf,' ');
//	/    ptr++;
	//    strtok(ptr," ");
//	    say(b,":%s 484 %s :Your connection is restricted!\r\n",SERVER,buf+5);

        }
	if (!strncasecmp(buf,"NICK ",5)) {
//    	    say(b,":%s 484 %s :Your connection is restricted!\r\n",SERVER,buf+5);
        }
        if (!strncasecmp(buf,"PING ",5)) {
		// PING 1072196728 poznan.irc.pl
	        strtok(buf," ");
		ptr = strtok(NULL," ");
		ptr = strtok(NULL," ");
	if (ptr==NULL)
		say(b,":%s PONG %s :%s\r\n",SERVER,SERVER,clients[k].nick);
	     else 
	    	say(b,":%s PONG %s :%s\r\n",ptr,ptr,clients[k].nick);
        }
	if (!strncasecmp(buf,"LIST",4)) {
	    x=0;
	    do {
		say(b,":%s 322 %s #%s 5 :Interia czat\r\n",SERVER,clients[k].nick,kanaly[x]);
	    } while (kanaly[++x]);
	}

	if (!strncasecmp(buf,"JOIN ",5)) {
	ptr = buf+5;
	strtok(ptr," ");
  if (clients[k].stat == 10) {
	    say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",clients[k].nick,clients[k].nick,clients[k].kanal,"opusczamy pokoj");
	    clients[k].stat = 4;
	    }
  if (clients[k].stat != 5) {
	    say(b,":%s!%s@czat.interia.pl JOIN :%s\r\n",clients[k].nick,clients[k].nick,ptr);
            say(b,":%s 332 %s %s :Kanal %s w interia czat\r\n",SERVER,clients[k].nick,ptr,ptr);
            say(b,":%s 333 %s %s crashev!tunel@admin 1071952070\r\n",SERVER,clients[k].nick,ptr);
	    say(b,":%s 353 %s = %s :%s ",SERVER,clients[k].nick,ptr,clients[k].nick);
		    fflush(0);
		    usleep(500);
	    	    clients[k].stat = 5;
	    strncpy(clients[k].kanal,ptr+1,sizeof(clients[k].kanal)-1);
	    ret = czat_join(&clients[k],ptr+1,k,clients[k].czathost);
	    if ( ret == 0 ) {
	    	   	  debug(log,"No such channel! \n");
	                  say(clients[k].fd,"\r\n:%s 366 %s %s :End of NAMES list.\r\n",SERVER,clients[k].nick,clients[k].kanal);
 			  say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",clients[k].nick,clients[k].nick,clients[k].kanal,"nie istnieje");
                          say(clients[k].fd,"\r\n:%s 403 %s #%s :No such channel\r\n",SERVER,clients[k].nick,clients[k].kanal);
		          clients[k].stat=4;
			  return;
		}
//		czat_join(&clients[k],ptr+1);
	    }
	}

}




void client_to_czat(char *p, int k) {
    char buf[4048];
    char *ptr;

    bzero(buf,sizeof(buf));
    strcpy(buf,p);
    strtok(buf,"\r\n");
    if (clients[k].stat<3) {
	if (!strncasecmp(buf,"USER ",5)){
		    strtok(buf,"\r\n");
	    if (strchr(buf,':'))
	    strncpy(clients[k].user,strchr(buf,':')+1,sizeof(clients[k].user)-1);
			    else
	    strncpy(clients[k].user,buf+5,sizeof(clients[k].user)-1);
//		    strtok(clients[k].user," ");
		    clients[k].stat++;
	    /* jesli ip zostalo podane w user to zmieniamy */
	    if (validip(clients[k].user)) { 
	        strncpy(clients[k].czathost,clients[k].user,253);
		debug(log,"[IP-%d] Set new IP based on USER Realname info: %s \n",k,clients[k].czathost);
	    }
		    
	}
        if (!strncasecmp(buf,"NICK ",5)){
		    strtok(buf," :");
		    ptr = strtok(NULL," :\r\n");
		    if (!ptr) { debug(log,"!!! Error null NICK!! \n"); return;}
		    strncpy(clients[k].nick,ptr,sizeof(clients[k].nick)-1);
		    clients[k].stat++;
	}
    }
    if ( clients[k].stat == 3 ) { 
	    debug(log,"[>>>] NICK: %s, USER %s \n",clients[k].nick,clients[k].user);
	    send_motd(clients[k].fd,clients[k].nick);
	    clients[k].toczat = open_czat_socket(14012,clients[k].czathost);
	    clients[k].port = 14012;
	    if (clients[k].toczat == -1) { 
		debug(log,"[ERROR CONNECTING TO CZAT: %s]\n",clients[k].czathost);
	        close_conn(k);
	        return;
	    }
	    clients[k].stat++;
	    return;
	}
    if (clients[k].stat>3) {
//		debug(log,"[IRC 2 CZAT]: %s \n",buf);
		parse_irc_cmd(buf,k);
    }

}



void parse (char *ptr, int len,int k) {
    int x,pre;
    int pp,ret;
    char *p,*tmp;
    int counter=0;
    Byte *uncompr;
    uLong uncomprLen = 10000*sizeof(int); /* don't overflow on MSDOS */

    switch (ptr[1]&0xff) {
        case 0x80: 
    if (clients[k].stat == 10) { 
	    say(clients[k].fd,":%s!%s@czat.interia.pl JOIN :#%s\r\n",ptr+4,ptr+4,clients[k].kanal);
	    } 

	           break;
	case 0x81: 
//	         for (x=0; x<len; x++) fprintf(log,"0%x|",ptr[x]);
//			 fflush(0);
//			 fprintf(log,"\n");
		   x = strlen(ptr + 3);
		   
		   p = ptr + 3 + x + 1;
		   debug(log,"[PARSE-PRIV]: x: %d, len(%d)-3-x-1: %d \n",x,len,len-3-x-1);
		   if ( !(tmp=czat_parse_priv(p,len-3-x-1)) )
		      { debug(log,"[xalloc] null xalloc from czat_parse_priv\n"); return; }
        if (clients[k].stat == 10) 
             	   say(clients[k].fd,":%s!%s@czat.interia.pl PRIVMSG #%s :%s\r\n",ptr+3,ptr+3,clients[k].kanal,tmp);
		   xfree(tmp);
		   break;
        case 0x82: 
        if (clients[k].stat == 10) 
	    say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",ptr+2,ptr+2,clients[k].kanal,ptr+2);

		   break;
	case 0x83: /* topic */
//		   for (pp=0;pp<len;pp++) if (isprint(ptr[pp])) printf("%c:",ptr[pp]);
		   break;
	case 0x84: 
		   ptr+=4;
		   counter+=strlen(clients[k].nick);
		   for (pp=0;pp<len-4;pp++) { 
			if (ptr[pp]!=0x00) { 
				say(clients[k].fd,"%c",ptr[pp]);
			counter++;
			}
			    else
			   { 
			      pp+=2; 
			      say(clients[k].fd," ");
			      counter++;
			      if (counter>=400) 
			      {
				    say(clients[k].fd,"\r\n:%s 353 %s = #%s :",SERVER,clients[k].nick,clients[k].kanal);
				    counter=0;
				   }
			     }
			
			   }
	           fflush(0);
	           say(clients[k].fd,"\r\n:%s 366 %s #%s :End of NAMES list.\r\n",SERVER,clients[k].nick,clients[k].kanal);
    	           clients[k].stat = 10;
		   break;
	case 0x85:
		   x = strlen(ptr+3); // dlugosc ksywki
		   if ( (x+4) == len - 2) {
				if (clients[k].stat!=5) say(clients[k].fd,":admin!admin@oper.interia.pl PRIVMSG %s :%s Zamknal okno lub ignoruje twoje wiadomosci\r\n",clients[k].nick,ptr+3);
				}
		    else
		    {
//			printf("-> %s ",ptr + x + 1 + 3);

//			if ( !(tmp = czat_privmsg_crypt(ptr+3,ptr+x+1+3,len-x-3-2)) )
			if ( !(tmp = czat_privmsg_crypt(clients[k].nick,ptr+x+1+3,len-x-3-2)) )
			 { debug(log,"[xalloc] null xalloc parse ()->czat_privmsg_crypt\n"); return; }
			say(clients[k].fd,":%s!%s@czat.interia.pl PRIVMSG %s :%s\r\n",ptr+3,ptr+3,clients[k].nick,tmp);
		        free(tmp);			
		    }
	           break;	    
	
	case 0x86: 
//		   for (pp=0;pp<len;pp++) if (isprint(ptr[pp])) printf("%c:",ptr[pp]);
		   break;
	case 0x8b: 
	        uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
	        if (!uncompr) { debug(log,"[UNCOMPR]: Error przy dekompresju uncompr = null \n"); break; }
		decompress_inflate (ptr+2, len-3 , uncompr, uncomprLen);
//		for (x=0;x<uncompr[0];x++) 
//		    printf("%c",uncompr[x]);
//		    printf("\n");
                pre = ((uncompr[1]<<8)&0xffff) | (uncompr[0]&0xff);
		parse(uncompr+1,pre,k);
		xfree(uncompr);
	        break;
        case 0x96:
	 debug(log,"[ ERROR !!!] : %x -> ",ptr[2]&0xff);
	    switch (ptr[2]&0xff) {
		case 0x03:
		   	  debug(log,"No such channel! \n");
	                  say(clients[k].fd,"\r\n:%s 366 %s %s :End of NAMES list.\r\n",SERVER,clients[k].nick,clients[k].kanal);
 			  say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",clients[k].nick,clients[k].nick,clients[k].kanal,"nie istnieje");
                          say(clients[k].fd,"\r\n:%s 403 %s #%s :No such channel\r\n",SERVER,clients[k].nick,clients[k].kanal);
		          clients[k].stat=4;
		          close_conn(k); 			  
		          break;
		case 0x01:
	        case 0x08:
		   	  debug(log,"Nickname already in use or fucked up/too long! \n");
		  if (clients[k].stat == 5) {
		          say(clients[k].fd,"\r\n:%s 366 %s %s :End of NAMES list.\r\n",SERVER,clients[k].nick,clients[k].kanal);
			  say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",clients[k].nick,clients[k].nick,clients[k].kanal,"ksywka zajeta");
			  say(clients[k].fd,":%s 433 * %s :Nickname is already in use.\r\n",SERVER,clients[k].nick);
		          clients[k].stat = 4;
 
		  }
			 close_conn(k); 
	    		 break;
	       case 0x0a:
	            debug(log,"[JOIN-0A-ERR] You'r banned on %s\n",clients[k].kanal);
		    say(clients[k].fd,"\r\n:%s 366 %s %s :End of NAMES list.\r\n",SERVER,clients[k].nick,clients[k].kanal);
		    say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",clients[k].nick,clients[k].nick,clients[k].kanal,"jestes zabanowany");
		    say(clients[k].fd,":%s 474 %s #%s :Cannot join channel(+b)\r\n",SERVER,clients[k].nick,clients[k].kanal);		    
                    clients[k].stat = 4;
		    close_conn(k); 
		    break;
               case 0x05: debug(log,"[ERROR-5-FLOOD?]\n");
	    	   	  debug(log,"[?] Reconnecting after flood .... \n");
say(clients[k].fd,":admin!admin@oper.interia.pl PRIVMSG %s :%s Wyluzuj troche z tymi mesgami,wysylasz ich za duzo!\r\n",clients[k].nick,ptr+3);
			 // tu jeszcze nas nie rozlaczylo a ostrzeglo
/*		    if (clients[k].toczat>0) close(clients[k].toczat);
		    clients[k].port = 1;
		    ret = czat_join(&clients[k],clients[k].kanal,k,clients[k].czathost);
		    if ( ret < 1 ) {
	    	   	  debug(log,"[-] Reconnecting after flood failed \n");
	                  say(clients[k].fd,"\r\n:%s 366 %s %s :End of NAMES list.\r\n",SERVER,clients[k].nick,clients[k].kanal);
 			  say(clients[k].fd,":%s!%s@czat.interia.pl PART #%s :%s\r\n",clients[k].nick,clients[k].nick,clients[k].kanal,"nie istnieje");
                          say(clients[k].fd,"\r\n:%s 403 %s #%s :No such channel\r\n",SERVER,clients[k].nick,clients[k].kanal);
		          clients[k].stat=4;
			  close_conn(k);
	    		}*/

	    		  break;
	       default:  debug(log," -> Unknown Error! \n");
	    		 close_conn(k); 
	              /* nieznany blad */
	      }
	      //close_conn(k); 
	   break;
	default: 
	    //printf("Nieznany! ");

    }
}



void czat_to_client(int k) {
    char *buf;
    int b = clients[k].toczat;
    int ret;
    unsigned char len,multi;
    int pre,count;
    
    if (read(b,&len,1)<=0) {close_conn(k);return;}
    if (read(b,&multi,1)<=0) {close_conn(k);return;}

    pre = ((multi<<8)&0xffff) | (len&0xff);
    debug(log,"[xalloc] czat_to_client \n");
    if ( !(buf = xalloc( pre+1 )) )
        { debug(log,"[xalloc] null xalloc in czat_to_client()\n"); return; }
    buf[0]=multi;
    count = 0;
    while (count < pre - 2){
               if ( (ret=read(b,buf+1+count,pre - 2 - count))<=0) {close_conn(k);return;}
	       count+=ret;
   }
	   ret = count+2;
    debug(log,"[+ Read ]: %d \n",ret);
    parse(buf,ret,k);
    xfree(buf);
}






int main (void) {
    char netbuf[4048];
    
    int lsock,csock;
    int maxfds;
    struct sockaddr_in cin;
    struct timeval tv;
    int cin_size;
    int i,ret;
    fd_set fds;



    signal(SIGPIPE,finish);
    signal(SIGINT,finish);
    signal(SIGTERM,finish);
//    signal(SIGSEGV,finish);
    signal(SIGCHLD,finish);

//    bzero(interia,sizeof(interia));
//    strcpy(interia,INTERIA_CZAT);
    lsock = open_listen_socket(PORT);
    printf("[+] Listen sock opened ,port : %d \n",PORT);    

    /* Open log file */
    if (  (log = fopen(LOG,"a")) == NULL )
    {
	printf("Cant open log file : %s\n",strerror(errno));
	printf("Stopping....\n");
	return -1;
    }

//    if (daemon(0,0)<0) { perror("daemon"); finish(0); }

    while (1) {
	    FD_ZERO(&fds);
	    for (i=0; i < MAXCLIENTS; i++) {
		if (clients[i].stat) FD_SET(clients[i].fd, &fds);
		if (clients[i].stat>3) FD_SET(clients[i].toczat, &fds);
	    }
	    FD_SET(lsock, &fds);

	maxfds = getdtablesize();

        tv.tv_sec=4;
	if ( select(maxfds, &fds,NULL,NULL,&tv)<0 ) {
	    perror("[-] select ");
	    continue;
	}

	/* New connection arrived */

	if (FD_ISSET(lsock, &fds)) {
	    bzero(&cin, sizeof(cin));
	    cin_size = sizeof(cin);
	    csock = accept(lsock, (struct sockaddr *)&cin, &cin_size);
	    if (csock < 0) {
		perror("[-] Accept");continue;
	    }
	    if (count+1 == MAXCLIENTS) {
		    unavail:
		    debug(log,"[-] Maxclients (%d) reached \n",MAXCLIENTS);
		    close(csock);
		    continue;
		}
	    debug(log,"[+] Connection from %s accepted[%d],slots used: %d \n",inet_ntoa(cin.sin_addr),csock,count+1);
	    ret = find_free( &clients );
	    if (ret<0) goto unavail;
	    	    clients[ret].stat = 1;
		    clients[ret].fd = csock;
//		    dup(csock);

	            set_socket_keepalive(clients[ret].fd);

//		    clients[ret].fl = fdopen(clients[ret].fd,"r");
		    FD_SET(clients[ret].fd, &fds);
		    strcpy(clients[ret].czathost,INTERIA_CZAT);

		    count++;
	}
    
    /* sprawdzanie zdarzen*/

    for (i=0; i< MAXCLIENTS; i++) {
	if ( (clients[i].stat) && (FD_ISSET(clients[i].fd, &fds)) ) {
		bzero(netbuf,sizeof(netbuf));
//		ptr = fgets(netbuf, sizeof(netbuf)-1, clients[i].fl);
		ret = reads(&clients[i].fd,netbuf);
		if (ret<=0) 
			close_conn(i);
	    			 else {
	    		client_to_czat(netbuf,i);
			}
	}

	if ( (clients[i].stat>3) && (FD_ISSET(clients[i].toczat, &fds)) ) {
		czat_to_client(i);
		}
    }   // for 

    } // while(1)
}
