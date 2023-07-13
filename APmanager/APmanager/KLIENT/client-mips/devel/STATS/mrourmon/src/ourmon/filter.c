/*
 * filter.c
 *
 * functions (or in-line code) for all cascading filters
 *
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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pcap.h>
#include <syslog.h>
#ifdef	TIME_WITH_SYS_TIME
#include <time.h>
#endif

#include "ourmon.h"
#include  "filter.h"
#include  "stats.h"

extern struct fixed_ipproto 	fixed_ipproto;
extern struct fixed_tcp3 	fixed_tcp3;

void fixed_ipproto_filter(unsigned int , struct t_entry *);
void fixed_tcp3_filter(unsigned int, struct t_entry *);
void fixed_iprange1_filter(unsigned int, struct t_entry *);
void fixed_cast_filter(unsigned int, struct t_entry *);
void topn_ip_filter(unsigned int, struct t_entry *);
void topn_syn_filter(unsigned int, struct t_entry *);
void topn_icmp_filter(unsigned int, struct t_entry *);
void topn_port_filter(unsigned int, struct t_entry *);
void topn_ip_scans_filter(unsigned int, struct t_entry *);
void topn_port_scans_filter(unsigned int, struct t_entry *);

ip_filter(unsigned int caplength, struct t_entry *te) 
{

	if (debug)  {
		printf("ip_filter pkt_len %d\n", caplength);
	}

	/* cascading filters - check them all ...
	*/
	if (fixed_ipproto.configured) {
		fixed_ipproto_filter(caplength, te);
	}

	if (fixed_tcp3.configured) {
		fixed_tcp3_filter(caplength, te);

	}

	if (fixed_iprange1.configured) {
		fixed_iprange1_filter(caplength, te);
	}

	if (fixed_cast.configured) {
		fixed_cast_filter(caplength, te);
	}

	/* topn manipulates linked lists and free lists
	 * block ALARM whilst in there
	 */
	block_signal();

	if (topn_ip.configured) {
		topn_ip_filter(caplength, te);
	}

	if (topn_syn.configured) {
		/* called for tcp/icmp as icmp errors are counted as well
		*/
         	if ( te ->proto == IPPROTO_TCP || te->proto == IPPROTO_ICMP) {
			topn_syn_filter(caplength, te);
		}
	}

	if (topn_icmperror.configured) {
         	if ( (te->proto == IPPROTO_UDP) || (te->proto == IPPROTO_ICMP) 
			|| (te->proto == IPPROTO_TCP)) {
			topn_icmp_filter(caplength, te);
		}
	}

	if (topn_port.configured) {
		topn_port_filter(caplength, te);
	}

        if (topn_scans_input.configured) {
                topn_ip_scans_filter(caplength, te);
        }

        if (topn_scans_ports_input.configured) {
                topn_port_scans_filter(caplength, te);
        }

        if (topn_irc.configured && te->psize && (te->proto == IPPROTO_TCP)) {
        	ircscan(&topn_irc.irclist, te);
	}

	unblock_signal();
}

/* note: we do not count "non-ip" except in the case
 * of fixed_ipproto, specified ip_prpto, and fixed_cast.
 * The other filters are only for ip addresses in terms
 * of their "xtra" bytes.
 */
non_ip_filter(caplength) 
{
	if (fixed_ipproto.configured) {
		fixed_ipproto.xtra_bytes += caplength;
	}
	if (spec_ipproto.configured) {
		spec_ipproto.xtra_bytes += caplength;
	}
	if (fixed_cast.configured) {
		fixed_cast.xtra_bytes += caplength;
	}
}

void
fixed_ipproto_filter(unsigned int caplength, struct t_entry *te) 
{
	switch(te->proto) {
	case IPPROTO_TCP:
		fixed_ipproto.tcp_bytes += caplength;
		break;
	case IPPROTO_UDP:
		fixed_ipproto.udp_bytes += caplength;
		break;
	case IPPROTO_ICMP:
		fixed_ipproto.icmp_bytes += caplength;
		break;
	/* TBD. this should be a global counter
	*/
	default:
		fixed_ipproto.xtra_bytes += caplength;
		break;
	}		
}

