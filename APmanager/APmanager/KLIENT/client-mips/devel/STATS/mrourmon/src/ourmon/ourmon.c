/*
 * ourmon - a network monitoring and anomaly detection system.
 */

/*
 * ourmon was originally based in part on trafshow.
 * At this point, the heritage is non-obvious but is acknowledged.
 */
/*
 *      Copyright (c) 1993-1997 JSC Rinet, Novosibirsk, Russia
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 * Redistribution in binary form may occur without any restrictions.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' WITHOUT ANY WARRANTIES OF ANY KIND.
 */

/*
 * Copyright (c) 2001,2003,2004 Portland State University 
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
#ifdef LINUX
#include <sys/ioctl.h>
#else
#include <sys/filio.h>
#endif
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <syslog.h>
#include <stdarg.h>
 

#include "ourmon.h"
void cleanup(int arg);
void ignorehup(int arg);

#define DEFAULT_CONF  "/etc/ourmon.conf"
#define DEFAULT_MONFILE "/tmp/mon.lite"

int snaplen = DEFAULT_SNAPLEN;	/* length of saved portion of packet */

/* global variables */
char *program_name;		/* myself */
char *device_name;		/* network interface name */
char *device2_name;		/* network interface name */
char *conf_name;		/* name of config file */
char *mon_name;			/* name of output/mon file */
char *readfile;			/* use a tcpdump save file */
int filemode = 0;		/* true if we are reading input from a save file */
int debug = 0;

int alarm_flag = 0;		/* flag -- screen refresh requested */
int Oflag = 0;

int timeout = 1000;		/* ms */

int g_bufsize = 0;		/* pcap/bpf input bufsize */

/* pcap handle */
pcap_t *pd;
static pcap_t *pd2;

/* if true no alarm as -c is 
 * in use and hopefully -r 
 */
int cntmode = 0;		

#define DEFAULT_ALARM 30
int alarm_secs = DEFAULT_ALARM; /* time to write out mon output file */

int addr_size, proto_size, bytes_size, cps_size, count_size;

void ourmon_twodevices(int op, int op2);

