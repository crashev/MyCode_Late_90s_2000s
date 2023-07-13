#include "arpfuncs.h"



char *
ether2str(u_char *ucpP) 
{
        static char scEther[20];

        bzero(scEther, sizeof(scEther));
        snprintf(scEther, sizeof(scEther) - 1 ,"%x:%x:%x:%x:%x:%x", ucpP[0], ucpP[1], ucpP[2], 
								    ucpP[3], ucpP[4], ucpP[5]);

        return scEther;
}



char *
ip2str(u_char *ucpP) 
{
        static char scIp[20];

        bzero(scIp, sizeof(scIp));
        snprintf(scIp, sizeof(scIp) - 1, "%u.%u.%u.%u", ucpP[0], ucpP[1], ucpP[2], ucpP[3]);

        return scIp;
}



int 
arp_add(struct arptable_struct **l, char *pcEther, char *pcIp)
{
        struct arptable_struct *new = NULL, *tmp = NULL;

        new = (struct arptable_struct *)malloc(sizeof(struct arptable_struct));
        if (new == NULL) {
                printf("[!] Brak pamieci dla nowego elementu!\n");
                return 0;
        }
	
	printf( "[?] ARP ADDING %s -> %s \n", pcEther, pcIp);
        new->pcIp     = strdup(pcIp);
	new->pcHwaddr = strdup(pcEther);
        new->iStat    = 0;
	new->tTime    = time(NULL);
	new->next     = NULL;

	if ( (tmp = *l) == NULL) {
                *l = new;
                return 1;
        }

        while (tmp->next)
                tmp = tmp->next;

        tmp->next = new;

        return 1;

}

void 
arp_print(struct arptable_struct *arp)
{
        int iI;
	char cTimeBuf[24];

        if (arp == NULL) printf("[!] ARP TABLE is EMPTY \n");
		else 
	{
	    printf(  "----- ARP TABLE ---------------- \n");
    	    for (iI = 0; arp; arp=arp->next, iI++) {
		memset(cTimeBuf, 0x0, sizeof(cTimeBuf));
		asctime_r(localtime((const time_t *)&arp->tTime), cTimeBuf);
		cTimeBuf[strlen(cTimeBuf)-1]='\0';
                printf("[%d] [%s]->(%s)[ADDED %s]\n", iI, arp->pcHwaddr, arp->pcIp, cTimeBuf);
	    }
	    printf("\n----- ARP TABLE ---------------- \n");
	}
}	

/*
    arp_exist()
    0  - table is empty
    1  - mac addr already in table
    2  - mac addr already in table but with different IP address!
    3  - ip in table, but with different MAC!
    -1 - no_record in arp table
*/


int
arp_exist(struct arptable_struct *arp, char *pcEther, char *pcIp)
{
	if (arp == NULL) 
		    return 0; 
	
	do {
		if (!strcmp(arp->pcHwaddr, pcEther)) {
		    if (!strcmp(arp->pcIp, pcIp))
			return 1;
			    else
			return 2;
		}
		if (!strcmp(arp->pcIp, pcIp)) {
		        if (!strcmp(arp->pcHwaddr, pcEther))
			    return 1;
			else
			    return 3;
		}
		
	arp = arp->next;	
	} while (arp);
	
	return -1;
}



void 
arp_flush(struct arptable_struct **arp)
{
	struct arptable_struct *tmp=*arp, *tmp2 = NULL;
	
    	printf("[ARP-FLUSH] Flushing! \n");
	if (tmp==NULL) 
		    return; 

        do {
                if (tmp->pcHwaddr) { 
		    free(tmp->pcHwaddr);
		    tmp->pcHwaddr = NULL;
		}
	    	if (tmp->pcIp) {
		    free(tmp->pcIp);
		    tmp->pcIp = NULL;
		}
		
		tmp2 = tmp;
		tmp = tmp->next;
		if (tmp2) {
		    free(tmp2);
		    tmp2 = NULL;
		}
	} while (tmp);
	
	*arp = NULL;
}

/*
    Returns count of entries in arp table
*/

int 
arp_count(struct arptable_struct *arp)
{
        int iI;

	if (arp == NULL) 
		    return 0;
	else {
    	    for (iI = 0; arp; arp=arp->next, iI++);
	    return iI;
	}
	
	return 0;
}	

