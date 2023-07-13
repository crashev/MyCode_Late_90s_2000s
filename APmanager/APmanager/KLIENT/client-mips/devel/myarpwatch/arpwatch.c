/* 

*/

#define LIBNET_LIL_ENDIAN 1

#include <stdio.h>
#include <string.h>
#include <pcap.h>
#include <libnet.h>

#include <netinet/in.h>
#include "arpfuncs.h"

#ifndef _FREEBSD
#include "linux_arp.h"
#endif

struct arptable_struct *arptable = NULL;

int main(int argc,char *argv[])  
{
        struct pcap_pkthdr pcap;
        pcap_t *pd;    
        struct libnet_ethernet_hdr *p;
        struct libnet_arp_hdr *ar;
        struct arh_struct *arh;
        char cErrbuf[PCAP_ERRBUF_SIZE];
        u_char *upcNet;
    
        if (argc!= 2) { printf("Usage: %s [device/interface] \n",argv[0]);return -1;}

        if ( (pd = pcap_open_live(argv[1],LIBNET_ETH_H + LIBNET_ARP_ETH_IP_H, 1, 500, cErrbuf))==NULL) {
		printf("pcap_open_live() : %s \n",cErrbuf);
	        return -1;
	}

	printf("(+) Tracking ... \n");
    
    while (1) {
	
		if ( ! (upcNet = (u_char *)pcap_next(pd, &pcap)) ) {
		    printf("[!] pcap_next() error \n");
		    return -1;
		}
	
		p   = (struct libnet_ethernet_hdr *)upcNet;
		ar  = (struct libnet_arp_hdr 	  *)(upcNet +LIBNET_ETH_H);
	        arh = (struct arh_struct 	  *)(upcNet + LIBNET_ETH_H + LIBNET_ARP_H);

		/* Process ARP_REPLY packets */
	
		if (ntohs(p->ether_type) == ETHERTYPE_ARP) 
		    if (ntohs(ar->ar_op) == ARPOP_REPLY) {
		
		switch (arp_exist(arptable, ether2str(arh->sha), ip2str(arh->spa)) ) {
		case  1: 
			    printf("[?] Record %s (%s) already in table \n", ether2str(arh->sha), 
									    ip2str(arh->spa));
			    /* No fw rule for this arp_record yet ? */
			    
			    break;
			
		case  2:
			printf("[!] Record %s (%s) already in table but with different IP! \n", 
							ether2str(arh->sha), ip2str(arh->spa));
			
			break;
		case  3:
			printf("[!] IP %s (%s) already in table,but with different MAC! - Spoof? \n", 	
							    ip2str(arh->spa),ether2str(arh->sha));
			break;
		case  0: 
			printf("[+] Table is empty ! \n");
			
		case -1: 
			printf("[?] No such record in table, adding \n");
			if (!arp_islocal(ether2str(arh->sha), ip2str(arh->spa)) ) {
				
			    arp_add(&arptable, ether2str(arh->sha), ip2str(arh->spa));
			    // if (arp_check_config())
			    // 		arp_fw_add_masquerade()
			    //	else
			    //		arp_fw_add_redirect()
			    } else
			        printf("[!] %s OR %s is local - NOT ADDED!\n", ether2str(arh->sha), ip2str(arh->spa));
			
			
			arp_print(arptable);
			if (arp_count(arptable)>6) 
				    arp_flush(&arptable);
			break;	
		
		}
	    } // if
    
    } // while (1)

} // main()
