/*
    Linux Module - Getting hw/ip/name of local interfaces and checking against spoofed ARPs

    Author: Pawel Furtak
*/

#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>

#include <netdb.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/types.h>

#include <linux/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "arpfuncs.h"

int 
arp_islocal(char *pcHwaddr, char *pcIpaddr) 
{
         struct ifconf conf;
         struct ifreq *ifek,ifek2;
         struct sockaddr_in *pp;
         u_char *upcHwaddr;
         int iStat;
         char cBuf[4048];
	 char cIpAddrBuf[30];
	 char cHwAddrBuf[30];
	 char cHwAddrParam[30];
         int iM=0, iI, iIlosc;
	 int iRet;
	 
	memset(cHwAddrParam, 0x0, sizeof(cHwAddrParam));
	strncpy(cHwAddrParam, pcHwaddr, sizeof(cHwAddrParam) - 1); 
        iM=socket(AF_INET,SOCK_STREAM,0);
        if (iM==-1) {	
		perror("cant open socket ");
		exit(-1);
        }


        conf.ifc_len = sizeof(cBuf);
        conf.ifc_buf = cBuf;

	iStat=ioctl(iM, SIOCGIFCONF, &conf);
	if (iStat==-1) {
		perror("Somethink goes wrong ");
		exit(-1);
	}
	
	ifek = conf.ifc_req;


	iIlosc=(conf.ifc_len/sizeof(struct ifreq));
	
	for (iI=1; iI<=iIlosc; iI++) {

	    bzero(&ifek2, sizeof(ifek2));
	    strncpy(ifek2.ifr_name, ifek->ifr_name, sizeof(ifek2.ifr_name)-1);
	    if ( (iRet = ioctl(iM, SIOCGIFHWADDR, &ifek2))<0 ) {
		return -1;
	    }
	    upcHwaddr = (unsigned char *)&ifek2.ifr_hwaddr.sa_data;

	    pp=(struct sockaddr_in *)&ifek->ifr_addr;
	    
	    memset(cIpAddrBuf, 0x0, sizeof(cIpAddrBuf));
	    memset(cHwAddrBuf, 0x0, sizeof(cHwAddrBuf));
	    strncpy(cIpAddrBuf, inet_ntoa(pp->sin_addr), sizeof(cIpAddrBuf) - 1);
	    
	    printf("[ARP-ISLOCAL-IP]: %s == %s\n", cIpAddrBuf, pcIpaddr);
	    if (ifek2.ifr_hwaddr.sa_family==1) {
		strncpy(cHwAddrBuf, ether2str(upcHwaddr), sizeof(cHwAddrBuf) - 1);
	        printf("[ARP-ISLOCAL-HW]: %s == %s\n", cHwAddrBuf, cHwAddrParam);

		if (!strcmp(cHwAddrBuf, cHwAddrParam)) {
		    close(iM);
		    return 1;
		}	
	    }
		if (!strcmp(cIpAddrBuf, pcIpaddr)) {
		    close(iM);
		    return 1;
		}	
	    
		    
	    ifek++;
        }	
	    
  return 0;
} 
