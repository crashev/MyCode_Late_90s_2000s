/*
 * hashsort.c - deal with dynamic allocation and sorting
 *	of an arbitrary number of nodes.  This is intended
 *	for use by the top n mechanism.
 */

/*
 * Copyright (c) 2001 Portland State University 
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

#include "hashsort.h"
#include "stats.h"

#include "trigger.h"

/* ehash is better at spreading out into buckets
 * given 8 bytes of ip src and ip dst
#define EHASH
 */
#ifdef EHASH
#include "ehash.c"	/* inlined function */
#else
/* bart massey's sacred hash
*/
#include "barthash.c"	/* inlined function */
#endif

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

char *intoa(unsigned int);

/* called before list can be used
*/
void
initList(struct list *l)
{
	l->count = 0;
	l->last = NULL;
	l->total = 0;
	l->tcpcount = l->udpcount = l->icmpcount = 0;

	/* zero out hash array
	*/
	bzero(&l->hashlist[0], HASHSIZE * sizeof(struct node *));
}

/*
 * search and insert the node if not found by search
 * count as well...
 */
void
searchInsertNode(struct list *l, struct ipkey *ipkey, COUNTER counter) 
{
	struct node *n;
	struct node *m;
	struct node *h;
	int first = 0;
	int mod;
	struct node *prevnode;

	/* search the hash/chain list 
	*/
	/* SCATTER */
#ifdef EHASH
	/*
	mod=ehash((unsigned char *)ipkey, 8) % HASHSIZE;
	*/
	mod=ehash((unsigned char *)ipkey, 12) % HASHSIZE;
	m = n = l->hashlist[mod];
#else
	{
		unsigned int x,y,z,h;
		x = barthash(ipkey->ipsrc);
		y = barthash(ipkey->ipdst);
		z = barthash((ipkey->dstport << 16) ^ ipkey->srcport);
		mod = (x ^ y ^ z) % HASHSIZE;
		m = n = l->hashlist[ mod ];
	}
#endif

	/* walk buckets
	*/
	for (; n != NULL; n = n->hnext) {
		if (n && (
			(n->ipkey.ipdst == ipkey->ipdst) &&
			(n->ipkey.ipsrc == ipkey->ipsrc) &&
			(n->ipkey.dstport == ipkey->dstport) &&
			(n->ipkey.srcport == ipkey->srcport) &&
			(n->ipkey.ipproto == ipkey->ipproto) 
			)) {

			n->counter += counter;
			l->total += counter;
			if (first == 0) {
				os.hs_bfirst++;
			}
			else {
				os.hs_bother++;
			}
			return;	/* search succeeded */
		}
		first++;
	}

	/* commit to insert
	*/
	os.hs_insert++;

	n = (struct node *) malloc(sizeof(struct node)); 

	/* out of space. ouch.
	*/
	if (n == NULL) {
		syslog(LOG_ERR, "topn: malloc out of space, count %d\n", l->count);
		return;
	}

	l->count++; 
	if (ipkey->ipproto == IPPROTO_TCP)
		l->tcpcount++;
	else if (ipkey->ipproto == IPPROTO_UDP)
		l->udpcount++;
	else if (ipkey->ipproto == IPPROTO_ICMP)
		l->icmpcount++;
	n->ipkey = *ipkey; 
	n->counter = counter;
	l->total += counter;

	/* insert the node in the hash list
	*/

	n->hnext = NULL;
		
	if (m == NULL) {
		l->hashlist[mod] = n;
	}
	/* collision, put new node in 1st
	*/
	else {
		os.hs_coll++;
		n->hnext = m;
		l->hashlist[mod] = n;
	}
	return;
}

void
debugList(struct list *l)
{
	dumpHash(l);
}

/*
 * deallocate malloced hash entries
 */
void
destroyList(struct list *l) 
{
	int i;
	struct node *n;
	struct node *prev;

	for ( i = 0; i < HASHSIZE; i++) {
		n = l->hashlist[i];
		while(n != NULL) {
			prev = n;
			n = n->hnext;
			free(prev);
		}
	}
}

