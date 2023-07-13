/*
 *
 * ipanalyze.c
 *
 * middle-level function that extracts most of the useful
 * bits from an IP packet, and shoves them in a structure
 * for use by cascading filters later.
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
#include <pcap.h>
#ifdef	TIME_WITH_SYS_TIME
#include <time.h>
#endif

#include "ourmon.h"
#include  "filter.h"

u_char     etherbroadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*
 * based on minimal upper-header info for
 * ip packets, we call filters
 *
 * basic flow:
 *	parse for various errors
 *	if 1st packet of ip datagram
 *		tcp
 *		udp
 *		icmp
 *		other
 *
 *	ip_filter(on various stats) and return
 *
 *	error_goto -- assume "non" ip and/or outrageous errors
 */

void
analyze_ip(ip, length, caplength, eh)
	register struct ip *ip; /* pointer to ip portion */
	int length;		/* length - of ip portion minus link */
	int caplength;		/* actual length of packet including link */
	struct ether_header *eh;
{
	register iplen;
	int hlen;
	register u_char *cp;
	struct t_entry te;

	/* process ethernet info, just in case
	*/
	te.eflag = 0;
	if (fixed_cast.configured && eh) {
		te.eflag = 1;
		te.bflag = te.mflag = 0;	/* assume unicast */
		/* check if broadcast, else multicast, default
		 * to unicast
		 */
		if ( bcmp(&eh->ether_dhost, etherbroadcastaddr, 6) == 0) {
			te.bflag = 1;
		}
#ifdef SOLARIS
                else if ((eh->ether_dhost.ether_addr_octet[0] & 1) == 1) {
#else
		else if ((eh->ether_dhost[0] & 1) == 1) {
#endif
			te.mflag = 1;
		}
	}

	/* process for all packet types including errors
	*/
	if (fixed_size.configured && eh) {
		if (caplength <= 100) {
			fixed_size.tiny++;
		}
		else if (caplength <= 500) {
			fixed_size.small++;
		}
		else if (caplength <= 1000) {
			fixed_size.medium++;
		}
		else {
			fixed_size.large++;
		}
	}

	/* if longer than snap (never mind the goto),
	 * ignore it
	 * TBD. count errors.
	 */
	if ((u_char *)(ip + 1) > snapend)  {
		goto too_short;
		return;
	}

	if (length < sizeof(*ip)) {
		goto too_short;
		return;
	}

	/* length of ip + rest
	*/
	iplen = ntohs(ip->ip_len);

	if (iplen < 1)  {
		goto too_short;
		return;
	}

	/* actual length compared to value in ip hdr
	*/
	if (length < iplen) {
		goto too_short;
		return;
	}

	/*
	 * If this is fragment zero, hand it to the next higher level protocol
	 * to fetch ports or icmp type else it is fragmented ip datagram.
	 */
	te.psize = te.sport = te.dport = 0;
	if ((ntohs(ip->ip_off) & 0x1fff) == 0) {
		/* advance to high level protocol header */
		hlen = ip->ip_hl * 4;
		cp = (u_char *)ip + hlen;
		if (ip->ip_p == IPPROTO_TCP) {
			/* tcp header too short
			*/
			if (cp + sizeof(struct tcphdr) > snapend ||
			    iplen - hlen < sizeof(struct tcphdr)) {
				goto too_short;
			}
#ifdef LINUX
#define th_sport source
#define th_dport dest
#endif
			te.sport = ntohs(((struct tcphdr *)cp)->th_sport);
			te.dport = ntohs(((struct tcphdr *)cp)->th_dport);
			te.synflag = te.finflag = te.resetflag = te.ackflag = 0;
#ifdef LINUX
                        if (((struct tcphdr *)cp)->ack) {
				te.ackflag = 1;
			}

                        if (((struct tcphdr *)cp)->syn) {
				te.synflag = 1;
			}
                        else if (((struct tcphdr *)cp)->fin) {
				te.finflag = 1;
			}
                        else if (((struct tcphdr *)cp)->rst) {
				te.resetflag = 1;
			}
#else
                        if ((((struct tcphdr *)cp)->th_flags &
                                (TH_SYN))) {
				te.synflag = 1;
                        }
                        else if ((((struct tcphdr *)cp)->th_flags &
                                (TH_FIN))) {
				te.finflag = 1;
                        }
                        else if ((((struct tcphdr *)cp)->th_flags &
                                (TH_RST))) {
				te.resetflag = 1;
                        }
                        if ((((struct tcphdr *)cp)->th_flags &
                                (TH_ACK))) {
				te.ackflag = 1;
			}
#endif
			/* determine protocol payload size if any
			*/
#ifdef LINUX
                        te.p = cp + (((struct tcphdr *)cp)->doff * 4);
#else
                        te.p = cp + (((struct tcphdr *)cp)->th_off * 4);
#endif
                        te.psize = snapend - te.p;  /* length of L7 portion if any */

#ifdef DEBUG
                        if (debug) {
                                printf("TCP PKT: synflag %d, snapend %x, payload length %d\n", 
					te.synflag, snapend, te.psize);
                        }
#endif

		} else if (ip->ip_p == IPPROTO_UDP) {
			/* udp header too short
			*/
			if (cp + sizeof(struct udphdr) > snapend ||
			    iplen - hlen < sizeof(struct udphdr)) {
				goto too_short;
			}
#ifdef LINUX
#define uh_sport source
#define uh_dport dest
#endif
			te.sport = ntohs(((struct udphdr *)cp)->uh_sport);
			te.dport = ntohs(((struct udphdr *)cp)->uh_dport);

    			/* save payload size and set pointer to payload
                        */
                        te.p = cp + sizeof(struct udphdr);
                        te.psize = snapend - te.p;        /* length of L7 portion if any */

		} else if (ip->ip_p == IPPROTO_ICMP) {
			/* icmp header too short */
			if (cp + sizeof(struct icmp) > snapend ||
			    iplen - hlen < sizeof(struct icmp)) {
				goto too_short;
			}
			te.sport = ((struct icmp *)cp)->icmp_type;
			te.dport = ((struct icmp *)cp)->icmp_code;

    			/* save payload size and set pointer to payload
                        */
                        te.p = cp + sizeof(struct icmp);
                        te.psize = snapend - te.p;        /* length of L7 portion if any */
		}
	}

	te.src.s_addr = ip->ip_src.s_addr;
	te.dst.s_addr = ip->ip_dst.s_addr;
	te.proto = ip->ip_p;
	te.bytes = iplen;
	ip_filter(caplength, &te);
	return;

too_short:
	/* just count it
	*/
	non_ip_filter(caplength);

	return;
}