int
main(argc, argv)
	int argc;
	char **argv;
{
	int notready = 1;
	int op, op2, cnt;
	bpf_u_int32 localnet, netmask;
	bpf_u_int32 localnet2, netmask2;
	char *cp, *infile, *expr, ebuf[PCAP_ERRBUF_SIZE];
	struct bpf_program fcode;
	extern char *optarg;
	extern int optind, opterr;
	void usage(), onalarm(), onwinch(), vers();
	/*
	void sigchild();
	*/
	extern int abort_on_misalignment();
	extern pcap_handler lookup_if();

	localnet = netmask = 0;
	localnet2 = netmask2 = 0;
	cnt = -1;
	mon_name = DEFAULT_MONFILE;
	conf_name = DEFAULT_CONF;
	device_name = NULL;
	device2_name = NULL;
	infile = NULL;
	if ((cp = (char *)strrchr(argv[0], '/')) != NULL)
		program_name = cp + 1;
	else	program_name = argv[0];

	if (abort_on_misalignment(ebuf) < 0) 
		error(0, ebuf);

	while ((op = getopt(argc, argv, "r:t:a:c:i:I:f:m:s:hd")) != EOF) {
		switch (op) {
		/* reset default alarm time in seconds
		*/
		case 'r':
			filemode = 1;
			cntmode = 1;			/* no alarms */
			readfile = optarg;
			break;
		case 'a': 
			alarm_secs = atoi(optarg);
			if (alarm_secs < 1) {
				fprintf(stderr,"alarm time > 0\n");
			}
			break;
		/* exit after recv. #n packets
		*/
		case 't':
			timeout = atoi(optarg);
			break;
		case 'c':
			cnt = atoi(optarg);
			cntmode = 1;			/* no alarms */
			if (cnt < 1) usage();
			break;
		case 'i':
			device_name = optarg;
			break;
		case 'I':
			device2_name = optarg;
			break;
		case 'f':
			conf_name = optarg;
			break;
		case 'm':
			mon_name = optarg;
			break;
		case 'd':
			debug = 1;
			break;
		case 's':
			snaplen = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
		}
	}


	/* if no device specified, look for one with pcap_lookupdev
	*/
	if (device_name == NULL &&
	    (device_name = pcap_lookupdev(ebuf)) == NULL)
		error(0, ebuf);

	/* Attach pcap to the network interface 
	 * device_name - device to put in prom. mode
 	 * snaplen - number of bytes to capture.  
	 * note: 
 	 * promiscuous mode set to 1.
	 * note timeout is read timeout in ms (1 sec).
	 * psu mod: pcap_open_live2 passes bpf buffer size.
	 *	might use it here.
	 * note2:
	 * this routine really gets the bpf device up ...
	 */
	if (filemode) {
		if ((pd = pcap_open_offline(readfile, ebuf)) == NULL) {
			error(0, ebuf);
		}
	}
	else if ((pd = pcap_open_live(device_name, snaplen, 1, timeout, ebuf)) == NULL) {
		error(0, ebuf);
	}
		
	/* if we have a 2nd device,  open a second pcap handle
	 */
	if (device2_name) {
		if ((pd2 = pcap_open_live(device2_name, snaplen, 1, 1000, ebuf)) == NULL)
		error(0, ebuf);
	}

	/* returns specified snapshot length
	*/
	if ((op = pcap_snapshot(pd)) > snaplen) 
		snaplen = op;

	/* returns link layer type (DLT)
	*/
	op = pcap_datalink(pd);
	if (device2_name)
		op2 = pcap_datalink(pd2);

#ifdef LIVEDANGEROUSLY
	/* limit to ethernet or lo0 at this time
	*/
	if (op != DLT_EN10MB  ) {
		printf("op is %d\n", op);
		error(0, "interface %s not an Ethernet", device_name);
	}
#endif

	/* determine network number and mask
	*/
	if (pcap_lookupnet(device_name, &localnet, &netmask, ebuf) < 0) {
		warning(0, ebuf);
	}

	/* delayed read and check of config file.
	 * we may need bpf info for config bpfs
	*/
	configRead(conf_name, op, snaplen, netmask);

	if (device2_name) {
		if (pcap_lookupnet(device2_name, &localnet2, &netmask2, 
			ebuf) < 0) {
			warning(0, ebuf);
		}
	}

	/* set expression pointer, if any
	*/
	expr = copy_argv(&argv[optind]);

	/* bpf compile
	*/
	if (pcap_compile(pd, &fcode, expr, Oflag, netmask) < 0 ||
	    pcap_setfilter(pd, &fcode) < 0)
		error(0, pcap_geterr(pd));

	/* ignored because we do not want to exit at boot
	*/
	(void) signal(SIGHUP, ignorehup);

	/* exit on SIGINT, SIGQUIT
	*/
	(void) signal(SIGINT, cleanup);
	(void) signal(SIGQUIT, cleanup);

	(void) signal(SIGALRM, onalarm);
#ifdef NOTUSED
        (void) signal(SIGCHLD, sigchild);  /* signal handler for tcpdump trigger */
#endif

	/* open syslog
	*/
        openlog("ourmon", LOG_CONS | LOG_PID , LOG_DAEMON);
        syslog(LOG_ERR, "ourmon is running");
#ifndef LINUX
        /* On BSD: this can help catch libpcap mismatch with old libpcap that does not
         * pay attention to sysctl bpf buffer size settings
        */
        syslog(LOG_ERR, "bpf_bufsize %d: snaplen %d, timeout %d\n", get_bpfbufsize(), snaplen, timeout);
#endif

#ifdef NOPE
#ifdef DAEMON
	/* if not debug become a daemon
	*/
        if (debug == 0) {
                if (daemon(1,0) < 0) {
                        syslog(LOG_ERR, "ourmon/main(): daemon (%m)\n");
                        exit(1);
                }
        }
#else
	if (fork() == 0) {
		exit(0);
	}
#endif
#endif

	storepid();

	if (cntmode == 0) {
		/* initialize ability to block signal for
		 * critical sections
		 */
		init_block();

		/* schedule alarm in alarm period secs
		*/
		alarmOn();
	}

	/* pcap_loop
	 *	pd - pcap handle
	 *	cnt - # of packets to read, then exit, if -1, will loop forever
 	 *	lookup_if - callback
	 */
onemoretime:
	if (device2_name == NULL) {
		if (pcap_loop(pd, cnt, lookup_if(op), NULL) < 0) {
			syslog(LOG_ERR, "ourmon pcap error: %s", pcap_geterr(pd));
			/*
			error(0, pcap_geterr(pd));
			*/
		}
		syslog(LOG_ERR, "ourmon pcap_loop exit : %s", pcap_geterr(pd));
		pcap_close(pd);
	}
	else  {
		ourmon_twodevices(op, op2);
	}
	/* if there is some whacky boot-time race condition
	 * delay and try again once
	 */
	if (notready) {
		notready = 0;
		sleep(10);
		goto onemoretime;
	}

	/* just a final report in case the count is not infinite
	 * this is used for feeding a capture file to ourmon to
	 * generate a big lump of stats with no alarm period
	*/
	write_report(mon_name, 0, 0, 0, 0);

	cleanup(0);
}

