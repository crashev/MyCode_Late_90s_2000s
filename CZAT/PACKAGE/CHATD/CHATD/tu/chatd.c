//
// -+- To jest czesc java.chat projektu -+- VERSION 1.0(BETA)
//
// Coded By Crashkiller '2000 <pawq@blue.profex.com.pl>
//
// 
// Daemon java.chat() version 1.0(BETA)
//  + Daemon ten obsluguje cala kafejke,aplety,ktore sie z nim komunikuja
//    dostaje informacje oraz wysylaja informacje.Ich zadaniem jest
//    wizualizacja danych otrzymanych z daemona.
//
/*  
     Changelog:
      - procesuj-komendy() - dodanie nowej funkcji,ktora bedzie rozpoznawac
                     komendy.

      - xx.03.2003 - prace wznowione po 3 latach odlezenia sie kodu
      - 22.08.2000 - fixed little buffer overrun in host_lookup()
      - 22.08.2000 - dodany say(int,char *,..),oraz poprawiono blad w 
                     czyszczeniu odchodzacych uzytkownikow
      - 22.08.2000 - juz praktycznie mozna powiedziec ,ze spelnia to
                     co trzba,dodana obsluga czyszczaca useruf odlaczajacych
		     sie.
      - 19.08.2000 - Pierwsza wersja daemona(BETA),narazie prosta komunikacja
        miedzy osobami ,ktore sie podlacza :>
      
      TODO:
           - check_ramka() - wewnetrzny protokol 
           - wiele rzeczy :>,limity,optymalizacja,duzo innych...
	   - privmsg
	   - kicki,bany,admin kanalu
      
     
     'Kod Napisany Jak Najprzejzysciej jak tylko umialem :> '
     'This code was written as clean as I ever could write it.'
*/

#include "includy.h"
#include "define.h"

#include <errno.h>

#define ILOSC_MAX_PRZYCHODZACYCH_POLACZEN 3

