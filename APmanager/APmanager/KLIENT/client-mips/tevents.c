/*
    Time Events File 
    (C) Komunix - Pawel Furtak '2005
    
*/

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "tevents.h"
#include "iptables.h"
#include "log.h"

static char *globalApName;
extern int iMyStatus;
 /* Protocol functions */
 
static char *StatsTraffic();
static char *StatsIpPorts_20_995_Traffic();
static char *StatsPerClient();


struct timeEvent
{
        char cEventName[15];
        char *(*function)();
        int iInterVal;
        int iLastEvent;
    
} timeEvents[] =
{
    {"Stats-Traffic", StatsTraffic, 5, 0, },
    {"Stats-IpTraffic", StatsIpPorts_20_995_Traffic, 5, 0, },
    {"Stats-PerClient", StatsPerClient, 5, 0, }
};


static char*
StatsTraffic()
{
        static char scRetBuf[1024];
        static char iface[50],read[50],write[50]; 
        int ret;
	FILE *in = fopen("/proc/net/dev", "r");
        if (!in)
	     return NULL;

        bzero(scRetBuf, sizeof(scRetBuf));
    
	fscanf(in, "%*[^\n]\n%*[^\n]"); /* skip two lines */

	while ( (ret = fscanf(in, " %16[^:]:%s %*s %*s %*s %*s %*s %*s %*s %s%*[^\n]\n",iface,read,write))==3) {
//	    if (!strncmp(iface,"eth",3)) {
	        strncat(scRetBuf,"+STATS TRAFFIC ", sizeof(scRetBuf)-1);
	        strncat(scRetBuf,globalApName, sizeof(scRetBuf)-1);
		strncat(scRetBuf," ", sizeof(scRetBuf) - 1);
		strncat(scRetBuf,iface, sizeof(scRetBuf) - 1);strncat(scRetBuf,"-traffic.rrd", sizeof(scRetBuf) - 1);strncat(scRetBuf," ", sizeof(scRetBuf) - 1);
		strncat(scRetBuf,read, sizeof(scRetBuf) -1 );strncat(scRetBuf," ", sizeof(scRetBuf) - 1);
		strncat(scRetBuf,write, sizeof(scRetBuf) -1 );
		strncat(scRetBuf,"\n", sizeof(scRetBuf) - 1);
//	    }
	}
        fclose(in);
    
        return scRetBuf;
}



unsigned long long
forward(char *pcMark)
{
    char *ptr = get_mark("PORTS", pcMark);
    	    if (ptr!=NULL) return(atoll(ptr));
		    else
	    return 0;
}		



/* 
    Funkcja wyciaga informacje o ruchu na portach 20-995 zbierane i zaznaczane
    przez iptables
*/

static char *
StatsIpPorts_20_995_Traffic()
{
    static char scRetBuf[255];
	    
    bzero(scRetBuf, sizeof(scRetBuf));
    snprintf(scRetBuf, sizeof(scRetBuf) - 1,"+STATS IPTRAFFIC %s ports20-995-iptraffic.rrd %llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu:%llu\n", globalApName, 
    forward("0x11"), 
    forward("0x12"), 
    forward("0x13"), 
    forward("0x14"), 
    forward("0x15"), 
    forward("0x16"), 
    forward("0x17"), 
    forward("0x18"), 
    forward("0x19"), 
    forward("0x1a"), 
    forward("0x1b"), 
    forward("0x1c"), 
    forward("0x1d"), 
    forward("0x1e"), 
    forward("0x1f"), 
    forward("0x20"), 
    forward("0x21"), 
    forward("0x22"), 
    forward("0x23"), // 993 
    forward("0x24"), 
    forward("0x25"),  // 995
    forward("0x26") 
    );

    return (scRetBuf);
}



static char*
StatsPerClient()
{
    
    char *ptr = getClientTraffic (globalApName);
//    printf("\n-------------------------------------------------\n");
//    printf("%s", ptr);
//    printf("\n");
    return ptr;
}
















/* Ok do some Time Based Events */

void TimeEvents(BIO *bio, char *pcApName) 
{
	    /* Time events */
	    int iRet=0;
	    time_t tEventTime;
	    struct tm *stmEvent; 
	    int iCountAll = (sizeof(timeEvents)/sizeof(struct timeEvent));
	    int iX = 0;
	    char *pcPtr;
	    
	    tEventTime  = time(NULL);
	    stmEvent 	= localtime(&tEventTime);
	    globalApName = pcApName;
	    
	    for (iX=0; iX<iCountAll; iX++) {
		if (( stmEvent->tm_min % timeEvents[iX].iInterVal == 0)
			&&(timeEvents[iX].iLastEvent!=stmEvent->tm_min)) {
			mprintf("(%d)[%s] - Exectuing Now! \n", stmEvent->tm_min,timeEvents[iX].cEventName);
			timeEvents[iX].iLastEvent=stmEvent->tm_min;	
			pcPtr = timeEvents[iX].function();
			if (pcPtr) {
//			    iRet=bio_printf(bio, "%s", pcPtr);
			if (iMyStatus)		
			    iRet=BIO_write(bio, pcPtr, strlen(pcPtr));
	//	    	    iRet=bio_printf(bio, "%s", pcPtr);

			    if (iRet>0) 
				BIO_flush(bio);
			}
			
			}
			
	    }

}
