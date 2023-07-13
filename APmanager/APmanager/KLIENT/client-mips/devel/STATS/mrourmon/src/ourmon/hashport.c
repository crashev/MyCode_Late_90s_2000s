/*
 * hashport.c - deal with dynamic allocation and sorting
 *      of an arbitrary number of nodes.  This is intended
 *      for use by the top n mechanism.
 */

/*
 * Copyright (c) 2003 Portland State University
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

#include "hashport.h"
#include "stats.h"
#include "ehash.c"		/* inlined function */

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

/* called before list can be used 
 */
void init_plist (struct plist *l) {
	l -> entries = 0;
	l -> last = NULL;
	bzero (&l->hashlist[0], HSIZE * sizeof(struct pnode *));
}

/*
 * search_srcport - hash list lookup
 *
 *	inputs:
 *		l - a list pointer
 *		srcport as logical key
 *	returns:
 *		NULL - didn't find it
 *		struct pnode *n 
 * side effect: if pnode found, increment src counter
 */ 
struct pnode *
search_srcport(struct plist *l, USHORT srcport, ULONG bytecount)
{
	struct pnode *n;
	int mod;
	int first = 0;

	if (l ->entries == 0) 
		return (NULL);
	if (l ->last && (l->last->port == srcport)) {
		l->last->bytecount += bytecount;
		l->last->srccounter += 1;
		os.hp_chits++;
		return (l->last);
	}
	os.hp_cmiss++;
	/* search the hash/chain list
	*/
	n = l->hashlist[mod=(ehash((unsigned char *)&srcport, 2) % HSIZE)];
	for (; n!= NULL ; n=n->next) {
		if (n && ( n->port == srcport)) {
			l->last = n;
			l->last->bytecount += bytecount;
			l->last->srccounter += 1;
			if (first) {
				os.hp_bfirst++;
			}
			else {
				os.hp_bother++;
			}
			return(n);
		}
		first++;
	}
	os.hp_bmiss++;
	return (NULL);
}

/*
 * search_dstport - hash list lookup
 *
 *      inputs:
 *              l - a list pointer
 *              dstport as logical key
 *      returns:
 *              NULL - didn't find it
 *              struct pnode *n
 * side effect: if pnode found, increment dst counter
 */

struct pnode *
search_dstport(struct plist *l, USHORT dstport, ULONG bytecount)
{
	struct pnode *n;                                                 
	int mod;
	if (l ->entries == 0)
		return (NULL);
	if (l ->last && (l->last->port == dstport)) {
		l->last->bytecount += bytecount;
		l->last->dstcounter += 1;
		return (l->last);
	}
	/* search the hash/chain list
	*/
	n = l->hashlist[mod=(ehash((unsigned char *)&dstport, 2) % HSIZE)];
	for (; n!= NULL ; n=n->next) {
		if (n && ( n->port == dstport)) {
			l->last = n;
			l->last->bytecount += bytecount;
			l->last->dstcounter += 1;
			return(n);
		}
	}
	return (NULL);
} 

/*
 *      insert_srcport
 *
 *      search_srcport succeeds or fails. if it succeeds
 *      we don't call insert_srcport.  if it fails, we
 *      call this function to create a new hash list
 *      entry and to set the initial src counter value.
 *
 *      returns:
 *              1 - ok
 *              0 - malloc failure
 */

int
insert_srcport (struct plist *l, USHORT srcport, ULONG bytecount)
{ 
	struct pnode *n;
	struct pnode *h, *prev;
	unsigned int mod;
	n = (struct pnode *) malloc(sizeof(struct pnode));
	if (n == NULL) {
		/*
		fprintf(stderr, "out of space with malloc in insert_srcport\n");
		*/
		syslog(LOG_ERR, "malloc failure in insert_srcport, count %d",
			l->entries);
		return (0);
	}
	l->entries += 1;
	n->port = srcport;
	n->srccounter =1;
	n->dstcounter = 0;
	n->bytecount = bytecount;
	l->last = n;
	/* insert the pnode in the hash list
	*/
	n->next = NULL;
	h = l->hashlist[mod=(ehash((unsigned char *)&srcport, sizeof(unsigned short)) % HSIZE)];
	if (h == NULL) {
		l->hashlist[mod] = n;
	}
	else {
		os.hp_coll++;
		n->next = h;
		l->hashlist[mod] = n;
	}
	return(1);
}

/*
 *      insert_dstport
 *
 *      search_dstport succeeds or fails. if it succeeds
 *      we don't call insert_dstport.  if it fails, we
 *      call this function to create a new hash list
 *      entry and to set the initial dst counter value.
 *
 *      returns:
 *              1 - ok
 *              0 - malloc failure
 */