void fixed_tcp3_filter(unsigned int caplength, struct t_entry *te) 
{
	if (te->proto == IPPROTO_TCP) {
		unsigned int dport = te->dport;
		unsigned int sport = te->sport;
		if (debug) {
			printf("tcp3, dport %d, sport %d\n", 
				dport, sport);
		}
		if (dport == fixed_tcp3.tcp_port1
			|| sport == fixed_tcp3.tcp_port1) {
			if (debug) {
				printf("tcp3, port1 b=%d\n", caplength);
			}
			fixed_tcp3.port1_bytes += caplength;
		}
		else if (dport == fixed_tcp3.tcp_port2
			|| sport == fixed_tcp3.tcp_port2) {
			if (debug) {
				printf("tcp3, port2 b=%d\n", caplength);
			}
			fixed_tcp3.port2_bytes += caplength;
		}
		else {
			if (debug) {
				printf("tcp3(p), xtra b=%d\n", caplength);
			}
			fixed_tcp3.xtra_bytes += caplength;
		}
	}
	else {
		if (debug) {
			printf("tcp3, xtra b=%d\n", caplength);
		}
		fixed_tcp3.xtra_bytes += caplength;
	}
}

/* take the src and dst in turn, and see if they belong
 * in the range.  reverse and check, else fail.
 */
void fixed_iprange1_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct in_addr src1, dst1, src2, dst2;

	src1.s_addr = te->src.s_addr & fixed_iprange1.netmask1.s_addr;
	dst1.s_addr = te->dst.s_addr & fixed_iprange1.netmask2.s_addr;
	if ( src1.s_addr == fixed_iprange1.ipnet1.s_addr 
		&&
	     dst1.s_addr == fixed_iprange1.ipnet2.s_addr ) {
		
		fixed_iprange1.range1_bytes += caplength;
		return;
	}
	src2.s_addr = te->src.s_addr & fixed_iprange1.netmask2.s_addr;
	dst2.s_addr = te->dst.s_addr & fixed_iprange1.netmask1.s_addr;
	if ( src2.s_addr == fixed_iprange1.ipnet2.s_addr 
		&&
	     dst2.s_addr == fixed_iprange1.ipnet1.s_addr ) {
		fixed_iprange1.range1_bytes += caplength;
		return;
	}
	fixed_iprange1.extra_bytes += caplength;
}

/* 
 * if we are handed ethernet packets, we function at ethernet
 * level, else we try and fake it with IP level packets.
 * sort IP addresses by address dst type, multicast, broadcast, extra
 * note: this will miss non-ip mcast/bcast
 */
void fixed_cast_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct in_addr dst;

	if (te->eflag) {
		if (te->bflag) {
			fixed_cast.bcast_bytes += caplength;
			return;
		}
		if (te->mflag) {
			fixed_cast.mcast_bytes += caplength;
			return;
		}
		fixed_cast.ucast_bytes += caplength;
		return;
	}
	/* it wasn't an ethernet packet
	*/
	dst.s_addr = ntohl(te->dst.s_addr);

	/* multicast IP
	*/
	if (IN_MULTICAST(dst.s_addr)) {
		fixed_cast.mcast_bytes += caplength;
		return;
	}

	/* IP broadcast checks
	*/

	/* leaks are possible here.  we in theory
	 * have the ether header here, and should simply
	 * check for all 1's in the dst mac.  We know we
	 * have an ip address
	 */
	if ((dst.s_addr == 0x0) | (dst.s_addr == 0xffffffff)) {
		fixed_cast.bcast_bytes += caplength;
		return;
	}
	if ((dst.s_addr == ntohl(fixed_cast.ipnet0.s_addr)) ||
		(dst.s_addr == ntohl(fixed_cast.ipnet1.s_addr))) {
		fixed_cast.bcast_bytes += caplength;
		return;
	}

	/* unicast land
	*/
	if (IN_CLASSA(dst.s_addr)) {
		fixed_cast.ucast_bytes += caplength;
		return;
	}
	if (IN_CLASSB(dst.s_addr)) {
		fixed_cast.ucast_bytes += caplength;
		return;
	}
	if (IN_CLASSC(dst.s_addr)) {
		fixed_cast.ucast_bytes += caplength;
		return;
	}
	fixed_cast.xtra_bytes += caplength;
}

/*
 * TBD: put some sort of warning/guard in to warn against too
 * much memory being used (anti - DOS attack).
 *
 * List mechanism here inserts new items at bottom of doubly-linked
 * list and tries to sort them up.  Kept sorted with topN at head of list.
 */
void
topn_ip_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct node *n;
	int rc;
	struct ipkey ik;

	/* ip dst is the key, caplength is the counter
	*/
	/* search and see if ip addr is found
	*/
        ik.ipsrc = te->src.s_addr;
        ik.ipdst = te->dst.s_addr;
        ik.srcport = te->sport;
	ik.dstport = te->dport;
	if (debug) {
		printf("%d %d\n", ik.srcport, ik.dstport);
	}
        ik.ipproto = te->proto;

#ifdef OLD
	n = searchNode(&topn_ip.iplist, &ik, caplength);

	/* failed to find it, insert it
	*/
	if (n == NULL) {
		rc = insertNode(&topn_ip.iplist,  &ik, caplength);
		if (rc == 0) {
			/*
			fprintf(stderr, "malloc failure - out of memory in topn_ip\n");
			*/
		} 
	}
