#include "includy.h"

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
if (strlen(o)>10) return 1; else return 0;
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

	
