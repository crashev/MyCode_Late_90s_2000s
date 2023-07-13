/*
 * hashsyn.c - count tcp syns with mention of rst/fin on a per ip src basis. 
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

#include <pcap.h>

/* use filter.h which is more general because we access
 * topn_syn in here
 */

/* 
#include "hashsyn.h"
*/
#include "filter.h"
#include "trigger.h"
#include "ehash.c"	/* inlined function */

static void  __inline
storePort( struct synnode *n, int dport);

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
void init_synlist (struct synlist *l) 
{
	l->entries = 0;
	l->last = NULL;
	bzero (&l->hashlist[0], THSIZE * sizeof(struct synnode *));
}

/*
 * search_srcsyn - hash list lookup
 *
 * If found we increment the counters.  syn/fin/rst are mutually exclusive.
 *
 * if icmpflag is set, we have an icmp packet, not a tcp packet.  
 * in that case, ipofinterest is the dst address we wish to change with an icmp error.
 *
 * if synflag then packet is coming from ipofinterest.  if fin or reset flags,
 * then packet is being sent to ipofinterest.
 *
 *	inputs:
 *		l - a list pointer
 *		ipsrc as logical key
 *	returns:
 *		NULL - didn't find it
 *		struct synnode *n 
 */ 
struct synnode *
search_srcsyn(struct synlist *l, unsigned long ipofinterest, unsigned long ipother, 
int synflag, int finflag, int rstflag, int saflag, int dport, int icmpflag)
{
	struct synnode *n;
	int mod;
	int cflag = 0;
	struct synnode *f = NULL;

	if (l ->entries == 0) 
		return (NULL);

	/* search the hash/chain list on ipofinterest
	 * note this lookup counts against ipofinterest
	*/
	n = l->hashlist[mod=(ehash((unsigned char *)&ipofinterest, 4) % THSIZE)];
	for (; n!= NULL ; n=n->next) {
		if (n && ( n->ipsrc == ipofinterest)) {
			l->last = n;
			if (synflag) {
				n->syncount++;
				/* if a syn with an ACK coming from an ip_src, then
				 * we count as syn/ack as well
				 */
				if (saflag) {
					n->sacount++;
				}
			}
			else if (finflag) {
				n->fincount++;	/* fin returned to this ip source */
				n->tcpto++;	/* pkts to this ip src */
			}
			else if (rstflag) {
				n->rstcount++;	/* reset returned to this ip source */
				n->tcpto++;	/* pkts to this ip src */
			}
			if (!icmpflag && !finflag && !rstflag) {
				n->tcpfrom++;	/* tcp pkts from this ip src */
				if (dport != -1) {
					storePort(n, dport);
				}
			}
			else {
				n->icmpcount++;	/* icmp errors sent to this ip src */
				return(n);
			}
			break;
		}
	}

	f = n;

	/* search the hash/chain list on ipother, just count it if found
	*/
	n = l->hashlist[mod=(ehash((unsigned char *)&ipother, 4) % THSIZE)];
	for (; n!= NULL ; n=n->next) {
		if (n && ( n->ipsrc == ipother)) {
			if (finflag) {
				n->tcpfrom++;	/* pkts from this ip src */
			}
			else if (rstflag) {
				n->tcpfrom++;	/* pkts from this ip src */
			}
			else if (!icmpflag) {
				n->tcpto++;	/* pkts to this ip src */
			}
			break;
		}
	}

	/* did we find ipofinterest or not 
	*/ 
	return(f);	
}

/*
 *      insert_srcsyn
 *
 *      search_srcsyn succeeds or fails. if it succeeds
 *      we don't call insert_srcsyn.  if it fails, we
 *      call this function to create a new hash list
 *      entry and to set the initial counter values.
 *
 *      returns:
 *              1 - ok
 *              0 - malloc failure
 */