#else
	searchInsertNode(&topn_ip.iplist, &ik, caplength);
#endif
}

/*
 * the tcp syn filter.
 * This is only called for TCP packets (caller checks this)
 */ 
void
topn_syn_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct synnode *n;
	int rc;
	unsigned long ipofinterest;
	unsigned long ipother;
	int dport = -1;			/* 0 is a possible port */
	int saflag = 0;			/* is this syn+ack or just syn */

	/* the packet might be an icmp packet.  deal with that entirely,
	 * after which we know it is a tcp packet only.
	 */
        if ( te->proto == IPPROTO_ICMP) {

		/* if any kind of icmp unreachable, redirect, timxceed,
		 * keep it, else ignore it.
		 */
		if (te->sport == 3 || te->sport == 5 || te->sport == 11) {
			;
		} 
		else {
			/* TBD. we could treat echos back as real work, but not now.
			*/
			return;
		}
		/* search on it, if we don't find it, we ignore it as there is no ip src for it
		 * if we do find it, we count the icmp error type.
		*/
		if (debug) {
			printf("FOO: topn_syn_filter ICMP of interest: %x\n", 
				te->dst.s_addr);
		}
		/* params: call synlist. pass dst in "src of interest" slot.  3 tcp flags = 0,
		 * -1 for dst port (not of interest), 1 for count icmp please.
		*/
		n = search_srcsyn(&topn_syn.synlist, te->dst.s_addr, 0, 0, 0, 0, 0, -1, 1);
		return;
	} 

	/* invariant: now a tcp packet
	*/
	/* choose an ip address to work with (ipofinterest).
	 * but record the other one for counting tcp packets in general.
	 * 
	 * if synflag, then src is of interest
	 * if fin or reset, then dst
	 * also count tcp packet against both src and dst
	 * So in summary we are counting tcp packets against both src/dst,
	 * but we are tracking syns from an ip src, and fins/resets back to an ip src.
	*/

	ipofinterest = te->src.s_addr;
	ipother = te->dst.s_addr;		
	dport = (int) te->dport;		/* dst port is of interest */
	if (te->synflag) {
		if (te->ackflag) {
			saflag = 1;		/* it is a syn ack */
		}
	} 
	else if (te->finflag) {
		ipofinterest = te->dst.s_addr;		/* fins sent back to this src */
		ipother = te->src.s_addr;
		/* dport is -1 here, not of interest */
		dport = -1;
	}
	/* else work on ip dst if reset flag is set.
	 */
	else if (te->resetflag) {
		ipofinterest = te->dst.s_addr;		/* resets sent back to this src */
		ipother = te->src.s_addr;
		/* dport is -1 here, not of interest */
		dport = -1;
	}

	/* else no flag set and we fall through (no s/f/r).  
	 * note: ip src is 1st param, ip dst is 2nd param in this case. 
	 * due to default set above.
	 */
	
	if (debug) {
		printf("topn_syn_filter: %x, %d, %d, %d, %d %d, \n", ipofinterest,
			te->synflag, te->finflag, te->resetflag, te->ackflag, dport);
	}

	n = search_srcsyn(
		&topn_syn.synlist, 
		ipofinterest, 
		ipother,
		te->synflag, 
		te->finflag, 
		te->resetflag, 
		saflag,			/* it is a syn/ack, as well as a syn */
		dport, 
		0);

	/* failed to find it, insert it
	*/
	if (n == NULL) {
		rc = insert_srcsyn(&topn_syn.synlist, ipofinterest, 
			te->synflag, 
			te->finflag, 
			te->resetflag, saflag, dport
		);
	}
}

/*
 * icmp error filter
 * called for all TCP/UDP and ICMP packets ...
 *
 * If tcp/udp/icmp echo request, we hash the ip src.
 * If icmp error packet, we lookup the ip dst to see if we have a tcp/udp/ping sender.
 * If we do, we count the error type, and the total errors.
 * We also count the number of tcp/udp/icmp ping packets.
 * We sort on the total icmp errors.
 *
 * Note: there is no strong correlation between a tcp/udp sender and an icmp
 * error.  Therefore we count L4 packets sent as an indicator of what the 
 * host in question is up to.
 */ 
