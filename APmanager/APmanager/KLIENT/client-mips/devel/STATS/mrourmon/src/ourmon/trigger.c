/*
 * trigger.c - trigger capability 
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
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>

#include "trigger.h"
#include "ourmon.h"


#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

extern pcap_t *pd;

/* global trigger state.  If > 1 then some trigger
 * is set
 */
unsigned int trigger_worm_state=0;
struct trigger_cap trigger_cap[MAXTRIGGERS];

/* basic information about trigger types
*/
const struct trigger_info trigger_info[] = {
	{TWORM, 0x01, "tworm", "tcp[tcpflags] & tcp-syn != 0"},
	{TOPNIP, 0x02, "topn_ip", "ip"}, 
	{0, 0, NULL, NULL}, 
};

#ifdef TRIGGERDUMP
static	FILE *tfd=NULL;
#endif

static void trigger_bpf_compile(struct bpf_program *fcode,	/* output */
	int bpfop, int snaplen, unsigned long netmask, 
	char* bpfexpr /* input */);

/*
 * get system timestamp for constructing
 * unique trigger_on time filename
 */
void 
static
get_ts(char *ts)
{
	struct tm *tm;
	time_t t;
	
	t = time(NULL);
	tm = localtime(&t);
	sprintf(ts,"%02d.%02d.%04d|%02d:%02d:%02d",
			tm->tm_mon+1,
			tm->tm_mday,
			tm->tm_year + 1900,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);
}

/*
 * initialize a trigger capability
 *
 * called at monconfig time, therefore can
 * do a printf on an error.  If this idea is
 * not the case, error i/o needs to change.
 *
 * goal: store the value, count, name in the trigger
 *	structure.
 *
 * returns:
 *	0 - ok
 *     -1 - error
 */
int 
trigger_config(int type, char *value, char *count, char *name)
{
	char *dfile;

	if( type >= MAXTRIGGERS) {
		fprintf(stderr,
			"trigger_config called with type %d - too many triggers\n");
		return (-1); 
	}
	
	/* note: type is just an array index that is 
	 * determined manually at this point
	 */
	trigger_cap[type].N_value = atoi(value);
	trigger_cap[type].count = atoi(count);
	if (trigger_cap[type].count < 0) {
               fprintf(stderr,"trigger: count cannot be negative\n");
               return (-1);
	}
        if (trigger_cap[type].N_value < 0) {
               fprintf(stderr,"trigger: value cannot be negative\n");
               return (-1);
        }
        if(strlen(name) > (1022-1)) {
              fprintf(stderr,"trigger: dump file path too long\n");
              return (-1);
        }

        trigger_cap[type].configured = TRUE;
	dfile = trigger_info[type].name; 
        sprintf(trigger_cap[type].filename, "%s/%s", name, dfile);
	return 0;
}

/*
 * trigger_bpf_config
 *
 * Given a trigger capability structure, with trigger_config already
 * called.  Compile a bpf expression for determining
 * whether or not packets should be stored in the trigger file.
 *
 * This routine is called at monconfig time to give a trigger
 * capability a default bpf and needed bpf information.
 *
 * inputs: 
 *	bpfop, snaplen, netmask -- typical bpf params needed
 *	at config time.  
 *
 *	bpfexpr - the bpf expression, which more than likely is
 * 		in the default trigger_info struct above.
 */
int
trigger_bpf_config(int type, int bpfop, int snaplen, 
	int netmask, char *bpfexpr)
{
	if( trigger_cap[type].configured == FALSE ) {
		fprintf(stderr,"trigger_bpf: %s need trigger configured\n",
				trigger_info[type].name);
		return (-1);		 
	}

	/* save these bpf compile params in case they are needed
	* for dynamic instance
	*/
	trigger_cap[type].bpfop = bpfop;
	trigger_cap[type].snaplen = snaplen;
	trigger_cap[type].netmask = netmask;

	/* no particular bpf expression needed so just use default
	 * which is already stored in local trigger_info structure.
	*/
	if( bpfexpr == NULL ) {
		bpfexpr = trigger_info[type].bpfexpr;
	}

	trigger_bpf_compile(
			&trigger_cap[type].fcodes, 
			bpfop, 
			snaplen, 
			netmask, 
			bpfexpr);

	return 0;
}

