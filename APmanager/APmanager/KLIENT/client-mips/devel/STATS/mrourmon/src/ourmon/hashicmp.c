/*
 * hashicmp.c - count ip src icmp errors with mention of errors sent back
 * 	to a specific ip src
 */

/*
 * Copyright (c) 2004 Portland State University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Portland State University.
 * 4. The name of Portland State University may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR/S ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR/S BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <syslog.h>

#include "hashicmp.h"
#include "ehash.c"	/* inlined function */

static void  __inline
storePort(struct icmpnode *n, int dport);


#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#ifdef TESTMAIN
static char *intoa(unsigned int addr);
#endif

/* called before list can be used 
 */
void init_icmplist (struct icmplist *l) {
	l->entries = 0;
	l->last = NULL;
	bzero (&l->hashlist[0], ICMPHSIZE * sizeof(struct icmpnode *));
}

/*
 * search_srcicmp - hash list lookup
 *
 * If found we increment the counters and the total.  
 *	errors are mutually exclusive.
 *
 *	inputs:
 *		l - a list pointer
 *		ipsrc as logical key
 *	returns:
 *		NULL - didn't find it
 *		struct icmpnode *n 
 */ 
struct icmpnode *
search_srcicmp(struct icmplist *l, unsigned long ipsrc, 
	int tcpcount, int udpcount, int icmpcount,
	int unreach, int redirect, int ttl, int udprcv, long udpport)
{
	struct icmpnode *n;
	int mod;

	if (l ->entries == 0) 
		return (NULL);

	/* cache of one lookup and increment counts
	*/
	if (l ->last && (l->last->ipsrc == ipsrc)) {
		if (tcpcount) {
			l->last->tcpcount++;
		}
		else if (udpcount) {
			l->last->udpcount++;
			if (udpport != -1) {
				storePort(l->last, udpport);
			}
		}
		/* note: these are pings sent by the ip src
		*/
		else if (icmpcount) {
			l->last->icmpcount++;
			l->last->totalicmp++;	/* pings might be suspicious too */
		}
		/* because of udp we give unreachables a boost
		*/
		else if (unreach) {
			l->last->unreach++;
			l->last->totalicmp += 2;
		}
		else if (redirect) {
			l->last->redirect++;
			l->last->totalicmp++;
		}
		else if (ttl) {
			l->last->ttl++;
			l->last->totalicmp++;
		}
		else if (udprcv) {
			l->last->udprcv++;
		}
		return (l->last);
	}

	/* search the hash/chain list
	*/
	n = l->hashlist[mod=(ehash((unsigned char *)&ipsrc, 4) % ICMPHSIZE)];
	for (; n!= NULL ; n=n->next) {
		if (n && ( n->ipsrc == ipsrc)) {
			l->last = n;

			if (tcpcount) {
				n->tcpcount++;
			}
			else if (udpcount) {
				if (udpport != -1) {
					storePort(n, udpport);
				}
				n->udpcount++;
			}
			else if (icmpcount) {
				n->icmpcount++;
				n->totalicmp++;
			}
			/* inc counts
			*/
			else if (unreach)  {
				n->unreach++;
				n->totalicmp += 2;
			}
			else if (redirect) {
				n->redirect++;
				n->totalicmp++;
			}
			else if (ttl) {
				n->ttl++;
				n->totalicmp++;
			}
			else if (udprcv) {
				n->udprcv++;
			}
			return(n);
		}
	}
	return (NULL);
}

/*
 *      insert_srcicmp
 *
 *      search_srcicmp succeeds or fails. if it succeeds
 *      we don't call insert_srcicmp.  if it fails, we
 *      call this function to create a new hash list
 *      entry and to set counter values.
 *
 *      returns:
 *              1 - ok
 *              0 - malloc failure
 */

