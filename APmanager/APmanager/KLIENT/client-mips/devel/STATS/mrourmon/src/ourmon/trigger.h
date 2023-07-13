
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

#ifndef TRIGGER_H
#define TRIGGER_H

#include <pcap.h>
/*
#include "filter.h"
*/

/* max trigger array structures
*/
#define MAXTRIGGERS 10

/* trigger types
*/
#define TWORM 0
#define TOPNIP 1

struct trigger_info {
	int type;
	unsigned int state; 
	char *name;
	char *bpfexpr;
}; 

extern const struct trigger_info trigger_info[];

struct trigger_cap {
	int configured;
	int state;			/* each trigger state */
	int N_value;
	char filename[2048];		/* where to store capture dump */
	int count;			/* number of packets to capture */
 	int curcount;			/* counter */	
	int bpfop;			/* saved config-time bpf params */
	int snaplen;			/* saved config-time bpf params */
	int netmask;			/* saved config-time bpf params */
	struct bpf_program fcodes;	/* compiled bpf expression */
	pcap_dumper_t *pd_dump;		/* pcap file descriptor */
};

extern struct trigger_cap trigger_cap[MAXTRIGGERS];
extern unsigned int trigger_worm_state;
 
/* prototypes
*/
char *trigger_on(int, char *);
void trigger_off(int);
int trigger_work(struct pcap_pkthdr *, u_char *, int, int); 
int trigger_config(int, char *, char *, char *);
int trigger_bpf_config(int, int, int, int, char *);
char *trigger_dynbpf_compile(int, char *);

#endif