void
topn_icmp_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct icmpnode *n;
	int rc;
	int icmpcode = te->sport;	/* major icmp code */
	int tcpflag;
	int udpflag;
	long udpport = -1;		/* can't use 0 as it may be used as a real port, therefore
				         * -1 for no port (signed long)
					 */

	/* if icmp error, we only search, we do not insert unless we have an icmp echo request
	 * if error, we search on the destination ...
	 * we only count on 3 kinds of icmp errors.  other errors are ignored.
	 * unreachable/redirect/timxceed
	 */
        if ( te->proto == IPPROTO_ICMP) {

		/* if any kind of icmp unreachable, redirect, timxceed,
		 * keep it, else ignore it.
		 */
		if (te->sport == 3 || te->sport == 5 || te->sport == 11) {
			/* search on it, if we don't find it, we ignore it as there is no ip src for it
			 * if we do find it, we count the icmp error type.
			 */
			if (debug) {
				printf("topn_icmperror_filter ICMP error: %s, %d\n", 
					inet_ntoa((struct in_addr) te->dst), te->sport);
			}
			n = search_srcicmp(&topn_icmperror.icmplist, te->dst.s_addr, 0, 0, 0,
				te->sport == 3, 	
				te->sport == 5, 
				te->sport == 11, 0, -1); 
		} 
		else {
			/* if echo request
			*/
			if (te->sport == 8) {
				if (debug) {
					printf("topn_icmperror_filter ICMP ping: %s, %d\n", 
						inet_ntoa((struct in_addr) te->src), te->sport);
				}
				/* count as icmp packet */
				n = search_srcicmp(&topn_icmperror.icmplist, te->src.s_addr, 0, 0, 1,
					0, 0, 0, 0, -1);
				/* if lookup failed, insert */
				if (n == NULL)  
					rc = insert_srcicmp(&topn_icmperror.icmplist, te->src.s_addr, 
						0, 0, 1, -1);
				 
			}
			return;
		}
		return;
	} 

	/* packet is tcp or udp packet */

	/* what L4 protocol (count it */
	if ( te->proto == IPPROTO_TCP ) {
		udpflag = 0;
		tcpflag = 1;
	}
	else if ( te->proto == IPPROTO_UDP ) {
		udpflag = 1;
		tcpflag = 0;
		udpport = (long) te->dport;
	}
	else {
		return;
	}
	
	/* search the hash and count the L4 type.  icmp error/udp recv flags are set to 0. 
	*/

	n = search_srcicmp(&topn_icmperror.icmplist, te->src.s_addr, tcpflag, udpflag, 0,
			0, 0, 0, 0, udpport);
	
	if (debug) {
		printf("topn_icmperror_filter: tcp/udp %s proto: %x\n", 
			inet_ntoa((struct in_addr)te->src), te->proto);
	}

	/* failed to find it, insert it and count the L4 type
	*/
	if (n == NULL) {
		rc = insert_srcicmp(&topn_icmperror.icmplist, te->src.s_addr, tcpflag, udpflag, 0, udpport);
	}

	/* in addition to above, for udp packets, we count returned UDP packets against
	 * known srcs.  We only count these, we never do an insert.
	*/
	if ( te->proto == IPPROTO_UDP ) {
		if (debug) {
			printf("topn_icmperror_filter: udprcv search: %s proto: %x\n", 
				inet_ntoa((struct in_addr)te->dst), te->proto);
		}
		n = search_srcicmp(&topn_icmperror.icmplist, te->dst.s_addr, 0, 0, 0,
			0, 0, 0, 1, -1);
	}
}

void 
topn_port_filter(unsigned int caplength, struct t_entry *te) 
{
         register struct pnode *n1,*n2;
         int r1,r2;
         unsigned short srcport;
         unsigned short dstport;
         srcport = te->sport;
         dstport = te->dport;
         /* TCP list
         */
         if ( te ->proto == IPPROTO_TCP) {
         	n1 = search_srcport(&topn_port.tcplist, srcport,caplength);
           	if (n1 == NULL) {
             		r1 = insert_srcport(&topn_port.tcplist, 
				srcport, caplength);
           	}
           	n2 = search_dstport(&topn_port.tcplist, dstport,caplength);
           	if (n2 == NULL) {
             		r2 = insert_dstport(&topn_port.tcplist, 
				dstport,caplength);
           	}
         }
         /* UDP list
          */
         if ( te ->proto == IPPROTO_UDP) {
           	n1 = search_srcport(&topn_port.udplist, srcport,caplength);
           	if (n1 == NULL) {
             		r1 = insert_srcport(&topn_port.udplist, 
				srcport,caplength);
           	}
           	n2 = search_dstport(&topn_port.udplist, dstport,caplength);
           	if (n2 == NULL) {
             		r2 = insert_dstport(&topn_port.udplist, dstport,caplength);
           	}
         }
}


/* 
 * topn ip destination scans filter.
 * This filter takes packets and hashes them in to
 * a temporary hashlist by SRCIP and DSTIP to get a count of
 * unique host to host flows.  The thinking is that if a host
 * has many destinations in one interval, it is probably doing
 * a net scan.  
 */
