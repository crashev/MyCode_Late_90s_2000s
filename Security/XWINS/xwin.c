// ------------------------------------------------------
// XSecurity Sniffer Version 1.0 BETA
// 
// Coded By -=  Crashiller ( pawq@kki.net.pl ) '99
//                         ( crashev@sys.com.pl )
//                         ( pawq@blue.profex.com.pl )
//
// Raport Any Bugs to      : pawq@kki.net.pl
// ----------------------------------------------------- 
// Note :  
// 
// 
// 
// 
//
//
#define IPHDRSIZE   sizeof(struct iphdr  )
#define TCPHDRSIZE  sizeof(struct tcphdr )

// Includez
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <net/tcp.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "netinet/ip.h"
#include <netinet/udp.h>

// Sylog daemon
#include <syslog.h>
// syslog(LOG_DAEMON|LOG_NOTICE,"Informacja");

     char bufor[1024];
char *checkuser(char *hostek,int port) {

     struct sockaddr_in sckddr;
     struct hostent *h;
     char cmd[1024];
     char host[1024];
     int i=0,a=0,c=0;
     int pipe;
     long inet;

strcpy(host,hostek);
    inet = inet_addr(host);
if (inet==-1) 
    {
       if (h = gethostbyname(host)) memcpy(&inet,h->h_addr,4); else inet=-1;
    }
if (inet==-1) 
     {
     fprintf(stderr,"Cant resolv : %s \n",host);
     }

    sckddr.sin_family=AF_INET;
    sckddr.sin_addr.s_addr=inet;
    sckddr.sin_port=htons(113);
    bzero(bufor,sizeof(bufor));	

if ((pipe=socket(AF_INET,SOCK_STREAM,0))<0) perror("Error opening a socket! ");
if (connect(pipe,(struct sockaddr_in *)&sckddr, sizeof(sckddr)) < 0) 
    {
//      perror("Ident Daemon Not Found On Server ");
      bzero(bufor,sizeof(bufor));strcpy(bufor,"NO-USER");close(pipe);return bufor;
    }

    sprintf(cmd,"%d, 6000\n",port);
    write(pipe,cmd,strlen(cmd));
    bzero(cmd,sizeof(cmd));
    read(pipe,cmd,1023);
for (i=0;i<=strlen(cmd);i++) 
    {
        if (cmd[i]==':') { a++; if (a==3) i++; }
        if (a==3) bufor[c++]=cmd[i];
    }
if (a==2) 
    { 
        bzero(bufor,sizeof(bufor));strcpy(bufor,"NO-USER");close(pipe);return bufor; 
    }
bzero(host,sizeof(host));
for (i=0;i<=strlen(bufor)-3;i++) { host[i]=bufor[i]; }
    bzero(bufor,sizeof(bufor));
    strcpy(bufor,host);
close(pipe);
	return bufor;
}

char *hostlookup(unsigned long in,int stat)
{
	static char buforek[1024];
	struct hostent * he;
	struct in_addr i;
        bzero(buforek,sizeof(buforek));	
	i.s_addr = in;
	he = gethostbyaddr((char *)&in, sizeof(in), AF_INET);
	if (he)
		strcpy(buforek, he->h_name);
	else 	strcpy(buforek, inet_ntoa(i));

if (stat==0) 
    { 
	strcpy(buforek, inet_ntoa(i));
        return buforek; } 
	else {
       return buforek; 
       }
}



void main(int argc,char *argv[]) {
int linked=0;
unsigned long source;
unsigned long destation;
char dns[255];
char dns_ip[255];
char bufor[2000];
char user[50];
int on=1,i;

struct sockaddr_in sin;
int len,resu;
int sraw,rraw;
struct tcphdr *tcp;
struct iphdr *ip;
unsigned char packdat[400];
unsigned long seq;
int n=1;
// Jeden socket do odbierania
    if ( (rraw=socket(AF_INET ,SOCK_RAW ,6)) <0 ) 
	 {
         fprintf(stderr,"Cant Open Raw Socket !\n");
         exit(-1);
	 }

ip  = (struct iphdr  *)(packdat);
tcp = (struct tcphdr *)(packdat+IPHDRSIZE);

memset(packdat,0,399);
/* Starting Daemon Working */
  syslog(LOG_DAEMON|LOG_NOTICE,"Starting XWindow Security Sniffer Daemon V1.0");
// going background
if (fork()) exit(0);
 while (1) 
    {
    len=sizeof(sin);
    resu=recvfrom(rraw,packdat,398,0,(struct sockaddr *)&sin,&len);
// Someone trying connect to port 6000 ??
if (tcp->th_dport == htons(6000)) {
          if(tcp->th_flags & TH_SYN) {
             if(tcp->th_win == htons(512)) {
// Port Opened?
for (i=0;i<2;i++) {
           resu=recvfrom(rraw,packdat,398,0,(struct sockaddr *)&sin,&len);
               if (tcp->th_flags & TH_ACK) {                           
                   if(tcp->th_win == htons(31896)) {
/* We got connection so now we have to LOG IT */
  strcpy(dns,hostlookup(ip->saddr,1));
  strcpy(dns_ip,hostlookup(ip->saddr,0));
  strcpy(user,checkuser(dns_ip,htons(tcp->th_sport)));
  sprintf(bufor,"[Connected] : %s ( %s ) \n",dns,dns_ip);
  syslog(LOG_NOTICE,bufor);bzero(bufor,sizeof(bufor));
  sprintf(bufor,"Connected [user-info] : %s@%s",user,dns_ip);
  syslog(LOG_DAEMON|LOG_NOTICE,bufor);bzero(bufor,sizeof(bufor));
  bzero(packdat,sizeof(packdat));linked++;
}}} }}}
if (tcp->th_dport == htons(6000)) {
          if(tcp->th_flags & TH_FIN) {
             if(tcp->th_win == htons(31896)) {
  if (linked>0) {
  sprintf(bufor,"Closing connection [%d]: %s\n",linked,hostlookup(ip->saddr,0));
  syslog(LOG_DAEMON|LOG_NOTICE,bufor);bzero(bufor,sizeof(bufor));
  linked--;
}}}}
}
}