int
insert_srcsyn (struct synlist *l, unsigned long ipsrc, int synflag, int finflag, int rstflag, int saflag, int dport)
{ 
	struct synnode *n;
	struct synnode *h, *prev;
	unsigned int mod;
	int i;

	n = (struct synnode *) malloc(sizeof(struct synnode));
	n->syncount = n->sacount = n->fincount = n->rstcount = 0L;
	if (n == NULL) {
		syslog(LOG_ERR, "malloc failure in insert_srcsyn, entry/count %d",
			l->entries);
		return (0);
	}
	l->entries += 1;
	n->ipsrc = ipsrc;
	n->pcount = n->tcpto = n->tcpfrom = n->icmpcount = 0;
	
	for ( i = 0; i < MAXPORTS; i++) {
		n->dports[i] = -1;
		n->dpcounts[i] = 0;
	}
	if (synflag) {
		n->syncount = 1;
		n->tcpfrom = 1;
		if (saflag) {
			n->sacount = 1;
		}
		storePort(n, dport);	/* only on syns which is arbitrary */
	}
	else if (finflag) {
		n->fincount = 1;
		n->tcpto = 1;
	}
	else if (rstflag) {
		n->rstcount = 1;
		n->tcpto = 1;
	}
	l->last = n;

	/* insert the synnode in the hash list
	*/
	n->next = NULL;
	h = l->hashlist[mod=(ehash((unsigned char *)&ipsrc, 4) % THSIZE)];
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
storePort(struct synnode *n, int dport)
{
	int i;

	for ( i = 0; i < MAXPORTS; i++) {
		/* have not seen the port and we have room,
		 * therefore store it and increment counter and return
		 */
		if (n->dports[i] == -1) {
			n->dports[i] = dport;
			n->dpcounts[i] = 1;
			n->pcount++;
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
destroy_synlist(struct synlist *l) 
{
	int i;
	struct synnode *n;
	struct synnode *prev;
	for (i =0; i< THSIZE; i++) {
		n = l->hashlist[i];
		while(n != NULL) {
			prev = n;
			n = n->next;
			free(prev);
		}
	}
}

#ifdef TESTMAIN
static
dump_hash(struct synlist *l) {
	struct synnode *h;
	int counters[THSIZE];
	int i;
	int ls = 0;
	printf("No. of entries %d\n", l->entries);
	bzero(&counters, sizeof(int)*THSIZE);
	printf("\nhashdump---------\nlist entries %d\n",l->entries);
	for (i=0;i<THSIZE;i++) {
		h = l->hashlist[i];
		while(h != NULL) {
			ls = 1;
			printf("%d:", i);
			printf("ip src %x", intoa(h->ipsrc));
			printf(":s/f/r: %d %d %d", h->syncount,
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
	for (i = 0; i<THSIZE; i++) {
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
struct synnode *
pack_array(struct synlist *l)
{
	struct synnode *head, *n;
	unsigned int c = l->entries;  /* count of hash entries allocated */
	int i = 0;
	int z;
	int hcounter = 0;
	WEIGHTCOUNTER t1;
	head = (struct synnode *) malloc(c * sizeof (struct synnode));
	if (head == NULL) {
		syslog(LOG_ERR, "malloc failure in syn/pack_array, count %d",
			l->entries);
		return (NULL);
	}
	/* walk the hashlist
	*/
	for (i =0 ; i<THSIZE; i++) {
		n = l->hashlist[i];
		/* if n is non-nulln use, therefore put it in the array,
		* and increment hcounter
		*/
		while (n != NULL) {
			head[hcounter].ipsrc    = n->ipsrc;
			head[hcounter].syncount = n->syncount;
			head[hcounter].sacount  = n->sacount;
			head[hcounter].fincount = n->fincount;
			head[hcounter].rstcount = n->rstcount;
			head[hcounter].tcpfrom  = n->tcpfrom;
			head[hcounter].tcpto    = n->tcpto;
			head[hcounter].icmpcount = n->icmpcount;
			head[hcounter].pcount   = n->pcount;

			t1 = n->tcpfrom + n->tcpto;
			if (t1 == 0) {
				head[hcounter].ww = head[hcounter].work = 0;
			}
			else {
				head[hcounter].ww = head[hcounter].work = 
					(n->syncount + n->fincount + n->rstcount) / t1;                        
				/* sort with w=100% (w varies from 0..1 here) by syn count by simply adding the syncount back in.
				 * it would be possible to do this within all weights by scaling
				 * the syncount by an absolute max. syns per second but it isn't
				 * clear as to whether or not that is needed.
				*/
				if (head[hcounter].work >= 1) {
					head[hcounter].work += n->syncount;
				}
			}

			for (z = 0; z < n->pcount; z++) {
				head[hcounter].dports[z] = n->dports[z]; 
				head[hcounter].dpcounts[z] = n->dpcounts[z];
			}
			n = n->next;
			hcounter++;
		}
	}  
	return(head);
}

static
int
sort_by_syncount(e1,e2)
struct synnode *e1, *e2;
{
        if (e1->syncount > e2->syncount) return -1;
        if (e1->syncount < e2->syncount) return 1;
        return 0;
}

static
int
sort_by_work(e1,e2)
struct synnode *e1, *e2;
{
        if (e1->work > e2->work) return -1;
        if (e1->work < e2->work) return 1;
        return 0;
}


/* given a synlist, sort it, and print it out (count entries max)
 *
 * inputs:
 *      fd - file for print output
 *      l - hash list
 *      count - max desired outputs for all filters
 */
void
print_synlist(FILE *fd, struct synlist *l, int count, int wflag, 
char *wormfile, int ipaflag, unsigned long usnet, unsigned long usmask)
{
	extern struct topn_syn topn_syn;
	struct synnode *head;
	int i;
	int z;
	int c = l->entries;  /* no of hash elements */
	int us, them, wcount;
	static int trigger_state = FALSE;				/* remember whether the trigger is on/off */
	FILE *wfd = NULL;
#ifdef SYNDUMP
	FILE *sfd = NULL;
#endif
	
	if (wflag) {
		wfd = fopen(wormfile, "w");
		if (wfd == NULL) {
			syslog(LOG_ERR, "could not open %s:(%m)", wormfile);  
			exit(1);
		}
	}
	us = them = wcount = 0;

	/* form the hashlist into a packed array
	*/
	head = pack_array(l);
	if (head == NULL) {
		return;
	}
	/* quicksort it either by syn count or by adjusted worms at the top syncount
	*/

	if (topn_syn.sortbysyn) {
		qsort((void *)head, c, sizeof(struct synnode), sort_by_syncount);
	}
	else {
		qsort((void *)head, c, sizeof(struct synnode), sort_by_work);
	}

	/* 
	 * 2nd list: 
	 * syn_list_ports: %u : ip : Nports: 
	 */
 
	/* print it out
	 * the new raw format is as follows:
	 * ip: syn: fin: rst: tcpfrom: tcpto: icmpcount: Nports: port,count,port,count,port,count
	*/
	fprintf(fd, "syn_list: %u : ", l->entries); 
	for (i = 0; i < c; i++) {
		if (wfd == NULL) {
			if ( i >= count) {
				break;
			}
			fprintf(fd, "%s:%u:%u:%u:%u:%u:%u:%u:%u:",
				intoa(head[i].ipsrc), 
				head[i].syncount, 
				head[i].sacount,
				head[i].fincount,
				head[i].rstcount,
				head[i].tcpfrom,
				head[i].tcpto,
				head[i].icmpcount, 
				head[i].pcount);

			for (z = 0; z < head[i].pcount; z++) {
				fprintf(fd, "%u,%u,", head[i].dports[z], head[i].dpcounts[z]);
			}
			fprintf(fd, ":");
			continue;
		}
		/* do it this way if the worm file is desired
		*/
		if ( i < count) {
			/* print to mon.lite
			*/
			fprintf(fd, "%s:%u:%u:%u:%u:%u:%u:%u:%u:",
				intoa(head[i].ipsrc), 
				head[i].syncount, 
				head[i].sacount, 
				head[i].fincount,
				head[i].rstcount,
				head[i].tcpfrom,
				head[i].tcpto,
				head[i].icmpcount, 
				head[i].pcount);

			for (z = 0; z < head[i].pcount; z++) {
				fprintf(fd, "%u,%u,", head[i].dports[z], head[i].dpcounts[z]);
			}
			fprintf(fd, ":");
		}
		if (head[i].fincount > head[i].syncount)
			continue;
/*
 * from looking at the syndump.txt file (all syn list entries sorted)
 * it appears that 20-40 is a reasonable range.  
#define WORMDIFF	20
 */
	
		if ((head[i].syncount - head[i].fincount) > topn_syn.wormdiff) {
			/* if the true work weight is greater than the config pre-filter
			 * weight, then we count things in tworm
			 */
			if ((int)(head[i].ww * 100) >= topn_syn.weight) { 
				wcount++;
				if (ipaflag) {
					if (((head[i].ipsrc & usmask) == usnet)) {
						us++;
					}
					else  { 
						them++;
					}
				}
			}
			/* print to wormfile
			*/
			fprintf(wfd, "%s:%u:%u:%u:%u:%u:%u:%u:%u:",
				intoa(head[i].ipsrc), 
				head[i].syncount, 
				head[i].sacount, 
				head[i].fincount,
				head[i].rstcount,
				head[i].tcpfrom,
				head[i].tcpto,
				head[i].icmpcount, 
				head[i].pcount);
			for (z = 0; z < head[i].pcount; z++) {
				fprintf(wfd, "%u,%u,", head[i].dports[z], head[i].dpcounts[z]);
			}
			fprintf(wfd, ":\n");
		}
	}

#ifdef SYNDUMP
	/* this is used in batch mode to study what has showed up in the syn list
	*/
	sfd = fopen("/tmp/syndump.txt", "w");
	if (sfd == NULL) {
		syslog(LOG_ERR, "could not open %s:(%m)", "/tmp/syndump.txt");  
		exit(1);
	}
	/* write all the syn info to a file -- all entries
	*/
	for (i = 0; i < l->entries; i++) {
			/* as this list is sorted, if we hit zero syns,
			 * simply stop
			 */
			if (head[i].syncount == 0) {
				break;
			}
			fprintf(sfd, "%s:%u:%u:%u:%u:%u:%u:%u:%u:",
				intoa(head[i].ipsrc), 
				head[i].syncount, 
				head[i].sacount, 
				head[i].fincount,
				head[i].rstcount,
				head[i].tcpfrom,
				head[i].tcpto,
				head[i].icmpcount, 
				head[i].pcount);
			for (z = 0; z < head[i].pcount; z++) {
				fprintf(sfd, "%u,%u,", head[i].dports[z], head[i].dpcounts[z]);
			}
			fprintf(sfd, ":\n");
	}
	fclose(sfd);
#endif

	fprintf(fd, "\n");
	if (ipaflag)
		fprintf(fd, "tworm: %d: %d: %d:\n", wcount, us, them);
	if (wfd)
		fclose(wfd);

	free(head);

        /* check for trigger condition
	 * we must keep internal state to keep from calling trigger off at
	 * inappropriate times.  
	 * we trigger if the total count is greater than the threshold.
        */
        if (trigger_cap[TWORM].configured) {
                if(wcount >= trigger_cap[TWORM].N_value) {
                        char fname[1024];
			char *e;

			/* if the trigger is not on, turn it on
			*/ 
			if (trigger_state == FALSE) {
                        	memset(fname, 0, sizeof(fname));
                        	e = trigger_on(TWORM, fname);

				if (e != NULL) {
					fprintf(fd,"elog: tworm trigger failed: %s\n", e);
					trigger_state = FALSE;	/* local static variable */
				}
				else {
					fprintf(fd, 
					"elog: tworm trigger on, current worm count is %d, packets are in %s\n", wcount,fname);
					trigger_state = TRUE;
				}
			}
                }
                else {
			if (trigger_state == TRUE) {
				trigger_off(TWORM);
				fprintf(fd, 
				"elog: tworm trigger OFF, current worm count is %d\n", wcount);
				trigger_state = FALSE;
			}
                }
        }
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
	struct synlist l;
	unsigned long ipsrc;
	int rc;
	struct synnode *n = NULL;

	/* load sample ipkey and counter
	*/
	ipsrc = inet_addr("1.1.1.1");

	init_synlist(&l);

	n = search_srcsyn(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("search failed\n");
	} 
	
	rc = insert_srcsyn(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	n = search_srcsyn(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("searchNode failed\n");
	} 

	/* add 4 more nodes and dump the list
	*/
	ipsrc = inet_addr("1.1.1.2");
	rc = insert_srcsyn(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	ipsrc = inet_addr("1.1.1.3");
	rc = insert_srcsyn(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}
	ipsrc = inet_addr("1.1.1.4");
	rc = insert_srcsyn(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	ipsrc = inet_addr("1.1.1.5");
	rc = insert_srcsyn(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	ipsrc = inet_addr("1.1.1.6");
	rc = insert_srcsyn(&l, ipsrc, 1, 0, 0);
	if (rc == 0) {
		printf("insertNode failed\n");
	}

	/* now search for a given node, and increment its
	 * value
	 */
	n = search_srcsyn(&l, ipsrc, 1, 0, 0);
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
	n = search_srcsyn(&l, ipsrc, 1, 0, 0);
	if ( n == NULL ) {
		printf("searchNode failed\n");
	} 
	else {
		printf("searchNode found node\n");
	}

	dump_hash(&l);

	print_synlist(stdout, &l, 10);

	destroy_synlist(&l);
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