/*
 * SIGALRM interrupt handler, should be ran each scr_interval seconds.
 */
alarmOff()
{
	alarm(0);
}

alarmOn()
{
	alarm(alarm_secs);
}
 
/*
 * onalarm - write out the mon output file.
 *
 * TBD: possible race with code modifying 
 * report counters. fix by blocking signal in counter code.
 */

void
onalarm()
{
	struct pcap_stat ps;
	struct pcap_stat ps2;
	static int init = 1;
	unsigned long caught = 0;
	unsigned long dropped = 0;
	unsigned long caught2 = 0;
	unsigned long dropped2 = 0;
	int fd1, fd2;
	unsigned int oflow=0;

	if (debug) {
		printf("onalarm called\n");
	}
	pcap_stats(pd, &ps);
	caught = ps.ps_recv;
	dropped = ps.ps_drop;

#ifdef LINUX
	/* do nothing 
	 * ps_recv and ps_drop are reset apparently by themselves.
	 * ps_drop however drops packets too easily ... and this
	 * needs investigation.  will sysctl on net.core params do
	 * any good?
	 */
#else
	/* bsd, solaris */
	fd1 = pcap_fileno(pd);
	ioctl(fd1, BIOCFLUSH);
#endif
	if (debug) {
		printf("PKTS: caught %u, drops %u \n", caught, dropped);
	}

	if (pd2) {
		pcap_stats(pd2, &ps2);
		caught2 = ps2.ps_recv;
		dropped2 = ps2.ps_drop;
#ifndef LINUX
		fd2 = pcap_fileno(pd2);
		ioctl(fd2, BIOCFLUSH);
#else
#endif
	}

	write_report(mon_name, caught, dropped, caught2, dropped2);

	/* TBD: note. we are not zeroing ps_recv, ps_drop, mainly
	 * because we can't! (at least at this level)
	 */
	zero_all_filters();

	/* note: this zeroes out any kernel buffer too in bpf-land,
	 * as well as resets caught/dropped stats.  It would
	 * be nice (maybe?) to just zero out caught/dropped.
	 */
 
	(void) signal(SIGALRM, onalarm);
	alarm(alarm_secs);

}

/* do whatever after a signal
*/
void
cleanup(int arg)
{
	syslog(LOG_ERR, "ourmon exit - cleanup(%d) called errno %m", arg);
	exit(0);
}

