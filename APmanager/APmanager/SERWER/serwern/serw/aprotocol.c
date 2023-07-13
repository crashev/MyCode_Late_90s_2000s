/*

    Plik zawierajacy SPECYFIKACJE protkolu ADMINIstracji, 
    wszelkie komendy protokolu rozpoznawane, obslugiwane i dodawane sa tutaj.
    (C) Komunix - Pawel Furtak '2005
    
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include "server.h"
#include <sys/stat.h>
#include <stdarg.h>

static int iSock = 0;
struct client_struct clients[ MAXCLIENTS];
static int iCurClient = 0;

static void
reply(const char *spcFormat, ...)
{
            static char scBuf[1024];
	    va_list ap;
            memset(scBuf, 0x0, sizeof(scBuf));
            va_start(ap, spcFormat);
            vsnprintf(scBuf, sizeof(scBuf) - 1, spcFormat, ap);
            va_end(ap);
	    if (iSock>0) 
		write(iSock, scBuf, strlen(scBuf));
}



static int
acmd_whoami(char **pcPtr, int iParamCount)
{
    reply("%s", "=END - [ADMIN-WHOAMI]: You'r admin! \n");
    return 0;
}


static int
acmd_status(char **pcPtr, int iParamCount)
{
    int iX;
    for (iX = 0; iX< MAXCLIENTS; iX++) 
	    if (clients[iX].stat) {
		reply("%s\t%s\t%d\t%s\n", clients[iX].name,
		clients[iX].host,clients[iX].stat, clients[iX].cversion);
	    }
    reply("=END\n");
    return 0;
}


static int
acmd_remote(char **pcPtr, int iParamCount)
{
    int iX;
    char cBuf[1024];
    
    if (iParamCount<2) {
	reply("=END - Wrong Param Count(%d), usage: +REMOTE [ROUTER] [PARAM1] [PARAM2]...\n", iParamCount);
	return -1;
    }
    
    bzero(cBuf, sizeof(cBuf));
    for (iX=1; iX< iParamCount; iX++) {
	strncat(cBuf, pcPtr[iX], sizeof(cBuf) - 1);	    
	strncat(cBuf, " ", sizeof(cBuf) - 1);	    
    }
    for (iX=0; iX<MAXCLIENTS; iX++)
	if ( (clients[iX].stat)&&(!strcmp(clients[iX].name, *pcPtr)) )
	{
	    reply("=SENDING %s to %s \n", cBuf, *pcPtr);
	    bio_printf(clients[iX].io, "%s\n", cBuf);
	    return 0;
	}
    
    reply("=END - %s not found in my clients table\n", *pcPtr);
    return -1;
}


static int
acmd_switch(char **pcPtr, int iParamCount)
{
    int iX = 0;
    
    if (iParamCount<1) {
	reply("=END\n");
	return -1;
    }
    for (iX=0; iX<MAXCLIENTS; iX++)
	if ( (clients[iX].stat) && (!strcmp(clients[iX].name, *pcPtr)) ) 
	{
	    reply("=SWITCHING TO %d \n", clients[iX].fd);
	    iCurClient = clients[iX].fd;
	    return 0;
	}
    reply("=END - No such Router\n");	
    return -1;
}


void switch0(void)
{
    iCurClient = 0;
}


/*
    Protocol Commands
*/

static struct protoCMD
{
        char command[20];
	int (*function)(char **, int);
} protoCMDs[] =
{
      { "+WHOAMI",     acmd_whoami,     },
      { "+STATUS",     acmd_status,     },
      { "+REMOTE",     acmd_remote, 	},
      { "+SWITCH",     acmd_switch,     },
};


int 
do_adminreply(int iX, char *pcBuf, int aSock) 
{
    if ( (iCurClient)&&(iCurClient==clients[iX].fd)&&((aSock)>0) ) 
    {
	write(aSock, pcBuf, strlen(pcBuf));
	write(aSock, "\n", 1);
	return 0;
    }    
    
    return -1;
}



int 
do_admincmd(char *pcBuffor, int iSockGet)
{
        int  iAllCmds = (sizeof(protoCMDs) / sizeof(struct protoCMD));
        int  iX = 0;
        char **ap, *pcParams[10];
	int	    iParams=0;

	iSock = iSockGet;

          chop(pcBuffor);
    	  if ((strlen(pcBuffor))<1)
			    return 0;
	  printf("[ADMIN-DOCMD]: %s (%d)\n", pcBuffor, strlen(pcBuffor));	
	  for (ap = pcParams; (*ap = strsep(&pcBuffor, " \t")) != NULL;iX++)
    	       if (**ap != '\0')
        	   if (++ap >= &pcParams[10])
            	    break;
        
		iParams=iX;
//	        if (iParams>0)
	    printf("[ADMIN-PARAMS]: %d \n", iParams);
		for (iX=0; iX<iAllCmds; iX++) {
			if (!strcmp(protoCMDs[iX].command, pcParams[0])) {
			    return protoCMDs[iX].function(pcParams+1, iParams-1);
			}
	        }
    
	        reply("%s", "=END - [-] Unknown command \n");
    
        return 0;
}