dumpHash(struct list *l)
{
	struct node *h;
	int counters[HASHSIZE];
	int i;
	int ls = 0;

	printf("count %d, total %d\n", l->count, l->total);
	bzero(&counters, sizeof(int) * HASHSIZE);

	printf("\nhash dump--------------\nlist count %d\n", l->count);
	for (i = 0; i < HASHSIZE; i++) {
		h = l->hashlist[i];
		while(h != NULL) {
			ls = 1;
			printf("[%d]: ", i);
			printf("ipsrc %s<->", intoa(h->ipkey.ipsrc));
			printf("ipdst %s ", intoa(h->ipkey.ipdst));
			printf("count %d :", h->counter);
			counters[i]++;  /* total buckets in tier used */
			h = h->hnext;
		}
		if (ls) {
			printf(":[%d]\n", i);
		}
		ls = 0;
	}
	printf("\n");
	for (i = 0; i < HASHSIZE; i++) {
		if (counters[i] != 0) {
			printf("counter[%d]: %d buckets\n", i, counters[i]);
		}
	}
	printf("end hash dump--------------\n");
}

/*
 * ok, we have a hash list.  pack it into an array
 * and return the array.
 *
 *	returns:
 *		1 - packed array exists
 *		0 - malloc failure
 */
static
struct node *
packArray(struct list *l)
{
	struct node *head, *n;
	unsigned int c = l->count;	/* count of hash entries allocated */
	int i = 0;
	int hcounter = 0;

	/* create the "array"
	*/
	head = (struct node *) malloc(c * sizeof (struct node));
	if (head == NULL) {
		syslog(LOG_ERR, "topn: packArray out of space, count %d\n",
			l->count);
		return(NULL);
	}

	/* walk the hashlist
	*/
	for ( i = 0; i < HASHSIZE; i++) {
		n = l->hashlist[i];
		/* if n is non-null, we have a node
		 * in use, therefore put it in the array,
		 * and increment hcounter
		 */
		if (n != NULL) {
			os.hs_ecount++;	/* number of full first buckets */
		}
		while (n != NULL) {
			head[hcounter].ipkey = n->ipkey; 
			head[hcounter].counter = n->counter;
			hcounter++;
			/* walk to end of linked list repeating previous
			 * operation
			 */
			n = n->hnext;
		}
	}
	return(head);
}

static int      
sortbycounter(e1, e2)
struct node *e1, *e2;
{
        if (e1->counter > e2->counter) return -1;
        if (e1->counter < e2->counter) return 1;
        return 0;
}       

/*
 * borrowed from trafshow.
 *
 * A faster replacement for inet_ntoa().
 */

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

static
char *
prototostring(int p)
{
	if (p == IPPROTO_TCP)
		return("tcp");
	else if (p == IPPROTO_UDP)
		return("udp");
	else if (p == IPPROTO_ICMP)
		return("icmp");
	return (NULL);
}

/*
 * given a list, sort it,
 * and print topN results.
 * if tcp flag, print top N tcp.
 * if udp flag, print top N udp,
 *
 * inputs:
 *	fd - file for print output
 *	l - hash array
 *	anyflag/tcpflag/udpflags - flags to indicates various filter outputs
 *	count - max desired outputs for all filters
 */
