/*
 Program do badania protokolu
 Interia Czat
 by Pawel Furtak '2003 - All rights reserved
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <zlib.h>


char *
check_err (int err)
{
  static char showe[100];
  switch (err)
    {
    case -1:
      strcpy (showe, "ERRNO");
      break;
    case -2:
      strcpy (showe, "STREAM ERROR");
      break;
    case -3:
      strcpy (showe, "DATA ERROR");
      break;
    case -4:
      strcpy (showe, "MEM ERROR");
      break;
    case -5:
      strcpy (showe, "BUF ERROR");
      break;
    case -6:
      strcpy (showe, "VERSIOn ERROR");
      break;
    default:
      strcpy (showe, "OK");
      break;
    }
  return (char *) showe;
}


void *xalloc(size_t size ) {
    void *p=malloc(size);

    if (!p) { 
	    printf("Malloc ERROR! \n");
	    exit(-1);
	}

    memset(p,0x0,size);
    return p;
}

void xfree(void *ptr) {
    if (ptr) 
	   free(ptr);
}

int
decompress_inflate (compr, comprLen, uncompr, uncomprLen)
     Byte *compr, *uncompr;
     uLong comprLen, uncomprLen;
{
  int err;
  int x = 0;
  z_stream d_stream;		/* decompression stream */

  strcpy ((char *) uncompr, "garbage");

  d_stream.zalloc = (alloc_func) 0;
  d_stream.zfree = (free_func) 0;
  d_stream.opaque = (voidpf) 0;

  d_stream.next_in = compr;
  d_stream.avail_in = 0;
  d_stream.next_out = uncompr;

  err = inflateInit (&d_stream);
  printf ("inflateInit: %s \n", check_err (err));

  while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen)
    {
      d_stream.avail_in = d_stream.avail_out = 1;	/* force small buffers */
      err = inflate (&d_stream, Z_NO_FLUSH);
      if (err == Z_STREAM_END)
	break;
      if (err < 0)
	{
	  printf ("Inflate %s \n", check_err (err));
	  return -1;
	}

    }

  err = inflateEnd (&d_stream);
  printf ("To2: %s \n", check_err (err));
  return d_stream.total_out;
}


char *czat_privmsg_crypt(char *nick, char *p ) {
    char *tmp=xalloc(strlen(p)+1);
    int klucz=strlen(nick);
    int x,i;
    x=0;

    for (i=0;i<strlen(p);i++) {
	    	tmp[i]=p[i]^(nick[x]^88);
		if (x++==klucz) x=0;
    }

    return tmp;
}

void czat_privmsg(char *ournick,int p,char *nick,char *wiadomosc){
    char *buf,*ptr;
    int len;

    len = strlen(nick)+strlen(wiadomosc)+4;
    buf = xalloc(len+1);
    ptr = czat_privmsg_crypt(ournick,wiadomosc);
//    printf("Zapisywany msg zakodowany: %c%c%c%s%c%s",len,0x41,0x41,nick,0x41,ptr);
    sprintf(buf,"%c%c%c%s%c%s",len,0x00,0x03,nick,0x00,ptr);
    write(p,buf,len);
    xfree(ptr);
    xfree(buf);
}


void say(int gdzie,char *fmt,...){
    char buf[4048];
    va_list ap;

    memset(buf,0x0,sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf,4040,fmt,ap);
    va_end(ap);
    write(gdzie,buf,strlen(buf));
}

