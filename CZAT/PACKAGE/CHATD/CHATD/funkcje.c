#include "includy.h"
#include <time.h>

extern int pin,cli;

void say(int gdzie,char *fmt,...){
    char buf[4048];
    va_list ap;

        memset(buf,0x0,sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf,4040,fmt,ap);
    va_end(ap);
    write(gdzie,buf,strlen(buf));
}


int check_ramka(char *o) 
{
    char dopuszczalne_znaki[] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-#:\n\r\0";
    int ret=0;
    int x,y,found=0;
    
    if ( (strlen(o)>2)&& (strlen(o)<30) ) ret = 1; else return 0;
    
    for (y=0;y<strlen(o);y++)
    {
	found=0;
        for (x=0;x < strlen(dopuszczalne_znaki); x++) 
		    if (o[y]==dopuszczalne_znaki[x]) found=1;
        if (found==0) { ret=0; break;}
    }
    return ret;
}


void chop(char *co) 
{
int a;
for (a=0;a<=strlen(co);a++) 
    {
     if ((co[a]=='\n')||(co[a]=='\r')) co[a]=0x0;
    }
}

// Operacje na lancuchach

void upcase(char *s) 
{

    int a=0;
    
    for (a=0; a<=strlen(s)-1; a++)
    {
        s[a]=toupper(s[a]);
    }
}

	
void debug(FILE *gdzie,char *fmt,...){
    char buf[4048];
    char timebuf[30];
    va_list ap;
        time_t t;
	struct tm *tm;
        t=time(NULL);
	bzero(timebuf,sizeof(timebuf));
	tm=localtime(&t);
	strftime(timebuf,sizeof(timebuf)-1,"%H:%M:%S",tm);


    memset(buf,0x0,sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf,4040,fmt,ap);
    va_end(ap);
//    write(gdzie,buf,strlen(buf));
    fprintf(gdzie,"%s -> %s",timebuf,buf);
    fflush(gdzie);
}