void
topn_ip_scans_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct snode *n;
	int rc;
	struct sipkey ik;
	extern int debug;

	/* ip dst is the key, caplength is the counter
	*/
	/* search and see if ip addr is found
	*/
        ik.ipsrc = te->src.s_addr;
        ik.ipdst = te->dst.s_addr;
        ik.srcport = te->sport;
	ik.dstport = te->dport;
	
	if (debug) {
		printf("TopnScans: %s %s\n", intoa2(ik.ipsrc), intoa2(ik.ipdst));
	}
        
	ik.ipproto = te->proto;
	caplength = 1;  //we only care about packet counts just now

	/* This is a hashlist search node function that takes a function-
         * pointer argument of type  long function(*ipkey).
         * It uses the function-pointer to extract from the ipkey a
         * hash value  - this allows me to reuse this search function later
         */
	n = searchKeyNode(&topn_scans_input.iplist, &ik, caplength,
			  hashSrcDst);

	/*We are keeping stats on hash usage and this is the total count of
	 *packets seen.
	 */
	topn_scans_ip_stats.count +=1;

	/* failed to find it, insert it
	*/
	if (n == NULL) {
	
		if (debug) {
			printf("TopnScans INSERT: %s ",intoa2(ik.ipsrc));
			printf("%s ",intoa2(ik.ipdst));
			printf("%d %d\n", 
			ik.ipsrc, ik.ipdst);
		}

		/* This is a hashlist insert node function that takes a function-
		 * pointer argument of type  long function(*ipkey).
		 * It uses the function-pointer to extract from the ipkey a
		 * hash value (this allows me to reuse this insert function later
		 * Note that it takes a stats structure because we gather statistics
		 * about inserts
		 */
		rc = insertKeyNode(&topn_scans_input.iplist,  &ik, 
					   caplength, hashSrcDst, 
					&topn_scans_ip_stats) ;
		if (rc == 0) {
			/* no printfs in daemon.
			fprintf(stderr, 
			"malloc failure - out of memory in topn_ip\n");
			*/
		} 
	}
	else{
		/* keep track of when we insert a new flow
		*/
		
		topn_scans_ip_stats.hits+=1;	
		if (debug) {
			printf("TopnScans Duplicate: %s ",intoa2(ik.ipsrc));
			printf("%s ",intoa2(ik.ipdst));
			printf("%d %d\n", 
			ik.ipsrc, ik.ipdst);
        	}
	}
}

/* 
 * Topn Port scans filter
 * This filter basically takes packets and hashes them in to
 * a temporary hashlist by SRCIP and DSTPORT to get a count of
 * unique host to port flows.  The thinking is that if a host
 * has many destination ports in one interval, it is probably doing
 * a port scan.  One semantic note about this filter is that it
 * does not differentiate between one host communicating to n others
 * on the same destination port and one host communicating with one
 * other on the same destination port.  Future work might include a
 * filter that hashes based one SRCIP, DSTIP, DSTPT to get this result. 
 */

void
topn_port_scans_filter(unsigned int caplength, struct t_entry *te) 
{
	register struct snode *n;
	int rc;
	struct sipkey ik;
	int i;
	extern int debug;
	
	struct slist* l = &topn_scans_ports_input.iplist;
	struct hashStats* stat =&topn_scans_ports_stats; 

	/* ip dst is the key, caplength is the counter
	*/
	/* search and see if ip addr is found
	*/
        ik.ipsrc = te->src.s_addr;
        ik.ipdst = te->dst.s_addr;
        ik.srcport = te->sport;
	ik.dstport = te->dport;
	
	if (debug) {
		printf("Topn_Port_Scans: %s %s\n", intoa2(ik.ipsrc), intoa2(ik.ipdst));
	}
	
	ik.ipproto = te->proto;

	if((ik.ipproto != IPPROTO_TCP)&&(ik.ipproto != IPPROTO_UDP)){
		return;
	}
	caplength = 1;  //we only care about packet counts just now
	
	/* This is a hashlist search node function that takes a function-
         * pointer argument of type  long function(*ipkey).
         * It uses the function-pointer to extract from the ipkey a
         * hash value (this allows me to reuse this search function later
         */
	for(i = 0; i<2; i++){

		n = searchKeyNode(l, &ik, caplength, hashSrcPort);

		/*This is the count of all packets seen by this filter*/
		if (stat){ 
			stat->count += 1;
		}

		/* failed to find it, insert it*/
		if (n == NULL) {
		
			if (debug) {
				printf("Topn_Port_Scans INSERT: %s ",intoa2(ik.ipsrc));
				printf("%s ",intoa2(ik.ipdst));
				printf("%d %d\n", 
				ik.ipsrc, ik.ipdst);
			}

			/*This is a hashlist insert node function that takes a function-
			 *pointer argument of type  long function(*ipkey).
			 *It uses the function-pointer to extract from the ipkey a
			 *hash value (this allows me to reuse this insert function later
			 *Note that it takes a stats structure because we gather statistics
			 *about inserts
			 */
			rc = insertKeyNode(l,  &ik, 
					   caplength, hashSrcPort,
					stat);
	

			if (rc == 0) {
				fprintf(stderr, 
					"malloc failure - out of memory in topn_port_scans\n");
			} 
		}
		else {
			/*This is a count of all "unique" host to port flows*/
			if(stat){
				stat->hits += 1;
			}
		
			if (debug) {
				printf("Topn_Port_Scans Duplicate: %s ",
					intoa2(ik.ipsrc));
				printf("%s ",intoa2(ik.ipdst));
				printf("%d %d\n", ik.ipsrc, ik.ipdst);
			}
		}

		if(te->proto == IPPROTO_TCP) {
			l = &topn_scans_ports_tcp_input.iplist;
			stat = NULL;
		}
		else {
			l = &topn_scans_ports_udp_input.iplist;
			stat = NULL;
		}
	}
}