int
insert_srcicmp (struct icmplist *l, unsigned long ipsrc, int tcpcount, int udpcount, int icmpcount, long udpport)
{ 
	struct icmpnode *n;
	struct icmpnode *h, *prev;
	unsigned int mod;
	int i;

	n = (struct icmpnode *) malloc(sizeof(struct icmpnode));
	if (n == NULL) {
		syslog(LOG_ERR, "malloc failure in insert_srcudp, entry/count %d",
			l->entries);
		return (0);
	}
	n->totalicmp = n->unreach = n->redirect = n->ttl = 
		n->tcpcount = n->udpcount = n->icmpcount = 0L;
	n->pcount = 0;
	n->udprcv = 0L;
        for ( i = 0; i < MAXUDPPORTS; i++) {
                n->dports[i] = -1;
                n->dpcounts[i] = 0;
        }

	l->entries += 1;
	n->ipsrc = ipsrc;
	if (tcpcount)
		n->tcpcount = 1;
	else if (udpcount) {
		n->udpcount = 1;
		if (udpport != -1) {
			storePort(n, udpport);
		}
	}
	else  {
		n->icmpcount = 1;
		n->totalicmp++;
	}
	
	l->last = n;

	/* insert the icmpnode in the hash list
	*/
	n->next = NULL;
	h = l->hashlist[mod=(ehash((unsigned char *)&ipsrc, 4) % ICMPHSIZE)];
	if (h == NULL) {
		l->hashlist[mod] = n;
	}
	else {
		n->next = h;
		l->hashlist[mod] = n;
	}
	return(1);
}

static void __inline
storePort(struct icmpnode *n, int dport)
{
        int i;

        for ( i = 0; i < MAXUDPPORTS; i++) {
                /* have not seen the port and we have room,
                 * therefore store it and increment counter and return
                 */
                if (n->dports[i] == -1) {
                        n->dports[i] = dport;
                        n->dpcounts[i] = 1;
                        n->pcount++;
/*
printf("XXX: %d %d %d\n",
n->dports[i], n->dpcounts[i], n->pcount);
*/
                        return;
                }
                /* found it, increment count, and return
                */
                if (n->dports[i] == dport) {
                        n->dpcounts[i]++;
                        return;
                }
        }
        /* didn't find it, must not be room,
         * too bad
         */
        return;
}

/*
 * deallocate malloced hash entries
 */

void
destroy_icmplist(struct icmplist *l) 
{
	int i;
	struct icmpnode *n;
	struct icmpnode *prev;
	for (i =0; i< ICMPHSIZE; i++) {
		n = l->hashlist[i];
		while(n != NULL) {
			prev = n;
			n = n->next;
			free(prev);
		}
	}
}

#ifdef TESTMAIN
WARNING - this routine is not right for this module ... needs fixing ...

static
dump_hash(struct icmplist *l) {
	struct icmpnode *h;
	int counters[ICMPHSIZE];
	int i;
	int ls = 0;
	printf("No. of entries %d\n", l->entries);
	bzero(&counters, sizeof(int)*ICMPHSIZE);
	printf("\nhashdump---------\nlist entries %d\n",l->entries);
	for (i=0;i<ICMPHSIZE;i++) {
		h = l->hashlist[i];
		while(h != NULL) {
			ls = 1;
			printf("%d:", i);
			printf("ip src %x", intoa(h->ipsrc));
			printf(":s/f/r: %d %d %d", h->udpcount,
				h->fincount, h->rstcount);
			counters[i]++;
			h = h->next;
		}
		if (ls) {
			printf(":%d \n",i);
		}
		ls = 0;
	}
	printf("\n");
	for (i = 0; i<ICMPHSIZE; i++) {
		if (counters[i] != 0) {
			printf("counters[%d]:%d buckets\n", i, counters[i]);
		}
	}
	printf("end hash dump---------\n");
}
#endif

/* pack the hashlist into an array
 * and return the array.
 *
 *	returns:
 *		1 - packed array exists
 *		0 - malloc failure
 */
