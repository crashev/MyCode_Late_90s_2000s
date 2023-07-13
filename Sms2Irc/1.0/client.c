// Sms2irc-client
// By crashev <crashev@ebox.pl>
//            <crashev@crashev.net>
//
//

#include "config.h"
#include <errno.h>
#include <syslog.h>

#include <time.h>

int strip_numer(char *data){
    char *ptr;
    char buf[255];
    if (strlen(data)<250) strcpy(buf,data); else return -1;
    ptr=strtok(buf,"@");
if ((ptr)&&(!strncmp(ptr,"48",2))) return atoi(ptr+2); 
                              else if (ptr) return atoi(ptr);
return -1;
}
int strip_prefiks(char *data)
{
    char buf[255];
    bzero(buf,sizeof(buf));
    if (strlen(data)>250) return -1;
if (!strncmp(data,"48",2)) strncpy(buf,data+2,3); else strncpy(buf,data,3);
return atoi(buf);
}
 
char *siec(int prefiks) 
{
if ((prefiks==601)||(prefiks==603)||(prefiks==605)||(prefiks==607)||(prefiks==609)) return "PlusGsm";
if ((prefiks==600)||(prefiks==602)||(prefiks==604)||(prefiks==606)||(prefiks==608)) return "EraGsm";
if ((prefiks==503)||(prefiks==504)||(prefiks==505)||(prefiks==506)||(prefiks==508)) return "IdeaCentertel";
return "NieznanaSiec";
}

int main(int argc,char *argv[]) {
	char buf[1024];
	char kanal[50];
	char ksywa[250];
	int pip,mozna=0,i=0;
	struct sockaddr_un sck;
        FILE *log;
	    struct tm *tym;
	    char s[255];
            time_t now;
    sck.sun_family=AF_UNIX;
    strcpy(sck.sun_path,SOCK_PATH);	
    pip = socket(PF_LOCAL,SOCK_STREAM,0);	

    if (!pip) 
	{
        syslog(LOG_NOTICE|LOG_ERR,"\nError-Socket : %s \n",strerror(errno));
        return -1;
        }
    if (connect(pip,(struct  sockaddr *)&sck,sizeof(sck))<0)
	{
        syslog(LOG_NOTICE|LOG_ERR,"\nError-Connect : %s \n",strerror(errno));
	return -1;
	}	
now=time(NULL);
tym=localtime(&now);
strftime(s,sizeof(s)-1,"%d-%B-%G.%H:%M:%S",tym);
    memset(buf,0x0,sizeof(buf));	      
    sprintf(buf,"%slog-%s",LOGS_PATH,s);
    log=fopen(buf,"w");
    if (!log) {   
    syslog(LOG_NOTICE|LOG_ERR,"\nError-fopen : %s \n ",strerror(errno)); 
    }

// Wysylanie wiasdomosci na kanal -------------------------------
    memset(buf,0x0,sizeof(buf));	      
    memset(ksywa,0x0,sizeof(ksywa));	      

    while (adres[i].ksywa!=NULL) {
    if (strip_numer(argv[2]+1)==adres[i].numer)
	{
	    strcpy(ksywa,adres[i].ksywa);
	    break;
	}
    i++;
    }
if (strlen(ksywa)==0) strcpy(ksywa,"Nadawca - Nieznany ->");
if (!strncmp(siec(strip_prefiks(argv[2]+1)),"NieznanaSiec",12)) {
strcat(ksywa," ");strcat(ksywa,siec(strip_prefiks(argv[2]+1)));
strcay(ksywa," | ");
if (strlen(argv[2]+1)<200) strcat(ksywa,argv[2]+1);
    } else {
    strcat(ksywa," ");strcat(ksywa,siec(strip_prefiks(argv[2]+1)));
    }
    sprintf(buf,"Od: [%s] \n[%s]: ",ksywa,(!strncmp(argv[1]+1,"Nowa wiadomosc z tel.",21))?siec(strip_prefiks(argv[2]+1)):argv[1]+1);
    write(pip,buf,strlen(buf));
    
    while (fgets(buf,sizeof(buf)-1,stdin)) 
{
    if (!strncmp(buf,"\n",1)) {mozna=1;continue;}
    if (mozna==1) {
    if (strncmp(buf,"------",6)) write(pip,buf,strlen(buf));
    }
    if (log) fprintf(log,"%s",buf);
    memset(buf,0x0,sizeof(buf));
}
// Wysylanie zakonczone --------------------------
    fclose(log);
    log=fopen(LOGS_PATH"system.log","a");
    read(pip,buf,sizeof(buf)-1);
    if (!strncmp(buf,"OK",2)&&(strstr(argv[2]+1,"text.plusgsm.pl")))
    {
        sscanf(buf,"OK %s",kanal);
	memset(buf,0x0,sizeof(buf));
        if (strlen(argv[2]+1)<100) 
	{
	    sprintf(buf,"echo \"Twoja wiadomosc zostala wyslana na kanal %s\"|mail %s -s sms2irc-raport",kanal,argv[2]+1);
            system(buf);
	    fprintf(log,"%s : %s\n",s,buf);
	}
    } else {
    }
    
    fclose(log);
    close(pip);
    return 0;
}   