int 
insert_dstport (struct plist *l, USHORT dstport , ULONG bytecount)
{
	struct pnode *n;   
	struct pnode *h, *prev;
	unsigned int mod;
	n = (struct pnode *) malloc(sizeof(struct pnode));
	if (n == NULL) {
		syslog(LOG_ERR, "malloc failure in insert_dstport, count %d",
			l->entries);
		/*
		fprintf(stderr, "out of space with malloc in insert_dstport\n");
		*/
		return (0);
	}
	l->entries += 1;
	n->port = dstport;
	n->bytecount = bytecount;
	n->srccounter = 0;
	n->dstcounter = 1;
	l->last = n;
	/* insert the pnode in the hash list
	*/
	n->next = NULL;
	h = l->hashlist[mod=(ehash((unsigned char *)&dstport, sizeof(unsigned short)) % 
HSIZE)];
	if (h == NULL) {
		l->hashlist[mod] = n;   
	}
	else {
		n->next = h;
		l->hashlist[mod] = n;
	}
	return(1);
}

/*
 * deallocate malloced hash entries
 */

void
destroy_plist(struct plist *l) 
{
	int i;
	struct pnode *n;
	struct pnode *prev;
	for (i =0; i< HSIZE; i++) {
		n = l->hashlist[i];
		while(n != NULL) {
			prev = n;
			n = n->next;
			free(prev);
		}
	}
}

dump_hash(struct plist *l) {
	struct pnode *h;
	int counters[HSIZE];
	int i;
	int ls = 0;
	printf("No. of entries %d\n", l->entries);
	bzero(&counters, sizeof(int)*HSIZE);
	printf("\nhashdump---------\nlist entries %d\n",l->entries);
	for (i=0;i<HSIZE;i++) {
		h = l->hashlist[i];
		while(h != NULL) {
			ls = 1;
			printf("%d:", i);
			printf("port %d", h->port);
			printf(" bytecount %u", h->bytecount);
			printf(" srccounter %u", h->srccounter);
			printf(" dstcounter %u",h->dstcounter);
			counters[i]++;
			h = h->next;
		}
		if (ls) {
			printf(":%d \n",i);
		}
		ls = 0;
	}
	printf("\n");
	for (i = 0; i<HSIZE; i++) {
		if (counters[i] != 0) {
			printf("counters[%d]:%d buckets\n", i, counters[i]);
		}
	}
	printf("end hash dump---------\n");
}

/* pack the hashlist into an array
 * and return the array.
 *
 *	returns:
 *		1 - packed array exists
 *		0 - malloc failure
 */
static
struct pnode *
pack_array(struct plist *l)
{
	struct pnode *head, *n;
	unsigned int c = l->entries;  /* count of hash entries allocated */
	int i = 0;
	int hcounter = 0;
	head = (struct pnode *) malloc(c * sizeof (struct pnode));
	if (head == NULL) {
		/*
		fprintf(stderr, "pack_array, malloc failed\n");
		*/
		syslog(LOG_ERR, "malloc failure in port/pack_array, count %d",
			l->entries);
		return (NULL);
	}
	/* walk the hashlist
	*/
	for (i =0 ; i<HSIZE; i++) {
		n = l->hashlist[i];
		/* if n is non-nulln use, therefore put it in the array,
		* and increment hcounter
		*/
		while (n != NULL) {
			head[hcounter].port = n->port;
			head[hcounter]. bytecount = n->bytecount;
			head[hcounter]. srccounter = n->srccounter;
			head[hcounter]. dstcounter = n->dstcounter;
			n = n->next;
			hcounter++;
		}
	}  
	return(head);
}

static int
sort_by_bytecount(e1,e2)
struct pnode *e1, *e2;
{
	if (e1->bytecount > e2->bytecount) return -1;
	if (e1->bytecount < e2->bytecount) return 1;
	return 0;
} 

/* given a tcplist, sort it,
 * and print topN tcp results.
 *
 * inputs:
 *      fd - file for print output
 *      l - hash array
 *      tcpflag/udpflags - flags to indicate various filter outputs
 *      count - max desired outputs for all filters
 */
void
print_tcpplist(FILE *fd, struct plist *l, int tcpflag, int count)
{

#ifdef TCPPORTREPORT 
	FILE *sfd;	
#endif
	struct pnode *head;
	int i;
	int c = l->entries;  /* no of hash elements */
	/* form the hashlist into a packed array
	*/
	head = pack_array(l);
	if (head == NULL) {
		/*
		fprintf(fd, "malloc error in print_tcpplist\n");
		*/
		return;
	}
	/* quicksort it
	*/
	qsort((void *)head, c, sizeof(struct pnode), sort_by_bytecount);
 
	/* print it out
	*/
	if (tcpflag) {
		fprintf(fd, "tcp_ports: %u : ", l->entries); 
		for (i = 0; i<c; i++) {
			if ( i >= count) {
				break;
			}
			fprintf(fd, "%d:%u:%i:%i : ",
			head[i].port, head[i].bytecount, head[i].srccounter, head[i].dstcounter);
		}
		fprintf(fd, "\n");
#ifdef TCPPORTREPORT
		/* 
		 * This is a hack intended for batch gathering of data.
	         * It is not on in normal ourmon operation.
		 *
		 * print out ALL of the ports seen during the sample period
		 * with one IP address per line, followed by the data tuple parts
		 */

		sfd = fopen("/tmp/portdump.txt", "w");
		if (sfd == NULL) {
			syslog(LOG_ERR, "could not open %s:(%m)", "/tmp/portdump.txt");  
			exit(1);
		}
		/* format:  port: total bytes: pkt counts as src port: pkt counts as dst port:
		*/
		fprintf (sfd, "# %u\n", l->entries);
		for (i = 0; i < l->entries; i++) {
			fprintf(sfd, "%d:%u:%i:%i : ",
			head[i].port, head[i].bytecount, head[i].srccounter, head[i].dstcounter);
			fprintf(sfd, ":\n");
		}
		fclose(sfd);
#endif
	}
	free(head);
}          