static
struct icmpnode *
pack_array(struct icmplist *l)
{
	struct icmpnode *head, *n;
	unsigned int c = l->entries;  /* count of hash entries allocated */
	int i = 0;
	int hcounter = 0;
	int z;

	head = (struct icmpnode *) malloc(c * sizeof (struct icmpnode));
	if (head == NULL) {
		syslog(LOG_ERR, "malloc failure in icmpnode/pack_array, count %d",
			l->entries);
		return (NULL);
	}
	/* walk the hashlist
	*/
	for (i =0 ; i<ICMPHSIZE; i++) {
		n = l->hashlist[i];
		/* if n is non-nulln use, therefore put it in the array,
		* and increment hcounter
		*/
		while (n != NULL) {
			head[hcounter].totalicmp = n->totalicmp;
			head[hcounter].ipsrc = n->ipsrc;
			head[hcounter].tcpcount = n->tcpcount;
			head[hcounter].udpcount = n->udpcount;
			head[hcounter].icmpcount = n->icmpcount;
			head[hcounter].unreach = n->unreach;
			head[hcounter].redirect = n->redirect;
			head[hcounter].ttl = n->ttl;
			head[hcounter].udprcv = n->udprcv;
			head[hcounter].pcount = n->pcount;
/*
printf("XXX: pack: pcount %d\n", head[hcounter].pcount);
*/

                        for (z = 0; z < n->pcount; z++) {
                                head[hcounter].dports[z] = n->dports[z];
                                head[hcounter].dpcounts[z] = n->dpcounts[z];
/*
printf("XXX: %d %d\n", head[hcounter].dports[z], head[hcounter].dpcounts[z]);
*/
                        }

			n = n->next;
			hcounter++;
			
		}
	}  
	return(head);
}

static 
int
sort_by_totalicmp(e1,e2)
struct icmpnode *e1, *e2;
{
	if (e1->totalicmp > e2->totalicmp) return -1;
	if (e1->totalicmp < e2->totalicmp) return 1;
	return 0;
} 

/* given a icmplist, sort it, and print it out (count entries max)
 * 
 * note: there are two lists here.  
 *	1. for ip src, sorted by max/total icmp errors
 *	2. for ip src, implicit udp list, sorted by Dave Burns weight notion:
 *		|udp send counts - udp recv counts| * icmp errors.
 *
 * inputs:
 *      fd - file for print output
 *      l - hash list
 *      count - max desired outputs for all filters
 */
void
print_icmplist(FILE *fd, struct icmplist *l, int count)
{
	struct icmpnode *head;
	int i;
	int z;
	int c = l->entries;  /* no of hash elements */

	/* form the hashlist into a packed array
	*/
	head = pack_array(l);
	if (head == NULL) {
		/*
		fprintf(fd, "malloc error in print_tcpicmplist\n");
		*/
		return;
	}
	/* quicksort it
	*/
	qsort((void *)head, c, sizeof(struct icmpnode), sort_by_totalicmp);
 
	/* print it out
	*/
	/*
	fprintf(fd, "#ip:totalerror:tcp:udp:icmp:unreach:redirect:ttl\n");
	*/
	fprintf(fd, "icmperror_list: %u : ", l->entries); 
	for (i = 0; i < c; i++) {
		if ( i >= count) {
			break;
		}
		/*
		 * format: ip address: total icmp errors: tcp: udp :icmp pkts: icmp unreach : redirect: ttl
		 */
		fprintf(fd, "%s:%u:%u:%u:%u:%u:%u:%u:",
			intoa(head[i].ipsrc), 
			head[i].totalicmp, 		/* this is the weight */
			head[i].tcpcount, 
			head[i].udpcount, 
			head[i].icmpcount, 
			head[i].unreach,
			head[i].redirect,
			head[i].ttl);
	}
	fprintf(fd, "\n");

	/* compute the weights for the udp specific view. 
 	 * overload totalicmp to use it as the weight
	 * the general idea: weight is udp packets sent - udp pkts recv * total_icmp_errors
	*/
	for (i = 0; i < c; i++) {
		/* irrelevant if not enough udp packets, or they balance
		*/
		if (head[i].udpcount <= head[i].udprcv) {
			head[i].totalicmp = 0;		/* overload for weight */
		} 
		/* else compute weight: note we overload totalicmp in this case
		*/
		else {
			/* we don't care about ttl count at this point, we put totalicmp value in it 
			*/
				head[i].ttl = head[i].totalicmp;
				head[i].totalicmp = (head[i].udpcount - head[i].udprcv) * head[i].totalicmp;
		}
	}

	/* quicksort it again
	*/
	qsort((void *)head, c, sizeof(struct icmpnode), sort_by_totalicmp);

	fprintf(fd, "udperror_list: %u : ", l->entries); 
	for (i = 0; i < c; i++) {
		if ( i >= count) {
			break;
		}
		/*
		 * format: ip address: weight: udp pkts sent, udp pkts recv: total icmp errors
		 */
		fprintf(fd, "%s:%u:%u:%u:%u:%u:%u:",
			intoa(head[i].ipsrc), 
			head[i].totalicmp, 	/* weight */
			head[i].udpcount, 	/* udp sent pkts */
			head[i].udprcv,		/* udp recv pkts */
			head[i].unreach, 	/* udp unreach errors */
			head[i].icmpcount, 	/* pings sent by src */
			head[i].pcount		/* udp dst port tuple count */
			);

/*
printf("XXX print: %d\n", head[i].pcount);
*/
		for (z = 0; z < head[i].pcount; z++) {
			fprintf(fd, "%u,%u,", head[i].dports[z], head[i].dpcounts[z]);
		}
		fprintf(fd, ":");
	}
	fprintf(fd, "\n");
	free(head);
}          


