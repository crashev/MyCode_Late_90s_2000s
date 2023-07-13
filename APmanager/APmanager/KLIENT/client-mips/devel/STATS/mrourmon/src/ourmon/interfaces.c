/*
 *	Copyright (c) 1993-1997 JSC Rinet, Novosibirsk, Russia
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 * Redistribution in binary form may occur without any restrictions.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' WITHOUT ANY WARRANTIES OF ANY KIND.
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


/* interfaces.h -- network interface data-link level routines */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/param.h>
#include <sys/types.h>
#ifdef	HAVE_SYS_MBUF_H
#include <sys/mbuf.h>
#endif
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef	HAVE_NET_SLIP_H
#include <net/slip.h>
#include <net/slcompress.h>
#endif
#include <syslog.h>

#include <pcap.h>
#include "ourmon.h"
#include "trigger.h"

#include <stdio.h>
#include <stdlib.h>

u_char *snapend;

/* Link-level header length */
#ifndef	ETHER_HDRLEN
#define	ETHER_HDRLEN	sizeof(struct ether_header)
#endif

#ifndef	SLIP_HDRLEN
#ifdef	SLC_BPFHDR
#define	SLIP_HDRLEN	SLC_BPFHDR	/* if _BSDI_VERSION >= 199510 */
#else
#define	SLIP_HDRLEN	16	/* bpf slip header length */
#endif
#endif

#ifndef	PPP_HDRLEN
#ifdef	SLC_BPFHDR
#define	PPP_HDRLEN	SLC_BPFHDR	/* if _BSDI_VERSION >= 199510 */
#else
#define	PPP_HDRLEN	4	/* sizeof(struct ppp_header) */
#endif
#endif

#ifdef	DLT_C_HDLC
#ifndef	CHDLC_HDRLEN
#define	CHDLC_HDRLEN	4	/* sizeof(struct cisco_hdr) */
#endif
#endif

#ifndef	NULL_HDRLEN
#define	NULL_HDRLEN	4	/* loopback header length */
#endif

extern int any_bpf;
extern unsigned int trigger_worm_state;	

/*
 * interface dependent routings, basically called thru jump table
 * (DLT type).  if_ether is for ethernet.
 * user - is string passed from main level thru pcap loop routine.
 * h - pcap header structure
 * p - the packet itself
 *	Sub-variables:
 *	length - length of the packet
 *	caplen - bytes actually captured.
 */
void
if_ether(user, h, p)
	char *user;
	struct pcap_pkthdr *h;
	register u_char *p;
{
	extern int debug;

	u_int caplen = h->caplen;	/* amt. actually captured */
	u_int length = h->len;		/* length of packet */

#ifdef DEBUG
	if (debug) {
		printf("ethernet: caplen %d, length %d\n", caplen, length);
	}
#endif
	if (caplen < ETHER_HDRLEN) 
		return;

	/* ethernet header still intact 
	*/
	if (any_bpf) {
		analyze_bpf(p, caplen, length);
	}
	/* snapend is pointer to end of packet buffer
	*/
	snapend = p + caplen;
	if ( ntohs(((struct ether_header *)p)->ether_type) == ETHERTYPE_IP) {
		analyze_ip(p + ETHER_HDRLEN, length - ETHER_HDRLEN, length,
			(struct ether_header *)p);
	}
	
	if (trigger_worm_state != 0)  {	
		trigger_work(h, p, caplen, length);
	}
}

void
if_slip(user, h, p)
	char *user;
	struct pcap_pkthdr *h;
	register u_char *p;
{
	u_int caplen = h->caplen;
	u_int length = h->len;

	if (caplen < SLIP_HDRLEN) return;

	if (any_bpf) {
		analyze_bpf(p, caplen, length);
	}

	snapend = p + caplen;
	analyze_ip(p + SLIP_HDRLEN, length - SLIP_HDRLEN, length, NULL);

	if (trigger_worm_state != 0)  {	
		trigger_work(h, p, caplen, length);
	}
}