/* given a udplist, sort it,
 * and print topN udp results.
 *
 * inputs:
 *      fd - file for print output
 *      l - hash array
 *      tcpflag/udpflags - flags to indicate various filter outputs
 *      count - max desired outputs for all filters
 */
 
void
print_udpplist(FILE *fd, struct plist *l, int udpflag, int count)
{
	struct pnode *head;
	int i;
	int c = l->entries;  /* no of hash elements */
	/* form the hashlist into a packed array
	*/
	head = pack_array(l); 
	if (head == NULL) {
		/*
		fprintf(fd, "malloc error in print_udpplist\n");
		*/
		return;
	}
	/* quicksort it
	*/
	qsort((void *)head, c, sizeof(struct pnode), sort_by_bytecount);

	/* print it out
	*/
	if (udpflag) {
		fprintf(fd, "udp_ports: %u : ",l->entries);
		for (i = 0; i<c; i++) {
			if (i >= count) {
			break;
			} 
			fprintf(fd,"%d:%u:%i:%i : ",
			head[i].port, head[i].bytecount, head[i].srccounter,head[i].dstcounter);
		}
		fprintf(fd, "\n");
	}
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
        struct plist l;
        ULONG c= 1;
        USHORT srcport = 25;
        USHORT dstport = 55;
        int rc;
        struct pnode *n = NULL;
        init_plist(&l);
        n = search_srcport(&l, srcport,c);
        if (n == NULL) {
                rc = insert_srcport(&l,srcport,c);
                if (rc == 0) {
                        printf("insert_srcport-%d failed\n",srcport);
                }
        }
        n = search_dstport(&l, dstport,c);
        if (n == NULL) {
                rc = insert_dstport(&l,dstport,c);
                if (rc == 0) {
                        printf("5.insert_dstport-%d failed\n",dstport);
                }
        }
        srcport = 55;
        dstport = 25;
        c = 2;
        n = search_srcport(&l, srcport,c);
        if (n == NULL){
                rc = insert_srcport(&l, srcport,c);
                if (rc==0) {
                        printf("7.insert_srcport failed\n",srcport);

                }
        }
        n = search_dstport(&l, dstport,c);
        if (n == NULL) {
                rc = insert_dstport(&l, dstport,c);
                if (rc == 0){
                        printf("8.insert_dstport failed\n",dstport);
                }
        }
        srcport = 34;
        dstport = 55;
        c = 2;
        n = search_srcport(&l, srcport,c);
        if (n == NULL){
                rc = insert_srcport(&l, srcport,c);
                if (rc==0) {
                        printf("7.insert_srcport failed\n",srcport);
                }
        }
        n = search_dstport(&l, dstport,c);
        if (n == NULL) {
                rc = insert_dstport(&l, dstport,c);
                if (rc == 0){
                        printf("8.insert_dstport failed\n",dstport);
                }
        }
        dump_hash(&l);
        print_tcpplist(stdout, &l,1,2);
        print_udpplist(stdout, &l,1,2);
        destroy_plist(&l);
}
#define LATER
#ifdef LATER
randomTest()
{
        struct plist l;
        unsigned long key1;
        unsigned counter1;
        unsigned long k,c;
        struct pnode *n;
        int i;
        int rc;

        init_plist(&l);

        for ( i = 0; i < 10000; i++) {
                key1 = getRandomWord();
                counter1 = getRandomWord();

                printf("[i=%d], key=%lx, counter=%lx\n", i, key1, counter1);
                n = search_srcport(&l, key1,counter1);
                if (n == NULL){
                        rc = insert_srcport(&l, key1,counter1);
                        if (rc==0) {
                                 printf("7.insert_srcport %u failed\n",key1);
                        }
                }
                else {
                        printf("FOUND ONE");
                }

        }

        printf("node count %d\n",l.entries); 
	dump_hash(&l);
	print_tcplist(stdout, &l,1,2);
        destroy_plist(&l);
}

#endif

main()
{
	functionTest();
//	randomTest();
}
#endif