analyze_bpf(p, caplen, packet_len)
char *p;
int caplen;
int packet_len;
{
	struct bpf_spec *b;
	extern struct bpf_spec bpfs[MAXBPFSPECS];
	int i, j, count;
	int anyok = 0;

        b = &bpfs[0];
        for ( i = 0; i < MAXBPFSPECS; i++, b++) {
	        if (b->configured == FALSE) {
       	        	break;
                }
		count = b->prog_count;
		anyok = 0;
                for ( j = 0; j < count; j++) {
			/* if result zero, match failed
			*/
                	if (bpf_filter(b->fcodes[j].bf_insns, p,
                                packet_len, caplen) == 0) {
				;  /* failure to match */
			}
			else {
				anyok = 1;
				/* count packets or bytes
				*/
				if (b->bpfpackets) {
					b->bpf_counts[j]++;
				}
				else {
					b->bpf_counts[j] += packet_len;
				}
			}
                }
		/* if we are counting extra and all filter
		 * specs in this set failed, then count against xtra.
		 */
		if (b->noxtra == 0 && anyok == 0) {
			if (b->bpfpackets) {
				b->xtra_bytes++;
			}
			else {
				b->xtra_bytes += packet_len;
			}
		}
	}
}

/*
 * note: we only print output in the mon.lite file if
 * a filter is configured in
 *
 * don't do something clever - like invoke DNS here
 * this is afterall a wannabe real-time application.
 */
