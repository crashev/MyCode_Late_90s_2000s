#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

void eat_spaces(char *str)
{
    int iX=0;
    int iY=0;
    
    for (iX=0; iX<strlen(str); iX++)
	if (str[iX]==' ')
	    {
		// przesuwamy
		for (iY=0; iY<strlen(str)-iX; iY++)
    		    str[iX+iY]=str[iX+iY+1];
	    }
}

static int reads(int *sock,char *buf)
{
 int c,count=0;
 do c=read(*sock,&buf[count],1); while ((c>0)&&(buf[count++]!='\n'));
 buf[count]='\0';
 return (c<=0)?c:count;
}

static void chop(char *pcPtr)
{
    int iX;
    for (iX=0; iX< strlen(pcPtr); iX++)
	if ( (pcPtr[iX]=='\n')||(pcPtr[iX]=='\r') )
	    pcPtr[iX]=0x0;
}


int main(int argc, char *argv[])
{

 /* Mysql Config Stuff */
    char user[50];
    char password[50];
    char dbname[50];
    char sock[255];
    int iConf=0;
    char buf[1024];
    int altlog_fd = open("config", O_RDONLY);

    if (altlog_fd < 0)
	return -1;			
    while (reads(&altlog_fd, buf))
    {
        // eat spaces
        eat_spaces(buf);
	chop(buf);
        if (!strncasecmp(buf,"USER:",5)) {
            strncpy(user, buf+5, sizeof(user) - 1);
            iConf++;
        }

        if (!strncasecmp(buf,"PASSWORD:",9)) {
            strncpy(password, buf+9, sizeof(password) - 1);
            iConf++;
        }

        if (!strncasecmp(buf,"DBNAME:",7)) {
            strncpy(dbname, buf+7, sizeof(dbname) - 1);
            iConf++;
        }

        if (!strncasecmp(buf,"SOCK:",5)) {
            strncpy(sock, buf+5, sizeof(sock) - 1);
            iConf++;
        }
        bzero(buf, sizeof(buf));

    }
    
    printf("Konfiguracja wczytana, wynik: %d \n", iConf);
    if (iConf == 4)
    {
	printf("USER  : %s \n", user);
	printf("PASS  : %s \n", password);
	printf("DBANEM: %s \n", dbname);
	printf("SOCK  : %s \n", sock);

    }    
	return 0;
}

