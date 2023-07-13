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


/* util.c -- misc auxiliary routines */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#ifdef	__STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "ourmon.h"

void
#ifdef	__STDC__
error(int perrno, char *fmt, ...)
#else
error(perrno, fmt, va_alist)
	int perrno;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char buf[200];

#ifdef	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)vsprintf(buf, fmt, ap);
	va_end(ap);

	(void)fprintf(stderr, "%s: ", program_name);
	if (perrno) perror(buf);
	else (void)fprintf(stderr, "%s\n", buf);
	(void)fflush(stderr);

	syslog(LOG_ERR, "ourmon pcap error %s: %m", buf);

	exit(1);
}

void
#ifdef	__STDC__
warning(int perrno, char *fmt, ...)
#else
warning(perrno, fmt, va_alist)
	int perrno;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char buf[200];

#ifdef	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)vsprintf(buf, fmt, ap);
	va_end(ap);

	(void)fprintf(stderr, "warning: %s: ", program_name);
	if (perrno) 
		perror(buf);
	else 
		(void)fprintf(stderr, "%s\n", buf);
	(void)fflush(stderr);
}

/*
 * Copy arg vector into a new buffer, concatenating arguments with spaces.
 */
char *
copy_argv(av)
	char **av;
{
	register char **p;
	u_int len = 0;
	char *buf, *src, *dst;

	p = av;
	if (*p == 0) return 0;
	while (*p) len += strlen(*p++) + 1;
	if ((buf = (char *)malloc(len)) == NULL) error(1, "copy_argv: malloc");
	p = av;
	dst = buf;
	while ((src = *p++) != NULL) {
		while ((*dst++ = *src++) != '\0');
		dst[-1] = ' ';
	}
	dst[-1] = '\0';

	return buf;
}

static struct proto_ent {
	char *name;
	u_short proto;
} proto_tab[] = {
	{ "tcp",  IPPROTO_TCP  },
	{ "udp",  IPPROTO_UDP  },
	{ "icmp", IPPROTO_ICMP },
	{ "egp",  IPPROTO_EGP  },
#ifndef IPPROTO_OSPF
#define IPPROTO_OSPF	89
#endif
	{ "ospf", IPPROTO_OSPF },
	{ "igmp", IPPROTO_IGMP },
#ifdef	IPPROTO_GGP
	{ "ggp",  IPPROTO_GGP  },
#endif
#ifdef	IPPROTO_ENCAP
	{ "encap",IPPROTO_ENCAP},
#endif
	{ "ip",   IPPROTO_IP   },
	{ "raw",  IPPROTO_RAW  },
	{ NULL, -1 },
};

char *
getprotoname(proto)
	u_short proto;
{
	register struct proto_ent *p;

	for (p = proto_tab; p->name != NULL; p++)
		if (proto == p->proto)
			return p->name;

	return NULL;
}

u_short
getprotonum(proto)
	char *proto;
{
	register struct proto_ent *p;

	for (p = proto_tab; p->name != NULL; p++)
		if (!strcasecmp(proto, p->name))
			return p->proto;

	return -1;
}

int resolveAddress(char *, struct sockaddr_in *);

/* resolve dns name or ip address as string to net order
 * socket "long"
 * 	returns: 0 - name could not be resolved
 *		 1
 */
int
resolveAddress(target, to)
char *target;		
struct sockaddr_in *to;
{
	struct hostent *hp;

	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(target);
	if (to->sin_addr.s_addr != (u_int)-1) {
		/*
		hostname = target;
		*/
		;
	}
	else {
		hp = gethostbyname(target);
		if (!hp)
			return(0);
		to->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
	}
	return(1);
}

/*
 * extractString linebuf - a string of form "...." output - same string sans
 * quotes max - max size of output buffer
 * 
 * we want to extract the string and toss the quotes
 * 
 * returns: NULL error.  ptr, if linebuf ptr advances ok
 */
char *
extractString(char *linebuf, char *output, int max)
{
	int             i = 0;
	int             l = strlen(linebuf);
	int             found = 0;
	char           *p = linebuf;

	if ((l - 2) >= max)
		return (NULL);

	/*
	 * skip ahead to 1st double quote
	 */
	while (l--) {
		if (*p == '"') {
			found = 1;
			break;
		}
		p++;
	}
	if (!found) {
		return (NULL);
	}
	found = 0;
	/*
	 * copy string into "value" buffer, leave out double quotes
	 */
	p++;
	l--;
	if (l == 0)
		return (NULL);
	while (l--) {
		*output++ = *p++;
		/*
		 * terminate on double quote.
		 */
		if (*p == '"') {
			*p = 0;
			*output = 0;
			found = 1;
			break;
		}
	}
	if (!found) {
		return (NULL);
	}
	return (p+1);
}


#define PIDFILE "/var/run/ourmon.pid"

void
storepid() {
	FILE *fp = fopen(PIDFILE, "w");
	
	/*
	 * No great loss if an error occurs -- the pid simply doesn't
	 * get stored.  Still should report it....   XXX
	 */
	if (fp) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
		return;
	}
	syslog(LOG_ERR, "could not open %s: %m", PIDFILE);
}
