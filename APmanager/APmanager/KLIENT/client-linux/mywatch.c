/*


    MyWatchModule
    
    Zadania:
    - Podgladanie aktywnych polaczen
    - Podgladanie transferow
    
    (c) Komunix - Pawel Furtak '2005    
*/

#define INTERVAL 2	// show dump every 2 secs
#define UPDATE 1	// update at least every 2secs

#define MAX_TRACED		100
#define DUMP_SHOW		20
#define START_VALUE		100

#include <stdio.h>
#include <string.h>
#include <libnet.h>
#include <pcap.h>

#include <time.h>
#include "log.h"

static int iCount = 0;

struct conn_tracker_struct
{
    char cEtherSrcAddr[50];
    char cEtherDstAddr[50];
    char cSrc[20];
    char cDst[20];
    int  iSport;
    int  iDport;
    u_long TotalBytes;
    u_long LastBytes;
    int lastAvg;
    time_t tStart;
    time_t tLast;
    int used;
    int iProto;
};

struct conn_tracker_struct cTracker[MAX_TRACED];

int find_free() {
    int iX;
    for (iX=0; iX<MAX_TRACED; iX++)
	if (cTracker[iX].used==0)
	    return iX;
    
    return -1;
}

char *
ether2str(u_char *ucpP) 
{
        static char scEther[20];

        bzero(scEther, sizeof(scEther));
        snprintf(scEther, sizeof(scEther) - 1 ,"%x:%x:%x:%x:%x:%x", ucpP[0], ucpP[1], ucpP[2], 
								    ucpP[3], ucpP[4], ucpP[5]);

        return scEther;
}