void
write_report(char *monfile, int caught, int dropped, int caught2, int dropped2)
{
	FILE *fd;
	int i;
	extern int any_bpf;
	struct bpf_spec *b;
        extern struct bpf_spec bpfs[MAXBPFSPECS];
	int j, k, count;
	static int firststart = 1;
#ifdef SYSCALLHACK
	extern int g_bufsize;
#endif
	int syscalls;
	extern int snaplen; 
	extern int alarmsecs;

	fd = fopen(monfile, "w");
	if (fd == NULL ) {
		syslog(LOG_ERR, "oops - can't open monfile %s\n", monfile);
		exit(1);
	}
	/* compute number of syscalls
	 * syscalls = caught * snaplen / bufsize
	*/
#ifdef SYSCALLHACK
	syscalls = ((caught * snaplen) / g_bufsize) + 1;
#else
	syscalls = 0;
#endif
	/*
	syscalls *= alarm_secs / (timeout/1000)
	*/
	if (sysname[0]) {
		fprintf(fd, "sysname: %s\n", sysname);
	}

	if (firststart) {
		firststart = 0;
		fprintf(fd, "elog: ourmon restarted\n");
	} 

	fprintf(fd, "pkts: caught:%d : drops:%d: caught2:%d : drop2:%d\n", 
		caught, dropped, caught2, dropped2);

/*
	fprintf(fd, "io: syscalls:%d\n", syscalls);
*/

	if (fixed_ipproto.configured) 
	  fprintf(fd, "fixed_ipproto: tcp:%u : udp:%u : icmp:%u : xtra:%u:\n",
			fixed_ipproto.tcp_bytes,
			fixed_ipproto.udp_bytes,
			fixed_ipproto.icmp_bytes,
			fixed_ipproto.xtra_bytes  
			);

	if (fixed_tcp3.configured) 
	  fprintf(fd, "fixed_tcp3: %d:%u : %d:%u : xtra:%u:\n",
			fixed_tcp3.tcp_port1,
			fixed_tcp3.port1_bytes,
			fixed_tcp3.tcp_port2,
			fixed_tcp3.port2_bytes,
			fixed_tcp3.xtra_bytes
			);

	if (fixed_iprange1.configured) {
	  fprintf(fd, "fixed_iprange1: %s-%s:%u : xtra:%u:\n",
			fixed_iprange1.s_ipnet1,
			fixed_iprange1.s_ipnet2,
			fixed_iprange1.range1_bytes,
			fixed_iprange1.extra_bytes
			);
	}

	if (fixed_cast.configured) {
	  fprintf(fd, "fixed_cast: mcast:%u : ucast:%u : bcast:%u : xtra:%u:\n",
			fixed_cast.mcast_bytes,
			fixed_cast.ucast_bytes,
			fixed_cast.bcast_bytes,
			fixed_cast.xtra_bytes 
			);
	}

	if (fixed_size.configured) {
	  fprintf(fd, "fixed_size: tiny:%u : small:%u : med:%u : big:%u:\n",
			fixed_size.tiny,
			fixed_size.small,
			fixed_size.medium,
			fixed_size.large 
			);
	}

	if (any_bpf) {
		b = &bpfs[0];
	        for ( j = 0; j < MAXBPFSPECS; j++, b++) {
	                if (b->configured == FALSE) {
       		        	break;
			}
			/* bpf:label:pktflag:#progs:label:count ... :xtra:count\n";
			 * label - graph label. unique.  
			 * pktflag - 1: means count as packets, 0 as bytes 
			 * #progs - # bpfs in this graph (hence # of counts)
			 * label: count pairs, one per line in graph
			 * xtra: count (0 if xtra is off)
			*/
                	count = b->prog_count;
	  		fprintf(fd, "bpf:%s:%d:%d:", b->label, b->bpfpackets,
				count);
                	for ( k = 0; k < count; k++) {
				fprintf(fd, "%s:%u:", b->prog_label[k], 
					b->bpf_counts[k]);
			}
	  		fprintf(fd, "xtra:%u\n", b->xtra_bytes);
                }
	}

	/* print out per topn related summaries
	 */

	/* topn proper
	*/
	if (topn_ip.configured) {
		printList(fd, &topn_ip.iplist, 1, 1, 1, 1, topn_ip.actualn);
	}

        if (topn_irc.configured) {
                printIrc(fd, &topn_irc.irclist);
        }

	/* topn syn
	*/
	if (topn_syn.configured) {
		print_synlist(fd, &topn_syn.synlist, topn_syn.actualn, 
			topn_syn.wflag, topn_syn.wormfile,
			topn_syn.ipaflag,
			topn_syn.ipaddr.s_addr,
			topn_syn.netmask.s_addr);
	}

	/* topn icmperror
	*/
	if (topn_icmperror.configured) {
		print_icmplist(fd, &topn_icmperror.icmplist, topn_icmperror.actualn);
	}

	/* topn port
	*/
	if (topn_port.configured) {
            	print_tcpplist (fd, &topn_port.tcplist, 1, topn_port.actualn);
            	print_udpplist (fd, &topn_port.udplist, 1, topn_port.actualn);
		fprintf(fd, "hp_stat: %u: %u: %u: %u: %u: %u:\n",
			os.hp_chits, os.hp_cmiss, os.hp_bfirst, 
			os.hp_bother, os.hp_bmiss, os.hp_coll);
        }

	/* topn ip scan
	*/

        /* after having everything accumulated in the
         * hashlist, transfer it to a second hashlist resolving all
         * source collisions by keeping counts for each SRCIP of how
         * many related records were entered.  Then print the ouput
         * list (using the prefix-parameterized print function)
         * This print the associated stats.
         */
        if (topn_scans_input.configured) {
		/* turn inputs into outputs
		*/
		if (debug) {
			printf("processTopNScans\n");
		}
                processTopNScans(&topn_scans_input.iplist,
                           &topn_scans_output.iplist,
                                &topn_scans_ip_stats);

		if (debug) {
			printf("printSingle\n");
		}
		/* print output list
		*/
                printSingleIPList(fd, &topn_scans_output.iplist,
                                1, 1, 1, 1,
                                topn_scans_output.actualn,
                                "ip_scan");

		/* print hash stats
		*/
		if (debug) {
			printf("printSingle\n");
		}
                printStats(fd, "ips_scan_STATS", &topn_scans_ip_stats);
        }

	/* topn scan ports
	*/
        if (topn_scans_ports_input.configured) {
                processTopNScans(&topn_scans_ports_input.iplist,
                           &topn_scans_ports_output.iplist,
                                &topn_scans_ports_stats);

                printSingleIPList(fd, &topn_scans_ports_output.iplist,
                                1, 1, 1, 1,
                                topn_scans_ports_output.actualn,
                                "ip_portscan");

                printStats(fd, "ip_portscan_STATS", &topn_scans_ports_stats);

                processTopNScans(&topn_scans_ports_tcp_input.iplist,
                           &topn_scans_ports_tcp_output.iplist,
                                &topn_scans_ports_tcp_stats);

                printSingleIPList(fd, &topn_scans_ports_tcp_output.iplist,
                                1, 1, 1, 1,
                                topn_scans_ports_tcp_output.actualn,
                                "tcp_portscan");

                processTopNScans(&topn_scans_ports_udp_input.iplist,
                           &topn_scans_ports_udp_output.iplist,
                                &topn_scans_ports_udp_stats);

                printSingleIPList(fd, &topn_scans_ports_udp_output.iplist,
                                1, 1, 1, 1,
                                topn_scans_ports_udp_output.actualn,
                                "udp_portscan");

        }

	fclose(fd);
}