/*
 * actually do the bpf expression compile
 * This function is designed to be called from monconfig.
 */
static
void 
trigger_bpf_compile(struct bpf_program *fcodes,	/* output */
	int bpfop, int snaplen, unsigned long netmask, 
	char* bpfexpr /* input */)
{
	int i,gotone=0;
	extern int debug;

	if(debug) {
		printf("trigger_bpf_config_more: expr %s", bpfexpr);
	}
 
	/* compile the bpf
	*/
	(void) 
	local_pcap_compile_nopcap(snaplen, bpfop, fcodes, bpfexpr, 0, netmask, 1);
} 

/* function for dynamic bpf expression compile
 *
 * assumptions:
 *	monconfig made call to set up static bpf info.
 * returns:
 *	string message for elog.  does NOT exit.
 *	NULL - ok.
*/
char *
trigger_dynbpf_compile(int type, 
	char* bpfexpr /* input */)
{
	static char sbuf[2048];
	int rc;

	if( trigger_cap[type].configured == FALSE ) {
		sprintf(sbuf, "trigger_dynbpf_compile: runtime error for type %s: expr %s\n",
				trigger_info[type].name, bpfexpr);
		return (sbuf);
	}
	
	/* compile the bpf
	*/
	rc = 
	local_pcap_compile_nopcap(
		trigger_cap[type].snaplen, 
		trigger_cap[type].bpfop, 
		&trigger_cap[type].fcodes, 
		bpfexpr, 0, 
		trigger_cap[type].netmask, 
		0);

	if (rc < 0) {
		sprintf(sbuf, "trigger_dynbpf_compile: runtime pcap compile error on %s\n",
			bpfexpr);
		return(sbuf);
	}

	return(NULL);
}

/* turn a trigger capability on
 *
 * input: type
 * output: fname if open worked.  
 *	error return string is direct.
 *
 * returns:
 *	char * string on error.
 *	else NULL
*/
char *
trigger_on(int type, char *fname)
{
	int i;
	unsigned int state;
	char *tri_name;
	char ts[100];
	static char filename[1024];
	static char sbuf[2048];
	
	/* get per trigger state
	*/
	state = trigger_info[type].state;
#ifdef TRIGGERDUMP
	fprintf(tfd, "state: %d : %d : %d\n", trigger_worm_state,state,trigger_worm_state&state);
#endif
	/* if corresponding trigger not on in global state variable
	*/
	if ( (trigger_worm_state & state) == 0 ) {
#ifdef TRIGGERDUMP
		fprintf(tfd, "trigger_type: %s is set\n", tri_name);
#endif
		/* set the corresponding bit of state
		*/
		trigger_worm_state |= state;
	}
	else {
		sprintf(sbuf, "trigger_on: error, trigger is still on\n");
		return sbuf;
	}

#ifdef TRIGGERDUMP
		fprintf(tfd, "state: %d\n", trigger_worm_state);
#endif
	tri_name = trigger_info[type].name; 

	/* check for idiotic error.  this should not happen
	*/
	if( trigger_cap[type].configured == FALSE ) {
		sprintf(sbuf, "trigger_on: %s not configured\n",tri_name);
		return (sbuf);
	}

#ifdef TRIGGERDUMP
	tfd = fopen("/tmp/triggerdump.txt", "a+");
	fprintf(tfd, "trigger_type: %s is on\n", tri_name);
#endif

	memset(ts, 0, sizeof(ts));
	memset(filename, 0, sizeof(filename));
	
	get_ts(ts);
	if( strlen(ts) == 0 ) {
		sprintf(sbuf,"trigger_on: %s timestamp failed\n",tri_name);
		return (sbuf); 
	}

	/* set the per struct_trigger value to on
	*/
	trigger_cap[type].state = 1;

	/* only capture count packet,set the count
	*/

	trigger_cap[type].curcount = trigger_cap[type].count; 

	/* setup filename = instance.<timestamp>.dmp
	*/
	sprintf(filename,"%s.<%s>.dmp",
		trigger_cap[type].filename,ts);

	/* associate the pcap desc with the file
	*/
	if((trigger_cap[type].pd_dump=pcap_dump_open(pd,filename))
			 == NULL) {
		sprintf(sbuf,
			"trigger_on:pcap_dump_open failed: %s\n",	
			pcap_geterr(pd));
#ifdef TRIGGERDUMP
		fprintf(tfd,"pcap_dump_open failed\n");
		fclose(tfd);
#endif
		return (sbuf);
	}			
	/* return the complete filename via a pointer
	*/
	strncpy(fname, filename, strlen(filename));

	/* return NULL to mean no error
	*/
	return (NULL);
}