void dump(char *pcFile)
{
    time_t tTmp;
    struct tm *tm;
    int iX = 0;
    char cProto[20];
    char cTime[20];
    unsigned long iTotalTraf=0, iTotalSpeed=0;
    FILE *fLog;    
    
    fLog = fopen(pcFile, "w");
    if (!fLog)
	return;
//    fLog=stderr;
    fprintf(fLog, "=============== TRAFFIC %s START ===============\n", pcFile);
    for (iX = 0; iX< DUMP_SHOW; iX++) 
	if (cTracker[iX].used) {
	    tTmp = 0;
	    tTmp = cTracker[iX].tStart;
	    tm = localtime(&tTmp);
	    bzero(cProto, sizeof(cProto));
	    switch (cTracker[iX].iProto) {
		case IPPROTO_TCP:
				 strcpy(cProto, "TCP");
				 break;
		case IPPROTO_UDP:
				 strcpy(cProto, "UDP");
				 break;
		case IPPROTO_ICMP:
				 strcpy(cProto, "ICMP");
				 break;
		default:
				 strcpy(cProto, "UNKNOWN");
				 break;
	    }
/*
    fprintf(fLog,"[%s %s (%d)] -> [%s %s (%d)]\n", 
        cTracker[iX].cEtherSrcAddr, cTracker[iX].cSrc, cTracker[iX].iSport,
         cTracker[iX].cEtherDstAddr, cTracker[iX].cDst, cTracker[iX].iDport);
*/
    fprintf(fLog,"%s %s %d %s %s %d ", 
        cTracker[iX].cEtherSrcAddr, cTracker[iX].cSrc, cTracker[iX].iSport,
         cTracker[iX].cEtherDstAddr, cTracker[iX].cDst, cTracker[iX].iDport);

	    bzero(cTime, sizeof(cTime));
	    if ((tm->tm_sec<10)&&(tm->tm_min>9))
		snprintf(cTime, sizeof(cTime) - 1,"%d:%d:0%d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	    else if ((tm->tm_sec>9)&&(tm->tm_min<10))
		snprintf(cTime, sizeof(cTime) - 1,"%d:0%d:%d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	    else if ((tm->tm_sec<10)&&(tm->tm_min<10))
		snprintf(cTime, sizeof(cTime) - 1,"%d:0%d:0%d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	    else
	    	snprintf(cTime, sizeof(cTime) - 1,"%d:%d:%d", tm->tm_hour, tm->tm_min, tm->tm_sec);

	        fprintf(fLog,"%s %s %lu %i", cTime, cProto, cTracker[iX].TotalBytes, cTracker[iX].lastAvg);
	    
	    iTotalTraf  += cTracker[iX].TotalBytes;
	    iTotalSpeed += cTracker[iX].lastAvg;
	    fprintf(fLog, "\n");
	}

    fprintf(fLog,"[TOTAL TRAFFIC: %luMB, TOTAL SPEED: %luKbps\n", iTotalTraf/(1024*1024),iTotalSpeed/1024);
    fprintf(fLog,"=============== TRAFFIC %s END ===============\n", pcFile);
    fflush(0);
    fclose(fLog);
    
}




void 
sort() 
{
    struct conn_tracker_struct connTmp;
    int iZamiana = 0;
    int i;
	    /* Posortowac! */

	// sort po totalu
/*
	    while (1) {
	    	iZamiana=0;
		for (i=0;i<MAX_TRACED-1 ;i++)
		    if ( (cTracker[i].used)&&(cTracker[i+1].used) )
			if ( (cTracker[i].TotalBytes) < (cTracker[i+1].TotalBytes) ) 
			{ 
			    bzero(&connTmp, sizeof(connTmp));
			    memcpy((void *)&connTmp, (void *)&cTracker[i], sizeof(cTracker[i]) - 1);
			    bzero(&cTracker[i], sizeof(cTracker[i]));
			    memcpy((void *)&cTracker[i],(void *)&cTracker[i+1] , sizeof(cTracker[i]) - 1);
			    bzero(&cTracker[i+1], sizeof(cTracker[i+1]));
			    memcpy((void *)&cTracker[i+1],(void *)&connTmp, sizeof(cTracker[i]) - 1);
			    iZamiana=1;
			}
	    	if (!iZamiana) break;
	    }			
*/    
	    /* A teraz po speedzie */
	while (1) 
	{
	    	iZamiana=0;
		for (i=0;i<MAX_TRACED-1 ;i++)
		    if ( (cTracker[i].used)&&(cTracker[i+1].used))
			if ( (cTracker[i].lastAvg) < (cTracker[i+1].lastAvg) ) 
			{ 
			    bzero(&connTmp, sizeof(connTmp));
			    memcpy((void *)&connTmp, (void *)&cTracker[i], sizeof(cTracker[i]) - 1);
			    bzero(&cTracker[i], sizeof(cTracker[i]));
			    memcpy((void *)&cTracker[i],(void *)&cTracker[i+1] , sizeof(cTracker[i]) - 1);
			    bzero(&cTracker[i+1], sizeof(cTracker[i+1]));
			    memcpy((void *)&cTracker[i+1],(void *)&connTmp, sizeof(cTracker[i]) - 1);
	    		    iZamiana=1;
			}
	if (!iZamiana) break;
	}	    			

}

void     dispatch_callback(const u_char *udata, const struct pcap_pkthdr *hdr, const u_char *pcappacket) {
//    int iCount = 0;
    struct timeval tv;
    int iX;
    
    /* Our net headers */

    struct libnet_ethernet_hdr *EthernetHdr 	= NULL;
    struct libnet_ipv4_hdr     *IpHdr		= NULL;
    struct libnet_tcp_hdr      *TcpHdr 		= NULL;
    struct libnet_udp_hdr      *UdpHdr 		= NULL;

    char EtherSrcAddr[50],EtherDstAddr[50];
    char IpSrcAddr[20], IpDstAddr[20];
    int iSrcPort, iDstPort;
    int iLen;
    int iUdp =0, iTcp = 0, iIcmp =0;
    time_t tNow, tPrev;
    int b,c;
    int iUpdate=0;
        
    tv = hdr->ts;

    EthernetHdr   = (struct libnet_ethernet_hdr   *)pcappacket;
    IpHdr  	  = (struct libnet_ipv4_hdr 	  *)(pcappacket +LIBNET_ETH_H);

    /* Not interested in other that IP */
    if (ntohs(EthernetHdr->ether_type)!=ETHERTYPE_IP)
        return;
    
    /* TCP or UDP */
    if (IpHdr->ip_p==IPPROTO_TCP) {
	TcpHdr = (struct libnet_tcp_hdr 	  *)(pcappacket + LIBNET_ETH_H + LIBNET_IPV4_H);
	iSrcPort = TcpHdr->th_sport;
	iDstPort = TcpHdr->th_dport;
	iLen 	 = LIBNET_TCP_H;
	iTcp 	 = 1;
    }
    else if (IpHdr->ip_p==IPPROTO_UDP) {
	UdpHdr = (struct libnet_udp_hdr 	  *)(pcappacket + LIBNET_ETH_H + LIBNET_IPV4_H);
	iSrcPort = UdpHdr->uh_sport;
	iDstPort = UdpHdr->uh_dport;
	iLen 	 = LIBNET_UDP_H;
	iUdp 	 = 1;
    }
    else if (IpHdr->ip_p==IPPROTO_ICMP) {
        iSrcPort = 0;
	iDstPort = 0;
	iIcmp = 1;
    }
    else {
//	printf("[ Unknown IP protocol!]\n");
	return;
    }
    iSrcPort = ntohs(iSrcPort);
    iDstPort = ntohs(iDstPort);
    

    bzero(EtherSrcAddr, sizeof(EtherSrcAddr));
    bzero(EtherDstAddr, sizeof(EtherDstAddr));
    bzero(IpSrcAddr, sizeof(IpSrcAddr));
    bzero(IpDstAddr, sizeof(IpDstAddr));        

    strncpy(EtherSrcAddr, ether2str(EthernetHdr->ether_shost), sizeof(EtherSrcAddr) - 1);
    strncpy(EtherDstAddr, ether2str(EthernetHdr->ether_dhost), sizeof(EtherDstAddr) - 1);


    if (ntohs(EthernetHdr->ether_type)!=ETHERTYPE_IP)
        return;

    strncpy(IpSrcAddr, inet_ntoa(IpHdr->ip_src), sizeof(IpSrcAddr) - 1);
    strncpy(IpDstAddr, inet_ntoa(IpHdr->ip_dst), sizeof(IpDstAddr) - 1);


    /* Sprawdz czy taki wpis istnieje juz w tabeli */
    iUpdate=0;
    tNow  = time(NULL);
    for (iX=0; iX< MAX_TRACED; iX++) {
     if (cTracker[iX].used) 
     {
        tPrev = cTracker[iX].tLast;
        b = (int)difftime(tNow, tPrev);
	
	if ( (!strcmp(cTracker[iX].cSrc, IpSrcAddr))&&(!strcmp(cTracker[iX].cDst, IpDstAddr)) 
	    &&(cTracker[iX].iSport == iSrcPort)&& (cTracker[iX].iDport == iDstPort) )
	{
	    cTracker[iX].TotalBytes += hdr->len;
	    cTracker[iX].LastBytes  += hdr->len; 
	    iUpdate=1;

	}
	/* Count average speed */
	    if (b>=UPDATE) {
		c = cTracker[iX].LastBytes / b;
		cTracker[iX].tLast 	 = time(NULL);
		cTracker[iX].LastBytes   = 0;
		cTracker[iX].lastAvg     = c;
	    }	    
    }
}// for
	    /* Posortowac! */

            sort();

	    if (iUpdate)
		return;
	

	    if ((iCount+1)>=MAX_TRACED) {
		iCount--;
	    }

	    bzero(&cTracker[iCount], sizeof(cTracker[iCount]));


	    strncpy(cTracker[iCount].cEtherSrcAddr, EtherSrcAddr, sizeof(cTracker[iCount].cEtherSrcAddr)-1);
	    strncpy(cTracker[iCount].cEtherDstAddr, EtherDstAddr, sizeof(cTracker[iCount].cEtherDstAddr)-1);
	    strncpy(cTracker[iCount].cSrc, IpSrcAddr, sizeof(cTracker[iCount].cSrc)-1);
	    strncpy(cTracker[iCount].cDst, IpDstAddr, sizeof(cTracker[iCount].cDst)-1);
	    cTracker[iCount].tLast 	 	 = time(NULL);
	    cTracker[iCount].LastBytes 	+= hdr->len; 
	    cTracker[iCount].lastAvg 	 = 1500;
	    cTracker[iCount].used 	 = 1;
	    cTracker[iCount].tStart 	 = time(NULL);
	    if (iTcp||iUdp) {
		cTracker[iCount].iSport 	 = iSrcPort;
		cTracker[iCount].iDport 	 = iDstPort;
	    }
	    else {
	    	cTracker[iCount].iSport 	 = 0;
		cTracker[iCount].iDport 	 = 0;

	    }
	    cTracker[iCount].iProto 		 = IpHdr->ip_p;

	    iX=iCount;

	    iCount++;
	    sort();

	    return ;	    	
}




int 
mywatch(char *pcIface)
{
    struct 	pcap_if *pcapIF, *ptr;
    int 	iRet;
    char 	cErrbuf[PCAP_ERRBUF_SIZE];
    pcap_t 	*pd;    
    char 	cPath[50];
    time_t 	tNow, tLast;
    int 	iFound = 0;
    iRet = pcap_findalldevs( &pcapIF, cErrbuf);
    ptr  = pcapIF;
    mprintf("[?] Starting MYwatch for iface: %s \n", pcIface);
    while (pcapIF) {
	if (!strcmp(pcIface, pcapIF->name))
	    iFound=1;
	pcapIF=pcapIF->next;
    }
    if (ptr) 
	pcap_freealldevs(ptr);
    if (!iFound) {
        mprintf("[?] Interface %s not found! \n", pcIface);
	return 0;
    }
    if ( (pd = pcap_open_live(pcIface, LIBNET_ETH_H + LIBNET_IPV4_H + 1024, 0, 500, cErrbuf))==NULL) 
    {
		mprintf("pcap_open_live() : %s \n",cErrbuf);
	        return -1;
    }


    for (iRet=0; iRet<   MAX_TRACED; iRet++)
	    bzero(&cTracker[iRet], sizeof(cTracker[iRet]));
 
//    signal(SIGALRM,dump);
//    alarm(INTERVAL);

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s.log", pcIface);
    
    tNow = tLast = time(NULL);

    mprintf("[?] Tracing %s...\n", pcIface);
    while (1) 
    {
	iRet  = pcap_dispatch(pd, 10, (pcap_handler)dispatch_callback, NULL);
	tNow = time(NULL);
	if (difftime(tNow, tLast)>=UPDATE) {
	    dump(cPath);
	    tLast=time(NULL);
	}

    }

    pcap_close(pd);
    return 0;

}
