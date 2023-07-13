// crashev '2001
// gcc -O3 -Wall tunel.c -o tunel;strip tunel
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include <string.h>

#define MYPORT 3341
#define DEFHOST "pp.pp.pp.pp"
#define DEFPORT 3341


int check_regula(struct sockaddr_in *tu) 
{
     FILE *conf;
     char *p;
     char buf[1024];
     char addr[255];
     int a1,a2,a3,a4;
     int d1,d2,d3,d4;
       
     memset(buf,0x0,sizeof(buf));
     memset(addr,0x0,sizeof(addr));
     strncpy(addr,inet_ntoa(tu->sin_addr),sizeof(addr)-1);
         p=strtok(addr,".");a1=atoi(p);
	 p=strtok(NULL,".");a2=atoi(p);
         p=strtok(NULL,".");a3=atoi(p);
         p=strtok(NULL,".");a4=atoi(p);

     conf=fopen("hosts.allow","r");
     if (conf==NULL) 
        {
    	    
//	    perror("Cant open config file hosts.allow ");
            return 1;
	}    
   while (!feof(conf)) 
      {
        memset(buf,0x0,sizeof(buf));
        fgets(buf,sizeof(buf)-1,conf);
	if (strlen(buf)<8) return 0;
        p=strtok(buf ,".");d1=atoi(p);
        p=strtok(NULL,".");d2=atoi(p);
        p=strtok(NULL,".");if (!strncmp(p,"*",1)) d3=-99; else d3=atoi(p);
        p=strtok(NULL,".");if (p!=NULL) {if (!strncmp(p,"*",1)) d4=-99; else d4=atoi(p);} else { d4=-99;}
        if ((d1==0)&&(d2==0)) return -2; 
	if ((d1>255)||(d2>255)||(d3>255)||(d4>255)) return -2;
        if ((a1==d1)&&(a2==d2)&& ((a3==d3)||(d3==-99)) && ((a4==d4)||(d4==-99)) ) 
	    {
	        fclose(conf); 
		return -10;
	    } 
      }
fclose(conf);
return 0;	
}

int main(int argc, char **argv)
{
 struct sockaddr_in sin;
 struct hostent *phe,*h;
 long ia;
 int tos;
 int lis, cli, ser;
 fd_set f1, f2;
 char buf[8192];
 int len, v = 1,ret;
if (argc<=3) 
{
    fprintf(stderr, "Usage: %s [<host> <port>] [<myport>] [<myhost>]\n", argv[0]);
    exit(-1);
 }
 memset((void *)&sin, 0, sizeof(sin));
 sin.sin_family = AF_INET;
 sin.sin_port = htons(argc > 3 ? atoi(argv[3]) : MYPORT);
 if(((lis = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)) {
  fprintf(stderr, "Cannot get free socket\n");
  return -1;
 }
 setsockopt(lis, SOL_SOCKET, SO_REUSEADDR, (void *) &v, sizeof(v));

if (argc>4) 
{
        fprintf(stderr,"Uzywamy hosta: %s\n",argv[4]);
        ia=inet_addr(argv[4]); 
  if (ia==-1) 
	{
    	    if((h=gethostbyname(argv[4]))) memcpy(&ia,h->h_addr,4);
	                                        else ia=-1;
	}
        if (ia==-1)
        {
	    fprintf(stderr,"cannot resolve %s \n",argv[4]);
            return -1;
	}
 }
 if(bind(lis, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
 {
  fprintf(stderr, "Cannot bind socket\n");
  return -1;
 }
 
 if(listen(lis, 1) < 0) {
  fprintf(stderr, "Cannot listen\n");
  return -1;
 }
 signal(SIGHUP, SIG_IGN);
 signal(SIGPIPE, SIG_IGN);
 signal(SIGCHLD, SIG_IGN);
 fprintf(stderr, "Ready to tunel from port %d\n", argc > 3 ? atoi(argv[3]) : MYPORT);

 while((len = sizeof(sin)) && (cli = accept(lis, (struct sockaddr *)&sin, &len)) >= 0) {
  if(!fork()) {
   close(lis);

   // Access Host - crashev
   ret=check_regula(&sin);
   if (ret==-10) 
    {
    } else if (ret<0)
    {
//	printf("Acces Check() internal error!!: %d\n",ret);
	perror("access ()");
    } else if (ret==0) {
//        printf("Access denied for host : %s \n",inet_ntoa(sin.sin_addr));
	write(cli,"Fuck you!\n",10);
	goto denied;
}        
   //
   if((ser = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    tos=64;setsockopt(ser, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
    close(cli);
    exit(0);
   }
if (argc>4) { 
 
 sin.sin_family = AF_INET;
 sin.sin_addr.s_addr=ia;

 if(bind(ser, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
  fprintf(stderr, "Cannot bind socket\n");
  return -1;
 }
 }
   memset((void *)&sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   sin.sin_port = htons(argc > 2 ? atoi(argv[2]) : DEFPORT);
 if(!(phe = gethostbyname(argc > 2 ? argv[1] : DEFHOST))) {
  fprintf(stderr, "Cannot find host\n");
  return -1;
 }
   memcpy((void *)&sin.sin_addr, phe -> h_addr, phe -> h_length);

   if(connect(ser, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    close(cli);
    exit(0);
   }
   FD_ZERO(&f1);
   FD_SET(cli, &f1);
   FD_SET(ser, &f1);
   while(1) {
    memcpy((void *)&f2, (void *)&f1, sizeof(f1));
    select(getdtablesize(), &f2, NULL, NULL, NULL);
    if(FD_ISSET(cli, &f2)) {
     if(!(len = read(cli, buf, 8192))) exit(0);
     write(ser, buf, len);
     if (len<0) exit(0);
    }
    if(FD_ISSET(ser, &f2)) {
     if(!(len = read(ser, buf, 8192))) exit(0);
     write(cli, buf, len);
     if (len<0) exit(0);
    }
   }
   
   close(cli);
   close(ser);
   exit(0);
  }
  waitpid(-1,NULL,WNOHANG);
  denied:
  close(cli);
 }
 return 0;
}