printList(FILE *fd, struct list *l, int anyflag, int tcpflag, int udpflag, int icmpflag, int count) 
{
	struct node *head;
	char *ps;
	char buf[1024];
	char b1[1024], b2[1024];
	int i;
	unsigned long subtotal = 0;
	unsigned long total = l->total;
	int protohasports = 0;
	int tcpcount, udpcount, icmpcount;
	static int trigger_state = FALSE;

	int c = l->count;	/* no of hash elements */

	tcpcount = udpcount = icmpcount = 0;

	/* form the hashlist into a packed array
	*/
	head = packArray(l);
	if (head == NULL) {
		/*
		fprintf(fd, "malloc error in printList\n");
		*/
		return;
	}

	/* quicksort it
	*/
	qsort((void *)head, c, sizeof (struct node), sortbycounter);

	/* print it out
	*/
	if (anyflag) {
		/* produce topn_ip
		*/
		fprintf(fd, "topn_ip : %u : ", c); 
		/* walk thru all elements of hash list
		*/
		for ( i = 0; i < c; i++) {
			/* if i reaches config count
			 * quit
			 */
			if (i >= count) {
				break;
			}
			/*
			subtotal += head[i].counter;
			*/
			/* produce a string for the proto type
			 * if possible, else a number.
			 */
			ps = prototostring(head[i].ipkey.ipproto);
			if (ps == NULL) {
				sprintf(buf, "%d", head[i].ipkey.ipproto);
				ps = &buf[0];
			}
			else {
				protohasports = 1;
			}
			/* intoa can only do one ip address at a time,
			 * therefore we must buffer one
			 */
			sprintf(b1, "%s", intoa(head[i].ipkey.ipdst));
			/* if we have non-zero ports include them,
			 * else drop them.
			 */
			if (protohasports) {
				fprintf(fd,"%s.%d->%s.%d(%s): %u : ",
					intoa(head[i].ipkey.ipsrc),
					head[i].ipkey.srcport,
					b1,
					head[i].ipkey.dstport,
					ps,
					head[i].counter);
			}
			else {
				fprintf(fd,"%s->%s(%s):%u : ",
					intoa(head[i].ipkey.ipsrc),
					b1,
					ps,
					head[i].counter);
			}
		}
		/*
		fprintf(fd, " xtra: %u\n", total - subtotal);
		*/
		fprintf(fd, "\n");
	}
	if (tcpflag) {
		fprintf(fd, "topn_tcp : %u : ", l->tcpcount); 
		for ( i = 0; i < c; i++) {
			if (head[i].ipkey.ipproto != IPPROTO_TCP)
				continue;
			tcpcount++;
			if (tcpcount > count) {
				break;
			}
			/*
			subtotal += head[i].counter;
			*/
			sprintf(b1, "%s", intoa(head[i].ipkey.ipdst));
			fprintf(fd, "%s.%d->%s.%d: %u : ",
				intoa(head[i].ipkey.ipsrc),
				head[i].ipkey.srcport,
				b1,
				head[i].ipkey.dstport,
				head[i].counter);
		}
		/*
		fprintf(fd, " xtra: %u\n", total - subtotal);
		*/
		fprintf(fd, "\n");
	}
	if (udpflag) {
		fprintf(fd, "topn_udp : %u : ", l->udpcount); 
		for ( i = 0; i < c; i++) {
			if (head[i].ipkey.ipproto != IPPROTO_UDP) {
				continue;
			}
			udpcount++;
			if (udpcount > count) {
				break;
			}
			/*
			subtotal += head[i].counter;
			*/
			sprintf(b1, "%s", intoa(head[i].ipkey.ipdst));
			fprintf(fd, "%s.%d->%s.%d: %u : ",
				intoa(head[i].ipkey.ipsrc),
				head[i].ipkey.srcport,
				b1,
				head[i].ipkey.dstport,
				head[i].counter);
		}
		/*
		fprintf(fd, " xtra: %u\n", total - subtotal);
		*/
		fprintf(fd, "\n");
	}
	if (icmpflag) {
		fprintf(fd, "topn_icmp : %u : ", l->icmpcount); 
		for ( i = 0; i < c; i++) {
			if (head[i].ipkey.ipproto != IPPROTO_ICMP)
				continue;
			icmpcount++;
			if (icmpcount > count) {
				break;
			}
			/* ip dst put in string b1 */
			sprintf(b1, "%s", intoa(head[i].ipkey.ipdst));
			/* ipsrc.majorcode->ipdst.minorcode 
			 * which needs to be pointed out in the documentation
			*/
			fprintf(fd, "%s.%d->%s.%d: %u : ",
				intoa(head[i].ipkey.ipsrc),
				head[i].ipkey.srcport,
				b1,
				head[i].ipkey.dstport,
				head[i].counter);
		}
		/*
		fprintf(fd, " xtra: %u\n", total - subtotal);
		*/
		fprintf(fd, "\n");
	}
	free(head);

	fprintf(fd, "#hs_stat: 0: 0: bfirst: bother: bmiss: coll: insert: fcount:\n");
        fprintf(fd, "hs_stat: %u: %u: %u: %u: %u: %u: %u: %u:\n",
                        os.hs_chits, 	/* dead */
			os.hs_cmiss, 	/* dead */
			os.hs_bfirst,
                        os.hs_bother, 
			os.hs_bmiss, 	/* dead */
			os.hs_coll,
                        os.hs_insert, 
			os.hs_ecount  
			);

	/* TBD. make this dynamic
	 *	l->tcpcount, l->udpcount, l->icmpcount.
	 * 	If any of these is greater than the value, then
	 *	set a dynamic bpf.  do this as follows:
	 *	0 to 3 of them may be greater than the value.
	 *	if only 1, set bpf to that one.
	 *	if more than 1, make bpf "ip".
	*/
	if(trigger_cap[TOPNIP].configured == TRUE) {
                char fname[1024];
		char *e = NULL;
		char *bpfexpr;
		int totalcount = l->count/30;		/* convert to flows per sec */
		int tcount = l->tcpcount/30;
		int ucount = l->udpcount/30;
		int icount = l->icmpcount/30;
		int tv = trigger_cap[TOPNIP].N_value;

		/* check trigger_on possibility
		*/
                if( (trigger_state == FALSE) && (totalcount >= tv))  {

			/* first figure out which bpf expression to use
			 * can we narrow it?
			*/
			bpfexpr = "ip";		/* this is the default */
			if ((icount > tv) && ((tcount < tv) && (ucount < tv))) {
				bpfexpr = "icmp";
			}
			else if ((ucount > tv) && ((tcount < tv) && (icount < tv))) {
				bpfexpr = "udp";
			}
			else if ((tcount > tv) && ((ucount < tv) && (icount < tv))) {
				bpfexpr = "tcp";
			}
			e = trigger_dynbpf_compile(TOPNIP, bpfexpr);
			if (e != NULL) {
				fprintf(fd, "elog: topn_ip dynamic trigger failure on %s\n", bpfexpr);
				return;
			}
		
			/* turn the trigger on to enable packet capture
			*/
                	memset(fname, 0, sizeof(fname));
                        e = trigger_on(TOPNIP,fname);
			if (e != NULL) {
                                fprintf(fd,"elog: topn_ip trigger failed: %s.\n", e);
				trigger_state = FALSE;
                        }
                        else {
                                fprintf(fd, "elog: topn_ip trigger is on, threshold is %d, packets are in %s, bpf %s.\n", 
					totalcount, fname, bpfexpr);
				trigger_state = TRUE;
                        }
                } 
                else {
			if (trigger_state == TRUE) {
				trigger_off(TOPNIP);
				fprintf(fd, "elog: topn_ip trigger is OFF, threshold is %d\n", totalcount);
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
	struct list l;
	COUNTER c;
	struct ipkey ik;
	int rc;
	struct node *n = NULL;

	/* load sample ipkey and counter
	*/
	ik.ipsrc = inet_addr("1.1.1.1");
	ik.ipdst = inet_addr("1.1.1.2");
	ik.srcport = ik.dstport = 0;
	ik.ipproto = IPPROTO_TCP;
	c = 1;	

	initList(&l);

	searchInsertNode(&l, &ik, c);
	
	searchInsertNode(&l, &ik, c);

	searchInsertNode(&l, &ik, c);

	/* add 4 more nodes and dump the list
	*/
	ik.ipsrc = inet_addr("1.1.1.2");
	ik.ipdst = inet_addr("1.1.1.3");
	c = 2;
	searchInsertNode(&l, &ik, c);

	ik.ipsrc = inet_addr("1.1.1.3");
	ik.ipdst = inet_addr("1.1.1.4");
	c = 3;
	ik.ipproto = IPPROTO_UDP;
	searchInsertNode(&l, &ik, c);

	ik.ipsrc = inet_addr("1.1.1.4");
	ik.ipdst = inet_addr("1.1.1.5");
	ik.ipproto = 1;
	c = 4;

	searchInsertNode(&l, &ik, c);

	ik.ipsrc = inet_addr("1.1.1.6");
	ik.ipdst = inet_addr("1.1.1.7");
	c = 5;
	searchInsertNode(&l, &ik, c);

	/* now search for a given node, and increment its
	 * value
	 */
	c = 5;
	searchInsertNode(&l, &ik, c);

	/* now search for a previous node, note last
	 * as a simple cache should not be used here
	 */
	ik.ipsrc = inet_addr("1.1.1.4");
	ik.ipdst = inet_addr("1.1.1.5");
	c = 1;
	searchInsertNode(&l, &ik, c);

	dumpHash(&l);
	printList(stdout, &l, 1, 1, 1, 1, 5);
	destroyList(&l);
}

randomTest()
{
	struct list l;
	struct ipkey ik;
	unsigned long key1, key2;
	COUNTER counter1;
	unsigned long k,c;
	struct node *n;
	int i;
	int rc;

	initList(&l);
	
	for ( i = 0; i < 10000; i++) {
		key1 = getRandomWord();
		key2 = getRandomWord();
		counter1 = getRandomWord();
		ik.ipsrc = key1;
		ik.ipdst = key2;

		printf("[i=%d], key1=%lx, key2=%lx, counter=%lx\n", i, key1, key2, counter1);

		searchInsertNode(&l, &ik, counter1);
		searchInsertNode(&l, &ik, counter1);
	}
	
	printf("node count %d\n", l.count);
	dumpHash(&l);
	printList(stdout, &l, 1, 1, 1, 1, 5);

	destroyList(&l);
}

struct ourstats os;

main()
{
	/*
	functionTest();
	*/
	randomTest();
}
#endif

