//
// -+- To jest czesc java.chat projektu -+- VERSION 1.0(BETA)
//
//	Coded by Pawel Furtak <crashev@ebox.pl> 2000-2003
// 
//    Daemon java.chat() version 1.0(BETA)
//  + Daemon ten obsluguje cala kafejke,aplety,ktore sie z nim komunikuja
//    dostaje informacje oraz wysylaja informacje.Ich zadaniem jest
//    wizualizacja danych otrzymanych z daemona.
//
/*  
     Changelog:

      - xx.03.2003 - ....
      - 22.08.2000 - fixed little buffer overrun in host_lookup()
      - 22.08.2000 - dodany say(int,char *,..),oraz poprawiono blad w 
                     czyszczeniu odchodzacych uzytkownikow
      - 22.08.2000 - juz praktycznie mozna powiedziec ,ze spelnia to
                     co trzba,dodana obsluga czyszczaca useruf odlaczajacych
		     sie.
      - 19.08.2000 - Pierwsza wersja daemona(BETA),narazie prosta komunikacja
        miedzy osobami ,ktore sie podlacza :>
      
     
     'Kod Napisany Jak Najprzejzysciej jak tylko umialem :> '
     'This code was written as clean as I ever could write it.'
*/

#include "includy.h"
#include "define.h"


#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>

#define ILOSC_MAX_PRZYCHODZACYCH_POLACZEN 3
extern void debug(FILE *gdzie,char *fmt,...);

char *hlookup(unsigned long ip)
{
    char *p;
    struct hostent *hp;
    struct in_addr in;
    char buf[256];

//    FILE *nowyread;
        
    memset(buf,0x0,sizeof(buf));
    in.s_addr=ip;    
    hp=gethostbyaddr((char *)&ip,sizeof(ip),PF_INET);
    
    if (hp) 
         strncpy(buf,hp->h_name,254);
     else
         strncpy(buf,inet_ntoa(in),254);
    p=buf;
    return p; 
     
}

//    extern char *hlookup(unsigned long ip);
    extern void say(int gdzie,char *fmt,...);
    extern int check_ramka(char *o);
    extern void chop(char *co); 

    int port=1902;
    int pin;
    int cli[ILOSC_MAX_PRZYCHODZACYCH_POLACZEN];
    int kolejka=0; // kolejka
    int all;

/*

     MAIN PROCEDURE , THE SOUL OF DAEMON 
     
*/     