void
ignorehup(int arg)
{
	(void) signal(SIGHUP, ignorehup);
}

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-a secs -c num -i name -f file -m file -d -s length] [filter_expr]\n\
		-a number\t# of secs per output file write\n\
		-c number\tcount number of packets and exit\n\
		-t number\tread timeout in ms (default 1000)\n\
		-i name\t\tnetwork interface name; ef0, sl0, ppp0, lo0, etc\n\
		-I name\t\t2nd network interface name; ef0, sl0, ppp0, lo0, etc\n\
		-f file\t\tconfig file name - default /etc/ourmon.conf\n\
		-d \t\tdebug flag \n\
		-r file\t\tread from tcpdump file\n\
		-s \t\tsnap length (total) of pkt \n\
		-m file\t\tmon output file name, ./mon.lite\n\
		expr\t\ttcpdump filter expression\n",
		program_name);

	exit(1);
}


/* 
 * support two interfaces.  basic idea is that
 * we use some "select" (or select-like) polling mechanism,
 * and then acc. to the i/o channel that has input,
 * we go out there and use the secret pcap_read with cnt == 0
 * value, which basically does the right thing.  It reads
 * as much i/o as possible, and processes it into packets,
 * driving the backend "lookup_if" routines that do the filtering.
 * logically we can take two ethernet interfaces then, and
 * merge them into one backend filtering stream.  
 *
 * TBDs:
 *	couldn't get select to work (would be blocking ...).
 *
 * btws:
 *	this routine never returns.
 */

#define SELECT	1

void
ourmon_twodevices(int op, int op2)
{
	int fd1;
	int fd2;
	int rc;
	int notdone = 1;
	extern pcap_handler lookup_if();
	int v = 1;

#ifdef SELECT
        struct timeval timeout = {0, 0};
        fd_set ready;
#endif

	/* extract bpf fds
	*/
	fd1 = pcap_fileno(pd);
	fd2 = pcap_fileno(pd2);

	/* instruct bpf to not dilly-dally about
	 * waiting for the buffer to fill before read
	 * returns.  We can't do that with two interfaces,
	 * else we risk losing packets.
	 */
#ifndef LINUX
	ioctl(fd1, BIOCIMMEDIATE, &v);
	ioctl(fd2, BIOCIMMEDIATE, &v);
#else
#endif

	while(notdone) {
		int on = 0;	/* on is byte count to read */
		int on2 = 0;

#ifdef USEFIONREAD
		/* this means we have a spin loop, as opposed
		 * to using select.  However this code WORKS!
		 */
		ioctl(fd1, FIONREAD, &on);
		if (on) {
			rc=pcap_read(pd, 0, lookup_if(op), NULL);
		}
		ioctl(fd2, FIONREAD, &on2);
		if (on2) {
			rc=pcap_read(pd2, 0, lookup_if(op2), NULL);
		}

#else 

			FD_ZERO(&ready);
        		FD_SET(fd1, &ready);
        		FD_SET(fd2, &ready);
			rc = select(100, &ready, NULL, NULL, NULL);
			/* 
			rc = select(getdtablesize(), &ready, NULL, NULL, &timeout);
			*/
			/* alarm will cause this to spin
			*/
			if (rc < 0) {
				/*
				perror("select");
				notdone = 0;
				*/
				continue;
			}
			if (FD_ISSET (fd1, &ready)) {
#define COUNT 1
				rc=pcap_read(pd, COUNT, lookup_if(op), NULL);
			}
			if (FD_ISSET (fd2, &ready)) {
				rc=pcap_read(pd2, COUNT, lookup_if(op2), NULL);
			}
#endif
		}
		pcap_close(pd);
		pcap_close(pd2);
}

#ifndef LINUX
/*
 * get size of bpf buffer
 */
int
get_bpfbufsize()
{
	int fd = pcap_fileno(pd);
	int v = 0;

        ioctl(fd, BIOCGBLEN, (caddr_t)&v);
	g_bufsize = v;
	return(v);
} 
#endif