char *czat_parse_priv(char *ptr, int len) {
	   char *tmpbuf = xalloc(len);
	   int i,x;
	   
	   printf("[PARSE-MSG]: LEN: %d \n",len);
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


void parse (char *ptr, int len) {
    int x,y,z;
    int pp;
    char *p,*tmp;
    Byte *uncompr;
    uLong uncomprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
    int pre,unp,total;


    printf("[?] Rozpatrujemy : %d \n",len);
    for (x=0; x<len; x++) printf("0%x|",ptr[x]&0xff);
    printf("\n");
    printf("[0] : %x \n",ptr[0]&0xff);
    printf("[1] : %x -> ",ptr[1]&0xff);
    
//    if ((ptr[0]&0xff)!=0x00) {printf("PROTOCOL BROKEN!->");}
    
    switch (ptr[1]&0xff) {
        case 0x80: 
		   printf("JOIN: ");printf("%s",ptr+4);
	           break;

	case 0x81: 
		   x = strlen(ptr + 3);
		   p = ptr + 3 + x + 1;
		   tmp=czat_parse_priv(p,len-3-x-1);
		   printf("PRIVMSG <%s> %s ",ptr+3,tmp);
		   xfree(tmp);
		   break;
	case 0x8b: 
		printf("Wiadomosc spakowana zlibem! Probujemy dzialac - START ------");
	        uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
	        unp=decompress_inflate (ptr+2, len-3 , uncompr, uncomprLen);
		printf("[???] uncomprLEN: %d \n",unp);
//		printf("[0]= %c \n",ptr[2]);
//		for (x=0;x<uncompr[0];x++) 
//		    printf("%c",uncompr[x]);
//		    printf("\n");
		printf("\n ----- SPRWDZENIE START ----------\n");
			for (x=0;x<unp;x++) 
		    printf("0%x|",uncompr[x]&0xff);
		    printf("\n");
		printf("\n ----- SPRWDZENIE STOP ------------\n");
		total=0;
		while (total<unp) {
                    printf("[UNPACKED-LEN-> %x & %x ]\n",uncompr[0],uncompr[1]);
		    pre = ((uncompr[1]<<8)&0xffff) | (uncompr[0]&0xff);
		    parse(uncompr+1,pre);
		    total+=pre;
		    uncompr+=pre;
		}
//		printf("[UNPACKED-LEN-> %x & %x ]\n",uncompr[0],uncompr[1]);
//		pre = ((uncompr[1]<<8)&0xffff) | (uncompr[0]&0xff);
//		parse(uncompr+1,pre,k);
//		parse(uncompr+1,pre);
		printf("Wiadomosc spakowana zlibem! Probujemy dzialac - END ------");
		   
	        break;
		   
        case 0x82: printf("PART: ");printf("%s",ptr+2);
		   break;

	case 0x83: printf("Nieznany 83-> ");
		   for (pp=0;pp<len;pp++) if (isprint(ptr[pp])) printf("%c:",ptr[pp]);
		   break;

	case 0x84: printf("Lista osob na kanale-> ");
		   ptr+=4;
		   for (pp=0;pp<len-4;pp++) 
			if (ptr[pp]!=0x00) printf("%c",ptr[pp]);
			    else
			   { pp+=2; printf(" ");}
		   break;
	case 0x86: printf("Zmiana stanu -> ");
		   for (pp=0;pp<len;pp++) if (isprint(ptr[pp])) printf("%c:",ptr[pp]);
		   break;

        case 0x96: printf("Error!");exit(-1);break;
	default: printf("Nieznany");
    }
    printf("\n");
}

int main(int argc,char *argv[]) {
    struct sockaddr_in sck;
    int pipe;
    char buf[8048];
    int ret,i,ii;
    unsigned char len;
    unsigned char multi;
    int pre;
    int count;
    
    sck.sin_family=PF_INET;
    sck.sin_port=htons(atoi(argv[3]));
    sck.sin_addr.s_addr=inet_addr("217.74.65.63");
    
    pipe=socket(PF_INET,SOCK_STREAM,0);
    perror("socket");
printf("[+] Connecting..... [%d]\n",atoi(argv[3]));
    connect(pipe,(struct sockaddr *)&sck,sizeof(sck));

printf("[:] Conneted! \n");
    bzero(buf,sizeof(buf));
    bzero(buf,sizeof(buf));
    read(pipe,&len,1);
    read(pipe,&multi,1);
    printf("[?] LEN: %x | MULTI: %x \n",len,multi);
    pre = ((multi<<8)&0xffff) | (len&0xff);
    printf("[+] Wczytana dlugosc startowa: %d[REAL: %x] \n",len,pre);
    ret=read(pipe,buf,pre - 2);    
    parse(buf,ret);
//    read(pipe,buf,sizeof(buf));
//    printf("[GOT %d]: %s \n",strlen(buf),buf);
    bzero(buf,sizeof(buf));
    printf("[+] Ksywka: %s , Kanal: %s \n",argv[1],argv[2]);
//    printf("rzutowanie: [%d] [%d]: \n",0x10,atol(10));
//    sprintf(buf,"%c%c%cmojaka%cFlirt%c",0x10,0x00,0x00,0x00,0x00);
    sprintf(buf,"%c%c%c%s%c%s%c",(strlen(argv[1])+strlen(argv[2])+5)&0xff,0x00,0x00,argv[1],0x00,argv[2],0x00);
//    printf("Zapis: [%d]: %x%c%c%s%c%s%c\n",strlen(argv[1])+strlen(argv[2])+2+3,(strlen(argv[1])+strlen(argv[2]) +2 )&0xff,0x41,0x41,argv[1],0x41,argv[2],0x41);
//    printf("[-=-]: %x\n",(18+3)&0xff);
//    printf("[-=-]: %d\n",(strlen(argv[1])+strlen(argv[2]) +2));
    write(pipe,buf,(strlen(argv[1])+strlen(argv[2]) +5));
//  czat_privmsg(argv[1],pipe,argv[4],"yo");


while (1) {
    bzero(buf,sizeof(buf));
    read(pipe,&len,1);  printf("[+] Wczytana dlugosc    : %d[%x] \n",len,len);
    read(pipe,&multi,1);printf("[+] Wczytany multiplexer: %d[%x] \n",multi,multi);
    pre = ((multi<<8)&0xffff) | (len&0xff);
    printf("Przewidywana wartosc sczytania-pre: %d \n",pre );
    printf("Przewidywana wartosc sczytania: %d \n",((multi<<8)&0xffff) | (len&0xff));

    buf[0]=multi;
    count = 0;
    while (count < pre - 2){
               ret=read(pipe,buf+1+count,pre - 2 - count);
	       count+=ret;
	       printf("[Read]: %d \n",ret);
	   }
	   ret = count+2;
//    for (i=0; i<ret; i++) printf("0%x|",buf[i]);fflush(0);
    printf("[+ Read ]: %d \n",ret);
//    for (i=0; i<ret; i++) printf("0%x|",buf[i]);
    fflush(0);
    printf("\n");
    parse(buf,ret);
    bzero(buf,sizeof(buf));
    if (ret==0) break;
    }
}    