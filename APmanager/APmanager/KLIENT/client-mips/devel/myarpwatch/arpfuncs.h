#ifndef _APRFUNCS_H
#define _ARPFUNCS_H

#include <libnet.h>
#include <string.h>
#include <time.h>

struct arh_struct {
        u_char sha[6];  // source hw addr
        u_char spa[4];  // source protocol addr
        u_char tha[6];  // destination hw addr
        u_char tpa[4];  // destination protocol addr
};


struct arptable_struct {
        char 		   *pcIp;		// IP address
        char 		   *pcHwaddr;	// Ethernet Hardware address
        int  		   iStat;	// state-> 0 - nothing added to ipt or error occured
					// 	   1 - added entry to MASQUERADE
					//	   2 - added entry to REDIRECT
        time_t 		   tTime;	// time of adding - first seen
        struct arptable_struct *next;
};

extern char *ether2str(u_char *upcP);
extern char *ip2str(u_char *upcP);

extern int  arp_add(struct arptable_struct **l, char *pcEther, char *pcIp);
extern void arp_print(struct arptable_struct *l);
extern int  arp_exist(struct arptable_struct *arp, char *pcEther, char *pcIp); 
extern void arp_flush(struct arptable_struct **arp);
extern int  arp_count(struct arptable_struct *l);
#endif