void
zero_all_filters()
{
	extern int any_bpf;
	struct bpf_spec *b;
        extern struct bpf_spec bpfs[MAXBPFSPECS];
	int i, j, count;

	if (fixed_ipproto.configured) {
		fixed_ipproto.tcp_bytes = fixed_ipproto.udp_bytes = 
		fixed_ipproto.icmp_bytes = fixed_ipproto.xtra_bytes = 0; 
	}

	if (fixed_tcp3.configured) {
		fixed_tcp3.port1_bytes = fixed_tcp3.port2_bytes = 
		fixed_tcp3.xtra_bytes = 0;
	}

	if (fixed_iprange1.configured) {
		fixed_iprange1.range1_bytes = 
		fixed_iprange1.extra_bytes = 0;
	}

	if (fixed_cast.configured) {
		fixed_cast.mcast_bytes = 
		fixed_cast.bcast_bytes = 
		fixed_cast.ucast_bytes = 
		fixed_cast.xtra_bytes = 0;
	}

	if (fixed_size.configured) {
		fixed_size.tiny = 
		fixed_size.small = 
		fixed_size.medium = 
		fixed_size.large = 0;
	}

	/* topN
	*/
	if (topn_ip.configured) {
		destroyList(&topn_ip.iplist);
		initList(&topn_ip.iplist);
	}

        if (topn_irc.configured) {
                destroyIrc(&topn_irc.irclist);
                initIrc(&topn_irc.irclist);
        }

	if (topn_syn.configured) {
		destroy_synlist(&topn_syn.synlist);
		init_synlist(&topn_syn.synlist);
	}

	if (topn_icmperror.configured) {
		destroy_icmplist(&topn_icmperror.icmplist);
		init_icmplist(&topn_icmperror.icmplist);
	}
        
        if (topn_port.configured) {
               destroy_plist(&topn_port.tcplist);
               destroy_plist(&topn_port.udplist);
               init_plist(&topn_port.tcplist);
               init_plist(&topn_port.udplist);
        }   

        if (topn_scans_input.configured) {
                destroyScanList(&topn_scans_input.iplist);
                initScanList(&topn_scans_input.iplist);
                destroyScanList(&topn_scans_output.iplist);
                initScanList(&topn_scans_output.iplist);
                initStats(&topn_scans_ip_stats);
        }

        if (topn_scans_ports_input.configured) {
                destroyScanList(&topn_scans_ports_input.iplist);
                initScanList(&topn_scans_ports_input.iplist);
                destroyScanList(&topn_scans_ports_output.iplist);
                initScanList(&topn_scans_ports_output.iplist);
                initStats(& topn_scans_ports_stats);

                destroyScanList(&topn_scans_ports_tcp_input.iplist);
                initScanList(&topn_scans_ports_tcp_input.iplist);
                destroyScanList(&topn_scans_ports_tcp_output.iplist);
                initScanList(&topn_scans_ports_tcp_output.iplist);
                initStats(& topn_scans_ports_tcp_stats);

                destroyScanList(&topn_scans_ports_udp_input.iplist);
                initScanList(&topn_scans_ports_udp_input.iplist);
                destroyScanList(&topn_scans_ports_udp_output.iplist);
                initScanList(&topn_scans_ports_udp_output.iplist);
                initStats(& topn_scans_ports_udp_stats);
        }

	if (any_bpf) {
		b = &bpfs[0];
		for ( i = 0; i < MAXBPFSPECS; i++, b++) {
	                if (b->configured == FALSE) {
       		        	break;
			}
                	count = b->prog_count;
			/*
			bzero(&b->bpf_counts[0], MAXBPFS*sizeof(COUNTER));
			*/
                	for ( j = 0; j < count; j++) {
				b->bpf_counts[j] = 0;
			}
			b->xtra_bytes = 0;
                }
	}

	bzero(&os, sizeof(struct ourstats));
}

