/*
 * hashscan.c - hashing done by topn ip and port scanning code.
 *
 * TBD. optimize.
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


#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#include "ehash.c"

#include "hashscan.h"

/*These 3 functions extract key information from the ipkey
 *and if there is more than one field, convolves them together
 */
unsigned long hashSrcDst(struct sipkey* ipkey){

	return (ipkey->ipsrc ^ ehash((unsigned char*)&(ipkey->ipdst), 4));
}
unsigned long hashSrc(struct sipkey* ipkey){
	return (ipkey->ipsrc);
}
unsigned long hashSrcPort(struct sipkey* ipkey){
        return (ipkey->ipsrc ^ 
		((ehash((unsigned char*)&(ipkey->dstport), 2)<<8) + 
		ehash((unsigned char*)&(ipkey->ipproto), 2)));
}

/* called before list can be used
*/
void
initScanList(struct slist *l)
{
	l->count = 0;
	l->last = NULL;
	l->total = 0;
	l->tcpcount = l->udpcount = l->icmpcount = 0;

	/* zero out hash array
	*/
	bzero(&l->hashlist[0], SHASHSIZE * sizeof(struct snode *));
}

/*This is exactly like searchNode except that it takes a 
 *search key function-pointer as a parameter so it can know
 *on what criteria to hash
 * BM
 */
struct snode *
searchKeyNode(struct slist *l, struct sipkey *ipkey, 
	   COUNTER counter,  func searchKeyFunc)
{
	struct snode *n;
	unsigned long ipdst = ipkey->ipdst;
	int mod;
	unsigned long searchKey;

	if (l->count == 0)
		return(NULL);

	if (l->last && (searchKeyFunc(&(l->last->ipkey)) == (searchKeyFunc(ipkey))
			)) {

		l->last->counter += counter;
		l->total += counter;
#ifdef OFF
   		if(0){
			printf("Duplicate Last insert with count: %d   ",l->last->counter);
			dumpIpkey(ipkey);
			printf("\n");
		}
#endif
		return(l->last);
	}

	searchKey = searchKeyFunc(ipkey);
	/* search the hash/chain list 
	*/
	n = l->hashlist[mod=(ehash((unsigned char *)&searchKey, 4) % SHASHSIZE)];

	for (; n != NULL; n = n->hnext) {
		if (n && (
			searchKeyFunc(&(n->ipkey)) == searchKeyFunc(ipkey)
			)) {

			l->last = n;  
			l->last->counter += counter;
			l->total += counter;
#ifdef OFF
			if(0){
				printf("Duplicate insert at: %d  count: %d  " ,mod,n->counter );
                	
				dumpIpkey(ipkey);
                		printf("\n");
			}
#endif
			return(n);
		}
	}
	return(NULL);
}

/*This is just used for debugging topn_scans*/
char *
intoa2(unsigned int addr)
{
	extern char *intoa();

	return(intoa(addr)); 
}

/* This is just like insertNode except that it takes an insert key
 *function AND a statistics structure as arguments
 * BM.
 */
int
insertKeyNode(struct slist *l, struct sipkey *ipkey, 
	   COUNTER counter, func searchKeyFunc, struct hashStats *stats)
{
	struct snode *prevnode;
	struct snode *n;
	extern int debug;

	n = (struct snode *) malloc(sizeof(struct snode)); 

	if (n == NULL) {
		syslog(LOG_ERR, "topn: malloc out of space, count %d\n",
			l->count);
		return(0);
	}

	/*
	bzero(n, sizeof(struct node));
	*/
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
	l->last = n;

	/* insert the node in the hash list
	*/
	{
		struct snode *h, *prev;
		unsigned int mod;
		unsigned long ipdst = ipkey->ipdst;
		unsigned long searchKey;
		n->hnext = NULL;
		searchKey = searchKeyFunc(ipkey);	
		h = l->hashlist[mod=(ehash((unsigned char *)&searchKey, 
			sizeof(unsigned long)) % SHASHSIZE)]; 

		if(stats){stats->misses +=1;}
		if (h == NULL) {
			l->hashlist[mod] = n;
			
			if(debug){
				printf("Insert Hashed to empty cell at: %d  count: %d  ",
					mod,n->counter);
				dumpIpkey(ipkey);
				printf(" \n");
			}
		}
		/* collision, put new node in 1st
		*/
		else {
			if(stats){stats->collisions += 1;}	
			n->hnext = h;
			l->hashlist[mod] = n;
			if(debug){
				printf("Insert Hashed & insert to address: %d  count: %d  ",
					mod,n->counter);	
		
				dumpIpkey(ipkey);
				printf(" \n");
			}
		}
	}
	return(1);
}

/* This function takes the input list and reinserts the data into
 * the output list based on SRCIP.  This allows us to create a digest
 * of counts of unique connections per SRCIP
 */
void processTopNScans(struct slist *input, struct slist *output, 
			struct hashStats *stats)
{
        struct snode *m, *n;
	int i = 0;
	int rc;
	int j = 0;
	extern int debug;