/*

     MAIN PROCEDURE , THE SOUL OF DAEMON 
     
*/     
char *hlookup(unsigned long ip)
{
    char *p;
    struct hostent *hp;
    struct in_addr in;
    char buf[256];
    
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

    int port=1900;
    int pin;
    int cli[ILOSC_MAX_PRZYCHODZACYCH_POLACZEN];
    int kolejka=0; // kolejka
    int all;

int main() 
{
        struct sockaddr_in sckin,sckout;
        struct timeval tv; //timeout not implemeted yet.
	 
        char cmdbuf[1024];
        char buf[2000];
        int a,b,ret,len,ilu=0;
        fd_set fds;

	struct nicker
        { 
	    char nick[20]; 
	    char host[255];
	    int dom;
	};
	struct nicker nick[MAXUSR+1];

/* Resetujemy wszystkie zmienne na stosie */

    memset(buf,0x0,sizeof(buf));
    memset(cmdbuf,0x0,sizeof(cmdbuf));
    bzero(nick,sizeof(nick));


fprintf(stderr,"Starting chat daemon %s..\n",banner);

    pin=socket(PF_INET,SOCK_STREAM,0);
    if (!pin) 
       {
         syslog(LOG_NOTICE,"Socket open error()");
	 perror("Socket() ");
	 exit(-1);
       }
       
    a=1;setsockopt(pin,SOL_SOCKET,SO_REUSEADDR,&a,sizeof(a));

    if (fcntl(pin,F_SETFL,O_NONBLOCK)<0)
	    perror("Fcntl failed");
    
    
    sckin.sin_family=PF_INET;
    sckin.sin_port=htons(port);
    sckin.sin_addr.s_addr=INADDR_ANY;
            
    if (bind(pin,(struct sockaddr *)&sckin,sizeof(sckin))<0) 
       {
         syslog(LOG_NOTICE,"Socket bind error()");
         perror("Bind() ");
	 exit(-1);
       }


    listen(pin,2);

// forkujemy i odzielamy sie od konsoli ,zamykajac stdin,stderr,stdout 
/*
    if (fork()) exit(0);
    if (setsid()&&fork()) exit(0);
//    for (a=0;a<3;a++) {close(a);}    
*/    

    
	FD_ZERO(&fds);
	FD_SET(pin,&fds);

	ilu=-1;kolejka=0;
	ilu=0;
while (1) 
 {
 
        tv.tv_sec=60;
	ret=select(255,&fds,NULL,NULL,&tv);
	len=sizeof(sckout);

/* Obsluga nowego polaczenia - przyjmowanie i przygotowywanie do czytania */
if (FD_ISSET(pin,&fds)) 
{
//	    ilu++;
	    bzero(cmdbuf,sizeof(cmdbuf));
	    bzero(buf,sizeof(buf));
	    bzero(&sckout,sizeof(sckout));
	    cli[kolejka]=accept(pin,(struct sockaddr *)&sckout,&len);
            if (cli<0) 
	    {
		    perror("accept");
		    goto err;
	    }

	    strncpy(cmdbuf,(char *)hlookup(sckout.sin_addr.s_addr),sizeof(cmdbuf)-1);
            syslog(LOG_NOTICE,"Connection from : %s \n",cmdbuf);
    if (ilu+1>=MAXUSR)
    {
	    say(cli[kolejka],"Sorry, max connection to the chat server has been reached.\nTry later\n\n");
	    say(cli[kolejka],"Przykro nam ale maksymalna ilosc polaczen zostala juz wykorzystana\nSproboj pozniej\n\n");
	    sleep(1);	    
            shutdown(cli[kolejka],2);
//	    sleep(2);	   
//	    ilu--; 
	    continue;
    }
    if ( kolejka == ILOSC_MAX_PRZYCHODZACYCH_POLACZEN-1 )
    {
	    say(cli[kolejka],"Sorry CHATD is busy now ,try again later!\n\n");

	    sleep(1);	    
            shutdown(cli[kolejka],2);
	    goto err;
    }
        if (fcntl(cli[kolejka],F_SETFL,O_NONBLOCK)<0)
	    perror("Fcntl-CLI failed");
//	    FD_SET(cli[kolejka],&fds);
	    say(cli[kolejka],"Yow!\n"); // request for ramka :>
	    kolejka++;


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

	    ret=recv(cli[a],buf,1000,0);
	    if (ret<=0)
	    {
		perror("recv()");
		printf("errno: %d (%s) \n",errno,strerror(errno));
		printf("[%d] Connection closed or refused \n",kolejka);
//	        ilu--;
                kolejka--;
		goto err;
            }
    if (!check_ramka(buf)) 
	{ 
	    say(cli[a],"Error in ramka\n");
	    shutdown(cli[a],2);
	    goto err;
//	    ilu--;
//	    continue;
	} 
	else 
	{ 
	    say(cli[a],"accepted ramka\n");
	    printf("[ILU]: %d \n",ilu);
	    strncpy(nick[ilu].host,cmdbuf,sizeof(nick[ilu].host)-1);
            strncpy(nick[ilu].nick,buf,sizeof(nick[ilu].nick)-1);
	    chop(nick[ilu].nick);
	    nick[ilu].dom=dup(cli[a]);
	    say(nick[ilu].dom,"users on line : \n\n");
        for (b=0;b<=ilu;b++) 
	   {
	    say(nick[ilu].dom,"%d) Nick : %s ,Host: %s ,Dom: %d\n",b,nick[b].nick,nick[b].host,nick[b].dom);
	   }	
	   memset(buf,0x0,sizeof(buf));
           b=0;while (b<ilu) { say(nick[b].dom,"%s has joined into cafe\n",nick[ilu].nick);b++;}
	   say(all,"%s has joined now into caffe by gate 1.0\n",nick[ilu].nick);
            ilu++;           

//    FD_CLR(cli[a],&fds);
       for (b=0;b<kolejka;b++) 
       {
            cli[a+b]=dup(cli[a+b+1]);
       }

         kolejka--;
	 printf("[Kolejka] : %d \n",kolejka);


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
           ret=recv(nick[a].dom,cmdbuf,1000,0);
    if ((strlen(cmdbuf)>0)&&(ret>0)&&(cmdbuf[0]!='\n')&&(cmdbuf[0]!='\r'))
	{
       chop(cmdbuf);
       b=0;while (b<ilu) { say(nick[b].dom,"<%s> %s\n",nick[a].nick,cmdbuf);b++;}
	} else 
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

	 strncpy(cmdbuf,nick[ilu].nick,sizeof(cmdbuf)-1);
         FD_CLR(nick[a].dom,&fds);
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
           nick[a+b].dom=dup(nick[a+b+1].dom);
       }
}
      b=0;while (b<=ilu) { say(nick[b].dom,"%s has left cafe\n",cmdbuf);b++;}	 
if (ilu<0) break;
    }
}

a++;
}
err:
a=0;
 
    FD_ZERO(&fds);
    FD_SET(pin,&fds);
if ((ilu<0)&&(kolejka==0)) continue;    
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