/*
 * if the trigger is on, turn it off
 */
void
trigger_off(int type)
{
	unsigned int state;

	state = trigger_info[type].state;
	if( (trigger_worm_state & type) == 1) {
		trigger_worm_state &= ~state;
		/* TBD. fix this. if count is exhausted do the close,
		 * don't wait for trigger_off
		 */
		pcap_dump_close(trigger_cap[type].pd_dump);
		trigger_cap[type].curcount = 0;
		trigger_cap[type].state = 0;
#ifdef TRIGGERDUMP
		tfd = fopen("/tmp/triggerdump.txt","a+");
		if ( tfd == NULL ) {
			syslog(LOG_ERR,"triggerdump.txt: open failed\n");
			exit(1);
		}
		fprintf(tfd, "trigger_off called \n");
		fclose(tfd);
#endif	
	}		
}

/* actually store a trigger packet
*/
int 
trigger_work(struct pcap_pkthdr *h, u_char *p, int caplen, int packet_len)
{
	int i,j;
	struct bpf_program *b;
	
#ifdef TRIGGERDUMP
	tfd = fopen("/tmp/triggerdump.txt", "a+");
	if(tfd==NULL)
	{
		syslog(LOG_ERR,"work: triggerdump open failed\n");
		exit(1);
	}
#endif
	/* TBD. make this work more efficiently by not looping through
	 * all capabilities because you know only the first N were used.
	 * i.e., track the number of capabilities actually used at config time.
	 */
	for( i = 0; i < MAXTRIGGERS; i++) {
		if ( trigger_cap[i].state == 1) {
#ifdef TRIGGERDUMP
	fprintf(tfd, "trigger_type: %d is working\n",i);
#endif
			/* if the count is exhausted, don't store any packets
			 * TBD. if this is true close the pcap file
			*/
			if( trigger_cap[i].curcount <= 0) {
				continue;
			}
			b = &(trigger_cap[i].fcodes);
			/*failure to match*/
			if( bpf_filter(b->bf_insns, p, 
				packet_len, caplen) == 0 ) {
				;
	#ifdef TRIGGERDUMP
				fprintf(tfd, "trigger_type: %d failed to match\n",i);
	#endif
			} else {
				/* TBD. return an error here somehow
				*/
				pcap_dump((u_char*)trigger_cap[i].pd_dump, h, p);
				trigger_cap[i].curcount--;
	#ifdef TRIGGERDUMP
				fprintf(tfd,"%d %d one more\n", i,trigger_cap[i].curcount);
	#endif
			} /*end if(bpf_filter)*/			
		}  /* if trigger cap. state is on */ 
	} /*end for*/
	
#ifdef TRIGGERDUMP
	fclose(tfd);
#endif
} 