	if(debug){
		printf("\n\nHashing down from SRC/XXX to SRC\n\n");
	}
	if(stats) {
		stats->maxChain = 0;
	}
	/* walk the input hash list
	*/
  	for ( i = 0; i < SHASHSIZE; i++) {
		j= 0;
		n = (input->hashlist)[i];
		while (n != NULL) {
			j++;
			/* search the output list for the ip src
			*/
			m = searchKeyNode(output, 
				    &n->ipkey, 1, hashSrc);
			/* if not found, insert it
			*/
		  	if (m == NULL) {
		    		if(debug){
					printf("Process TOPN SCANS hashdown insert");
					dumpIpkey(&n->ipkey);
					printf("\n");
				}
				/* note counting here ... 
				*/
		    		rc = insertKeyNode(output,  
				    &n->ipkey, 1, hashSrc , NULL);
				/* 
		    		if (rc == 0) {
		     			fprintf(stderr, "malloc failure - out of memory in topn_ip\n");
		    		} 
				*/
		  	}
#ifdef MOREDEBUG
			else {
				if(debug){
					printf("Process TOPN SCANS hashdown duplicate");
					dumpIpkey(&n->ipkey);
					printf("\n");
				}
			}
#endif
			n = n->hnext;
		}
		if(stats){	
			if (j > stats->maxChain){
				stats->maxChain = j;
			}
		}
	}
}

static
void
debugScanList(struct slist *l)
{
	dumpHash(l);
}

/*
 * deallocate malloced hash entries
 */
void
destroyScanList(struct slist *l) 
{
	int i;
	struct snode *n;
	struct snode *prev;

	for ( i = 0; i < SHASHSIZE; i++) {
		n = l->hashlist[i];
		while(n != NULL) {
			prev = n;
			n = n->hnext;
			free(prev);
		}
	}
}

static
dumpScanHash(struct slist *l)
{
	struct snode *h;
	int counters[SHASHSIZE];
	int i;
	int ls = 0;

	printf("count %d, total %d\n", l->count, l->total);
	bzero(&counters, sizeof(int) * SHASHSIZE);

	printf("\nhash dump------%d--------\nlist count %d\n" ,SHASHSIZE,l->count);
	for (i = 0; i <SHASHSIZE ; i++) {
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
	for (i = 0; i < SHASHSIZE; i++) {
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
struct snode *
packArray(struct slist *l)
{
	struct snode *head, *n;
	unsigned int c = l->count;	/* count of hash entries allocated */
	int i = 0;
	int hcounter=0;

	/* create the "array"
	*/
	head = (struct snode *) malloc(c * sizeof (struct snode));
	/* TBD. syslog this
	*/
	if (head == NULL) {
		/*
		fprintf(stderr, "packArray, malloc failed\n");
		*/
		syslog(LOG_ERR, "topn: packArray out of space, count %d\n",
			l->count);
		return(NULL);
	}

	/* walk the hashlist
	*/
	for ( i = 0; i < SHASHSIZE; i++) {
		n = l->hashlist[i];
		/* if n is non-null, we have a node
		 * in use, therefore put it in the array,
		 * and increment hcounter
		 */
		while (n != NULL) {
			head[hcounter].ipkey = n->ipkey; 
			head[hcounter].counter = n->counter;
			/* walk to end of linked list repeating previous
			 * operation
			 */
			n = n->hnext;
			hcounter++;
		}
	}
	return(head);
}

static int      
sortbycounter(e1, e2)
struct snode *e1, *e2;
{
        if (e1->counter > e2->counter) return -1;
        if (e1->counter < e2->counter) return 1;
        return 0;
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
 * This is just like printList except that it takes a label prefix
 * as an additional last argument and it pre-appends this prefix to
 * all labeled output.  EG.  If the prefix was "small_" then:
 * topn_ip -> "small_topn_ip"
 * topn_tcp -> "small_topn_tcp"
 * etc...
 */
printSingleIPList(FILE *fd, struct slist *l, int anyflag, int tcpflag, int udpflag, int icmpflag, 
int count, char* listname) 
{
	struct snode *head;
	char *ps;
	char buf[1024];
	char b1[1024], b2[1024];
	int i;
	unsigned long subtotal = 0;
	unsigned long total = l->total;
	int tcpcount, udpcount, icmpcount;

	int c = l->count;	/* no of hash elements */

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
	qsort((void *)head, c, sizeof (struct snode), sortbycounter);

	/* print it out
	*/
	fprintf(fd, "%s: %u : ", listname, c); 
	for ( i = 0; i < c; i++) {
		if (i >= count) {
			break;
		}
		subtotal += head[i].counter;

		fprintf(fd,"%s : %u : ",
				intoa(head[i].ipkey.ipsrc),
				head[i].counter);
		
	}
	fprintf(fd,"\n");
	free(head);
}


/*clearly this exists just to clear out a stats struct*/
void initStats(struct hashStats *stats)
{
	stats->count = 0;
	stats->hits = 0;
	stats->misses = 0;
	stats->collisions = 0;
	stats->maxChain = 0;
}


void printStats(FILE* fd, char* prefix, struct hashStats *stats)
{

	fprintf(fd, 
		"%s : HEAD : Size, Count, Hits, Miss, Coll, Max\n",
		prefix);
	fprintf(fd, "%s : DATA : %d : %d : %d : %d : %d : %d\n",
		prefix, SHASHSIZE, stats->count, stats->hits, stats->misses,
		stats->collisions, stats->maxChain); 

}

void dumpIpkey(struct sipkey *ik)
{
	printf("%s ", intoa(ik->ipsrc));
	printf("%s ", intoa(ik->ipdst));
}

