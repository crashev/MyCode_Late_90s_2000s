/*
 * ircscan.c - build up irc info
 * 
 * snaplen for this to work in any useful manner
 * should be on order of 128 .. 256 bytes.
 *
 * this is work in progress. not done yet.
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

#include "ourmon.h"	/* struct t_entry */
#include "ircscan.h"

#include "ehash.c"

void isjoin(struct irclist *, struct in_addr, struct in_addr, unsigned short, char *, int );
void isping(struct irclist *, struct in_addr, struct in_addr, unsigned short);
void ispong(struct irclist *, struct in_addr, struct in_addr, unsigned short);
struct ircchan * chanStore(struct irclist *, struct in_addr, struct in_addr, unsigned short, char *, int); 

#define BLANK 0x20

extern struct topn_irc topn_irc;

/* called before list can be used
*/
void
initIrc(struct irclist *l)
{
	bzero(l, sizeof(struct irclist));
}

destroyIrc(struct irclist *l)
{
	/* nothing to do yet
	*/
}


/*
 * ircscan:
 * overview:
 *	scan L7 payload for JOIN/PING/PONG irc messages
 *	goal is to determine where irc connectivity exists.
 *	Interested in channel names with JOINs, and ip src
 *	relationships with JOIN/PING/PONG.
 *
 * privacy note:  no keys exposed here, and no internal message
 *	guts exposed either.
 *	
 * assumptions:
 *	only called for TCP packets.
 *	only called if there is any L7 payload at all.  not for TCP syns  
 *		without payload.
 *
 * algorithm:
 *	scan first, then store information.
 *
 *	scan to bring up into JOIN/PING/PONG that are formatted in
 *	irc manner.  bail as quickly as possible if not an IRC message.
 *	E.g., html with SHOPPING.
 *
 *	join/ping/pong handled separately.
 *
 * rough lex plan:
 *	get JOIN/PING/PONG messages
 *
 * note: CRNL (0xa0xd)
 *
 * JOIN channel 
 * [NONE|NONBLANKS BLANKS]JOIN<BLANKS><#|& NONBLANKS><BLANKS | CRNL>
 *  j1	                  j2   j3      j4             j5
 *
 * PING
 * [NONE|NONBLANKS BLANKS]PING<BLANKS | CRNL>
 *  p1                    p2   p3
 * PONG
 * [NONE|NONBLANKS BLANKS]PONG<BLANKS | CRNL>
 *  P1                    P2   P3
 */
void
ircscan(struct irclist *l, struct t_entry *te) 
{
	extern int debug;
	char channame[512];				/* max size of channel name */
	int chansize; 
	char *cpt;
	unsigned long ipsrc, ipdst;
	unsigned short dport;
	unsigned char *p;
	int psize;

/*
	if (debug) {
		printf("IRCSCAN called \n");
	}
*/
	p     = te->p;
	psize = te->psize;

	/* go to state 2 if NONE
	*/
	if (*p == 'J')
		goto state2;
	if (*p == 'P')
		goto state2;

	/* cycle thru NONBLANKS if any
	*/
	while(psize) {
		if (*p == BLANK)
			goto blanks1;
		p++;
		psize--;
	}
	return;

	/* cycle thru BLANKS if any
	*/
blanks1:
	while(psize && (*p == BLANK)) {
		p++;
		psize--;
	}
	if (psize && ((*p == 'J') || (*p == 'P')))
		goto state2;
	return;

state2:
	/* conservative bail here.  e.g., JOIN and CRNL are minimal
	*/
	if (psize < 6) {
		return;
	}

	/* JOIN ? */
	if ((*p == 'J') && (*(p+1) == 'O') && (*(p+2) == 'I') && (*(p+3) == 'N')) {
		p += 4;
		psize = psize - 4;
		if (psize == 0)
			return;

		/* at this point must have blank
		*/
		if (*p != BLANK)
			return;

		/* move to word following blanks
		*/
		while (psize && (*p == BLANK)) {
			psize--;
			p++;
		}
		if (psize == 0) {
			return;
		}
		/* now at channel name, save it in local buffer
		*/
		if ((*p == '#') || (*p == '&')) {
			chansize = 0;
			cpt = &channame[0];
			while (psize && (*p != BLANK) && (*p != '\r')) {
				*cpt++ = *p++;
				chansize++;
				if (chansize > 510)
					return;
				psize--;
			}
			if (psize == 0) {
				return;
			}
			channame[chansize] = 0;
			isjoin(l, te->src, te->dst, te->dport, channame, chansize);
		}
		return;
	}

	/* PING */
	else 
	if ((*p == 'P') && (*(p+1) == 'I') && (*(p+2) == 'N') && (*(p+3) == 'G')) {
		/* at this point must have blank or CRNL.  PING has at least one word
	         * behind it in all cases, although we are not absorbing it
		*/
		p += 4;
		psize = psize - 4;
		if (psize == 0)
			return;
		if (*p == BLANK) {
			isping(l, te->src, te->dst, te->dport);
			return;
		}
		if (psize == 2 && ((*p == '\r') && (*p == '\n'))) {
			isping(l, te->src, te->dst, te->dport);
			return;
		}
		return;
	}

	/* PONG */
	else 
	if ((*p == 'P') && (*(p+1) == 'O') && (*(p+2) == 'N') && (*(p+3) == 'G')) {
		/* at this point must have blank or CRNL
		 * POING has at least one word behind it in all cases.
		*/
		p += 4;
		psize = psize - 4;
		if (psize == 0)
			return;
		if (*p == BLANK) {
			ispong(l, te->src, te->dst, te->dport);
			return;
		}
		if ((psize == 2) && ((*p == '\r') && (*p == '\n'))) {
			ispong(l, te->src, te->dst, te->dport);
			return;
		}
		return;
	}

	/* PRIVMSG */
	else
	if ((*p == 'P') && (*(p+1) == 'R') && (*(p+2) == 'I') && (*(p+3) == 'V') && (*(p+4) == 'M') && (*(p+5) == 'S') && (*(p+6) == 'G')) {
		p += 7;
		psize = psize - 7;
		if (psize == 0)
			return;
		if (*p == BLANK) {
			if (debug) {
				char buf[1024];
				sprintf(buf, "%s", inet_ntoa(te->src));
				printf("\tIRCPRIVMSG: %s %s %d\n", buf, inet_ntoa(te->dst), dport);
			}
			l->totalprivmsgs++;
			return;
		}
	}
	return;	
}