#ifdef TESTMAIN

static int f = 0;

unsigned long
getRandomWord()
{
	static unsigned long rw;

#define REALLYRANDOM
#ifdef REALLYRANDOM
	if (f == 0) {
		f = open("/dev/urandom", 0);
		if (f < 0) {
			perror("open");
			exit(1);
		}
	}
	read(f, &rw, sizeof(unsigned long));
#else
	rw = rand();
#endif
	return(rw);
}

functionTest()
{
	struct icmplist l;
	unsigned long ipsrc;
	int rc;
	struct icmpnode *n = NULL;

	/* load sample ipkey and counter
	*/
	ipsrc = inet_addr("1.1.1.1");

	init_icmplist(&l);

	n = search_srcudp(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("search failed\n");
	} 
	
	rc = insert_srcudp(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	n = search_srcudp(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("searchNode failed\n");
	} 

	/* add 4 more nodes and dump the list
	*/
	ipsrc = inet_addr("1.1.1.2");
	rc = insert_srcudp(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	ipsrc = inet_addr("1.1.1.3");
	rc = insert_srcudp(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}
	ipsrc = inet_addr("1.1.1.4");
	rc = insert_srcudp(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	ipsrc = inet_addr("1.1.1.5");
	rc = insert_srcudp(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	ipsrc = inet_addr("1.1.1.6");
	rc = insert_srcudp(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	/* now search for a given node, and increment its
	 * value
	 */
	n = search_srcudp(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("searchNode failed\n");
	} 
	else {
		printf("searchNode found node\n");
	}

	/* now search for a previous node, note last
	 * as a simple cache should not be used here
	 */
	ipsrc = inet_addr("1.1.1.4");
	n = search_srcudp(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("searchNode failed\n");
	} 
	else {
		printf("searchNode found node\n");
	}

	dump_hash(&l);

	print_icmplist(stdout, &l, 10);

	destroy_icmplist(&l);
}


/*
 * borrowed from trafshow.
 *
 * A faster replacement for inet_ntoa().
 */

static
char *
intoa(addr)
unsigned int addr;
{
	register char *cp;
	register unsigned int byte;
	register int n;
	static char buf[sizeof(".xxx.xxx.xxx.xxx")];

	addr = ntohl(addr);
	cp = &buf[sizeof buf];
	*--cp = '\0';

	n = 4;
	do {
		byte = addr & 0xff;
		*--cp = byte % 10 + '0';
		byte /= 10;
		if (byte > 0) {
			*--cp = byte % 10 + '0';
			byte /= 10;
			if (byte > 0)
				*--cp = byte + '0';
		}
		*--cp = '.';
		addr >>= 8;
	} while (--n > 0);

	return cp + 1;
}


main()
{
	functionTest();
}
#endif