int main() 
{
	struct stat tmppp;    
        struct sockaddr_in sckin,sckout;
        struct timeval tv; //timeout not implemeted yet.
	 
        char cmdbuf[1024];
	char arg[100];
	
        char buf[2000];
        int a,b,ret,len,ilu=0,last=255;
	int found=0;
        fd_set fds;
	
	FILE *outlog;

	struct nicker
        { 
	    char nick[20]; 
	    char host[255];
	    char kanal[20];
	    int dom;
	    
	};
	struct nicker nick[MAXUSR+1];
    
	char *tmpnick;
	char *tmpchann;
        signal(SIGPIPE,SIG_IGN);


	/* Resetujemy wszystkie zmienne na stosie */

    if ( (outlog=fopen("logs/debug.log","a+"))==NULL)
    {
	printf("Coulnd open/create debug file\n");
	return -1;
    }
    
    memset(buf,0x0,sizeof(buf));
    memset(cmdbuf,0x0,sizeof(cmdbuf));
    bzero(nick,sizeof(nick));


    debug(outlog,"??? Starting chat daemon %s..\n",banner);

    pin=socket(PF_INET,SOCK_STREAM,0);
    if (!pin) 
       {
         debug(outlog,"!!! Couldnt create socket %s\n",strerror(errno));
	 exit(-1);
       }
       
    a=1;setsockopt(pin,SOL_SOCKET,SO_REUSEADDR,&a,sizeof(a));

    if (fcntl(pin,F_SETFL,O_NONBLOCK)<0)
    {
        debug(outlog,"!!! Couldnt set up non block option %s\n",strerror(errno));
	exit(-1);
    }
    
    sckin.sin_family=PF_INET;
    sckin.sin_port=htons(port);
    sckin.sin_addr.s_addr=INADDR_ANY;
            
    if (bind(pin,(struct sockaddr *)&sckin,sizeof(sckin))<0) 
       {
	debug(outlog,"!!! Couldnt bind socket %s\n",strerror(errno));
	 exit(-1);
       }


    listen(pin,2);

// forkujemy i odzielamy sie od konsoli ,zamykajac stdin,stderr,stdout 

    if (fork()) exit(0);
    if (setsid()&&fork()) exit(0);
    for (a=0;a<3;a++) {close(a);}    
    

    
	FD_ZERO(&fds);
	FD_SET(pin,&fds);

	ilu=-1;kolejka=0;
	ilu=0;
while (1) 
 {
 
        tv.tv_sec=10;

//	debug(outlog,"??? LAST = %d \n",last);
	ret=select(last+2,&fds,NULL,NULL,&tv);
	len=sizeof(sckout);

/* Obsluga nowego polaczenia - przyjmowanie i przygotowywanie do czytania */
if (FD_ISSET(pin,&fds)) 
{

		    bzero(cmdbuf, sizeof(cmdbuf));
		    bzero(buf, sizeof(buf));
		    bzero(&sckout, sizeof(sckout));
	
	    cli[kolejka]=accept(pin, (struct sockaddr *)&sckout, &len);
            if (cli<0) 
	    {
		    debug(outlog,"!!! Accept error %s\n",strerror(errno));
		    goto err;
	    }

	    strncpy(cmdbuf, (char *)hlookup(sckout.sin_addr.s_addr), sizeof(cmdbuf)-1);
            debug(outlog,"+++ Connection from %s\n",inet_ntoa(sckout.sin_addr));
	    //syslog(LOG_NOTICE, "Connection from : %s \n", cmdbuf);

    if (ilu+1>=MAXUSR)
    {
 	    say(cli[kolejka],"Sorry, max connection to the chat server has been reached.\nTry later\n\n");
	    say(cli[kolejka],"Przykro nam ale maksymalna ilosc polaczen zostala juz wykorzystana\nSproboj pozniej\n\n");
//            shutdown(cli[kolejka],2);
	    close(cli[kolejka]);	
	    debug(outlog,"??? Max connections reached (ilu+1) %d\n",ilu+1);
//	    kolejka--;
            goto err;
    }
    if ( kolejka == ILOSC_MAX_PRZYCHODZACYCH_POLACZEN-1 )
    {
	    say(cli[kolejka],"Sorry CHATD is busy now ,try again later!\n\n");
	    debug(outlog,"??? Chatd is busy now <kolejka> %d\n",kolejka);
            //shutdown(cli[kolejka],2);
	    close(cli[kolejka]);
//	    goto err;
    }
            if (fcntl(cli[kolejka],F_SETFL,O_NONBLOCK)<0)
	    {
	    	    debug(outlog,"!!! Coulnd set up nonblock socket for incoming connection\n");
		    close(cli[kolejka]);
		    
	    } else {
		debug(outlog,"??? Set up nonblock socket for incoming connection sucesfull\n");
	        if (cli[kolejka]>last) last=cli[kolejka];
	        say(cli[kolejka],"*YWitamy na czacie!\n"); // request for ramka :>
		debug(outlog,"??? Przydzielony fd dla accept : %d \n",cli[kolejka]);
	        kolejka++;
	    }
//	    goto err;

}

/* a tutaj jest juz obsluga nowych polaczen - czytanie ramki/sprawdanie czy nie
   rozlaczyl sie ktos */
a=0;
while (a<kolejka)
{
    if (FD_ISSET(cli[a],&fds))
    {
    /* 
    
	Tutaj obsluga protokolu :
	    1) Wysylamy prosbe o potwierdzenie biletu.
	    2) Sprawdzamy czy dane protokolu od klienta sie zgadzaja
    
     */
            bzero(buf,sizeof(buf));
	    ret=recv(cli[a],buf,sizeof(buf)-1,0);
	    if (ret<=0)
	    {
	        debug(outlog,"!!! [%d] Connection refused %s\n",kolejka,strerror(errno));
		close(cli[a]);
	//       ilu--;
                kolejka--;
		goto err;
            }
    debug(outlog,"??? Recived ramka buf [FD: %d] : %s \n",cli[a],buf);
    if (!check_ramka(buf)) 
	{ 
	    say(cli[a],"Wprowadzona przez ciebie ksywka jest niepoprawna, zmien ja i sprobuj ponownie.W przypadku problemow zajrzyj do dzialu pomocy\n");
            debug(outlog,"!!! Klient wprowadzil niepoprawna ksywke: %s\n",buf);	
//	    shutdown(cli[a],2);
            close(cli[a]);
	    kolejka--;
	    goto err;
	} 
	else 
	{ 
        chop(buf);

	tmpnick = strtok(buf,":");
	tmpchann = strtok(NULL,":");

	if ((!tmpnick)|| (!tmpchann)) {
	     	    say(cli[a],"--- PROTOCOL BROKEN!\n");
		    close(cli[a]);
		    kolejka--;
		    goto err;
        }


        if ( (tmpchann[0]!='#')||(strlen(tmpchann)>15) ) {
		    close(cli[a]);
		    kolejka--;
		    goto err;
        }
	
	
        for (b=0;b<=ilu;b++) 
	   {
	     
	     if (!strcasecmp(nick[b].nick,tmpnick))
	     {
	     	    say(cli[a],"--- Ktos juz uzywa wybranej przez ciebie KSYWKI !!!\n");
                   debug(outlog,"--- Ktos uzywa wybranej ksywki %s\n",buf);
		    //shutdown(cli[a],2);
		    close(cli[a]);
		    kolejka--;
		    goto err;

	     }
	   }	

//	    say(cli[a],"accepted ramka\n");
//	    printf("[ILU]: %d \n",ilu);
	    strncpy(nick[ilu].host,cmdbuf,sizeof(nick[ilu].host)-1);
            strncpy(nick[ilu].nick,tmpnick,sizeof(nick[ilu].nick)-1);
            strncpy(nick[ilu].kanal,tmpchann,sizeof(nick[ilu].kanal)-1);
	    chop(nick[ilu].nick);
	    nick[ilu].dom=cli[a];
	    nick[ilu].dom=dup(cli[a]);
	    close(cli[a]);
//	    say(nick[ilu].dom,"users on line : \n\n");
        for (b=0;b<=ilu;b++) 
	   {
	      if (!strcmp(nick[ilu].kanal,nick[b].kanal) ) say(nick[ilu].dom,"=addlist:%s:%d\n",nick[b].nick,nick[b].dom);
	   }	
	   memset(buf,0x0,sizeof(buf));
           b=0;
	   while (b<ilu) 
	    { 
		if (!strcmp(nick[ilu].kanal,nick[b].kanal) ) say(nick[b].dom,"=join:%s:%d\n",nick[ilu].nick,nick[ilu].dom);
		b++;
	    }
           debug(outlog,"??? [%s] has joined %s\n",nick[ilu].nick,nick[ilu].kanal);
           ilu++;           
		    //    FD_CLR(cli[a],&fds);
       for (b=0;b<kolejka;b++) 
       {
//            cli[a+b]=dup(cli[a+b+1]);
            cli[a+b]=cli[a+b+1];
       }

         kolejka--;
         debug(outlog,"??? Stan kolejki [%d] \n",kolejka);
         }
     }
a++;
}               


/* sprwadzamy zdarzenia */
    a=0;
while ( a <  ilu) 
{
    if (FD_ISSET(nick[a].dom,&fds)) 
    {
           
	   memset(cmdbuf,0x0,sizeof(cmdbuf));
//           if (nowyread) fclose(nowyread);
//	   nowyread=fdopen(nick[a].dom,"r");
	   ret=recv(nick[a].dom,cmdbuf,sizeof(cmdbuf)-1,0);
           
    if ((strlen(cmdbuf)>0)&&(ret>0)&&(cmdbuf[0]!='\n')&&(cmdbuf[0]!='\r'))
	{
            chop(cmdbuf);	// obcinamy zbedne znaki na koncu

    /* rozaptrujemy bufor */

if (cmdbuf[0]=='+')
    if ( !strcmp(cmdbuf, "+QUIT") ) { 
	    shutdown(nick[a].dom, 2);
	    ret=-1;
	}
	else if ( !strncmp(cmdbuf, "+SEND",5)&&(strlen(cmdbuf)>6) ) 
	{ 
	   char znacznik;
	   bzero(arg,sizeof(arg));
	   // +SEND:NORMAL=tekst
	   // +SEND:BOLDDD=tekst
	   // +SEND:ITALIC=tekst
	   // +SEND:BOLITA=tekst
	    
	   // 01234567890123456789
	   // +SEND:BOLITA:C=tekst
		   /* Some protocol checking */
		   if ((cmdbuf[5]!=':')||(cmdbuf[14]!='=')||(cmdbuf[12]!=':') ) break;
		   
	   strncpy(arg,cmdbuf+6,6);
	   znacznik='*';
	   if (!strcmp(arg,"NORMAL")) znacznik='*'; // normal
	   if (!strcmp(arg,"BOLDDD")) znacznik='&'; // bold
	   if (!strcmp(arg,"ITALIC")) znacznik='$'; // italic
           if (!strcmp(arg,"BOLITA")) znacznik='#'; // both
	
	   
	   
	/* Wysylanie informacji wazna rzecz ,3ba obsluzyc
	   style BOLD/ITALIC 
	 */    
	    b=0;
	    while (b<ilu) { 
		if (!strcmp(nick[a].kanal,nick[b].kanal) ) say(nick[b].dom,"%c%c<%s> %s\n", znacznik,cmdbuf[13],nick[a].nick, cmdbuf+15);
	        b++;
	    
	    }
	    
        } else if ( !strncmp(cmdbuf, "+USERINFO",9) && (strlen(cmdbuf)>10))
	{
	       found=0;
	       strncpy(arg,cmdbuf+10,sizeof(arg)-1);
	       for (b=0; b < ilu; b++) if (!strcmp(nick[b].nick, arg )) 
	       {
	    		    say(nick[a].dom,"=USERINFO:%s:%s\n",nick[b].nick,nick[b].host);
			    found=1;
			    break;
	       }
	       if (found==0) say(nick[a].dom,"=USERINFO:Uzytkownik %s:nie zostal znaleziony\n",arg);
	} else if ( !strncmp(cmdbuf, "+PRIVMSG",8) && (strlen(cmdbuf)>9))
	{
	    int fdd;
	            debug(outlog,"??? PRIVMSG %s\n",cmdbuf);
		    bzero(arg,sizeof(arg));
	            sscanf(cmdbuf,"+PRIVMSG %d :",&fdd);
		    if (!strchr(cmdbuf,':')) break;
		    strncpy(arg,strchr(cmdbuf,':')+1,sizeof(arg)-1);
		    debug(outlog,"??? PRIVMSG do %d , WIADOMOSC: %s \n",fdd,arg);

		    if ( (fstat(fdd,&tmppp)<0)  || (fstat(nick[a].dom,&tmppp)<0)) {
			    debug(outlog,"!!! fstat < 0 !!! - broken pipe\n");
			    break;
		    }
		    
    debug(outlog,"??? DO czata leci: =PRIVMSG:%s:%c%c<%s>%s\n",nick[a].nick,
				    arg[0],arg[1],nick[a].nick,arg+2);
		    say(fdd,"=PRIVMSG:%s:%c%c<%s>%s\n",nick[a].nick,
				    arg[0],arg[1],nick[a].nick,arg+2);
//		    say(nick[a].dom,"=PRIVMSG:%s:%s\n",nick[a].nick,arg);
	}
    }
	 
    if (ret <= 0) 
    {
    /*
       Jakby ktos kiedys bedzie badal to source i sie zgubil to :
       
       poprawiona procedura odchodzacy(),po zejsciu z kanalu
       nastepuje reorganizacja tak aby elementy byly umieszczone
       od zera ,to znaczy ,jezeli odejdzie nick[2],w to miejsce 3ba prz-
       estawic innych 
       nick1 ,nick2, ...,nick3
       
      */
      
         /* Najpierw czyscimy odchodzacego z kanalu */

	 strncpy(cmdbuf,nick[a].nick,sizeof(cmdbuf)-1);

        b=0;
        while (b<ilu) 
        { 
	    	if (!strcmp(nick[a].kanal,nick[b].kanal) ) say(nick[b].dom,"=part:%s:00\n",cmdbuf);
	    b++;
	}	 

         debug(outlog,"??? Zamykamy fd : %d \n",nick[a].dom);
	 close(nick[a].dom);
         FD_CLR(nick[a].dom,&fds);
//	 close(nick[a].dom);
         memset(nick[a].host,0x0,sizeof(nick[a].host));
	 memset(nick[a].nick,0x0,sizeof(nick[a].nick));
	 nick[a].dom=0;
	 
       /* 
	   wolna struktorka o zmiennej a ,nastepuje reogranizacja 
           moze to goopi algorytm ,ale dziala ;)
       */


	ilu--;
if (ilu>=0) 
{
       for (b=0;b<=ilu;b++) 
       {
           strcpy(nick[a+b].nick,nick[a+b+1].nick);
           strcpy(nick[a+b].host,nick[a+b+1].host);
//           nick[a+b].dom=dup(nick[a+b+1].dom);
           nick[a+b].dom=nick[a+b+1].dom;
       }
}
            debug(outlog,"??? [%s] parting \n",cmdbuf);
	if (ilu<0) break;
    }
}

a++;
}
err:
a=0;
 
    FD_ZERO(&fds);
    FD_SET(pin,&fds);

//  if ((ilu<0)&&(kolejka==0)) continue;    
    while (a< ilu) 
    {
        FD_SET(nick[a].dom,&fds);
        a++;
    }
    a=0;
    while (a < kolejka)
    {
    	    FD_SET(cli[a],&fds);
	    a++;
    }

 }
}
/* EOF HERE */