void
if_ppp(user, h, p)
	char *user;
	struct pcap_pkthdr *h;
	register u_char *p;
{
	u_int caplen = h->caplen;
	u_int length = h->len;
	register hdrlen = 0;
	u_short type;
	u_char *packetp;

	if (caplen < PPP_HDRLEN) return;

	packetp = p;
	snapend = p + caplen;

#ifdef	SLC_BPFHDRLEN
	p += SLC_BPFHDRLEN;	/* pointer to link level header */
#endif
	/* PPP address and PPP control fields may be present (-acfc) */
	if (p[0] == 0xff && p[1] == 0x03) {
		p += 2;
		hdrlen += 2;
	}
	/* Retrive the protocol type */
	if (*p & 01) {	/* Compressed protocol field (pfc) */
		type = *p++;
		hdrlen++;
	} else {	/* Un-compressed protocol field (-pfc) */
		type = ntohs(*(u_short *)p);
		p += 2;
		hdrlen += 2;
	}
	if (type == 0x21) {	/* IP protocol */
#ifdef	SLC_BPFHDR
		p = packetp + SLC_BPFHDR;	/* skip bpf pseudo header */
		hdrlen = SLC_BPFHDR;
#endif
		if (any_bpf) {
			analyze_bpf(p, caplen, length);
		}
		analyze_ip(p, length - hdrlen, length, NULL);
	}

	if (trigger_worm_state != 0)  {	
		trigger_work(h, p, caplen, length);
	}
}

#ifdef	DLT_C_HDLC
void
if_chdlc(user, h, p)
	char *user;
	struct pcap_pkthdr *h;
	register u_char *p;
{
	u_int caplen = h->caplen;
	u_int length = h->len;

	if (caplen < CHDLC_HDRLEN) return;

	if (any_bpf) {
		analyze_bpf(p, caplen, length);
	}
	snapend = p + caplen;
	if (ntohs(*(u_short *)(p + 2)) == 0x0800) /* IP protocol */
		analyze_ip(p + CHDLC_HDRLEN, length - CHDLC_HDRLEN, length,
			NULL);

	if (trigger_worm_state != 0)  {	
		trigger_work(h, p, caplen, length);
	}
}
#endif

#ifdef	DLT_RAW
void
if_rawip(user, h, p)
	char *user;
	struct pcap_pkthdr *h;
	register u_char *p;
{
	u_int caplen = h->caplen;
	u_int length = h->len;

	if (any_bpf) {
		analyze_bpf(p, caplen, length);
	}
	snapend = p + caplen;
	analyze_ip(p, length, length, NULL);

	if (trigger_worm_state != 0)  {	
		trigger_work(h, p, caplen, length);
	}
}
#endif

void
if_null(user, h, p)
	char *user;
	struct pcap_pkthdr *h;
	register u_char *p;
{
	u_int caplen = h->caplen;
	u_int length = h->len;
  	u_int family;
	
	if (debug) {
		printf("lo0: caplen %d, length %d\n", caplen, length);
	}

	/* ??? will this work */
	if (any_bpf) {
		analyze_bpf(p, caplen, length);
	}

	memcpy(&family, p, sizeof(family));
	snapend = p + caplen;
	if (family == AF_INET)
		analyze_ip(p + NULL_HDRLEN, length - NULL_HDRLEN, length, NULL);

	if (trigger_worm_state != 0)  {	
		trigger_work(h, p, caplen, length);
	}
}

/*
 * hardware dependent functions table.
 */
static struct if_func {
	void (*f)();
	int type;
} if_funcs[] = {
	{	if_ether,	DLT_EN10MB	},	/* Ethernet */
#ifdef	DLT_IEEE802
	{	if_ether,	DLT_IEEE802	},	/* IEEE 802 */
#endif
	{	if_slip,	DLT_SLIP	},	/* SLIP */
#ifdef	DLT_SLIP_BSDOS
	{	if_slip,	DLT_SLIP_BSDOS	}, /* libpcap stupid fake */
#endif
	{	if_ppp,		DLT_PPP		},	/* PPP */
#ifdef	DLT_PPP_BSDOS
	{	if_ppp,		DLT_PPP_BSDOS	}, /* libpcap stupid fake */
#endif
#ifdef	DLT_C_HDLC
	{	if_chdlc,	DLT_C_HDLC	},	/* Cisco HDLC */
#endif
#ifdef	DLT_RAW
	{	if_rawip,	DLT_RAW		},	/* raw IP */
#endif
	{	if_null,	DLT_NULL	},	/* loopback */
	{ NULL, 0 },
};

/*
 * Assign data link type to interface function.
 */
pcap_handler
lookup_if(type)
	int type;
{
	struct if_func *p;

	for (p = if_funcs; p->f != NULL; ++p)
		if (type == p->type) return p->f;
	error(0, "unknown data link type 0x%x", type);

	/* NOTREACHED */
	return 0;
}