void
isjoin(struct irclist *l, struct in_addr ipsrc, struct in_addr ipdst, unsigned short dport,
	char *chan, int cs)
{
	struct ircchan *nchan;
	extern int debug;

	if (debug) {
		char buf[1024];
		sprintf(buf, "%s", inet_ntoa(ipsrc));
		printf("\tIRCJOIN: %s %s %d %s %d\n", buf, inet_ntoa(ipdst), dport, chan, cs);
	}

	/* update total joins
	*/
	l->totaljoins++;

	/* store the channel
	nchan = chanStore(l, ipsrc, ipdst, dport, chan, cs);
	*/

	/* now store both ip addresses.
	 * note: ip dst here is a server by definition.
	iplookup(ipsrc);
	*/
}

void
isping(struct irclist *l, struct in_addr ipsrc, struct in_addr ipdst, unsigned short dport)
{
	extern int debug;

	if (debug) {
		char buf[1024];
		sprintf(buf, "%s", inet_ntoa(ipsrc));
		printf("\tIRCPING: %s %s %d\n", buf, inet_ntoa(ipdst), dport);
	}
	l->totalpings++;
}

void
ispong(struct irclist *l, struct in_addr ipsrc, struct in_addr ipdst, unsigned short dport)
{
	extern int debug;

	if (debug) {
		char buf[1024];
		sprintf(buf, "%s", inet_ntoa(ipsrc));
		printf("\tIRCPONG: %s %s %d\n", buf, inet_ntoa(ipdst), dport);
	}

	l->totalpongs++;
}

/*
 * we have a join.  extract the channel information,
 * as well as the server/client association
 */

struct ircchan *
chanStore(struct irclist *l, struct in_addr ipsrc, struct in_addr ipdst, unsigned short dport, char *chan, int cs) 
{
	struct ircchan *nchan;
	int mod;
	int foundit = 0;

	/* look it up in channel hash list 
	 * we either find it or we do not find it.
	*/
        nchan = l->chanhash[mod=(ehash((unsigned char *)chan, cs) % IRCHASH)];
        for (; nchan != NULL ; nchan=nchan->next) {
		/* note: match here only on channel name 
		*/
		if (nchan && (strcmp(chan, nchan->name) == 0)) {
			/* found node */
			foundit = 1;
			break;
		}
	}

	/* if we found it, update it
	*/

	if (foundit) {
		nchan->hits++;
		return(nchan);
	}

	/* didn't find it, create it, stick it in, init it
	*/
        nchan = (struct ircchan *) malloc(sizeof(struct ircchan));

        /* out of space. ouch.
        */
        if (nchan == NULL) {
                syslog(LOG_ERR, "topn_irc: malloc out of space, channel count %d\n", l->chancount);
                return;
        }

	/* store the name 
	*/
	nchan->name = (char *) malloc((char *) cs+1);
	strncpy(nchan->name, chan, cs);

	nchan->hits = 1;
	return(nchan);
}

printIrc(FILE *fd, struct irclist *l)
{
	fprintf(fd, "ircstats: %d: %d: %d: %d:\n", l->totaljoins, l->totalpings, l->totalpongs, l->totalprivmsgs);
}

