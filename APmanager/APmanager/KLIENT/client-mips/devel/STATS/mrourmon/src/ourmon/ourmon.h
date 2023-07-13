/*
 * ourmon.h
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#ifndef LINUX
#include <netinet/ip_var.h>
#endif
#include <netinet/if_ether.h>

#define	DEFAULT_SNAPLEN	68	/* length of saved portion of packet */

#define	PROTO_SIZE	4	/* protocol size */
#define	BYTES_SIZE	10	/* bytes size */
#define	CPS_SIZE	6	/* cps size */
#define	COUNT_SIZE	9	/* misc counters size */

struct t_entry {
	int	eflag;		/* ethernet flags (m/b) present */
	int	mflag;		/* true if ethernet multicast */
	int	bflag;		/* true if ethernet broadcast */
	int  	synflag;	/* tcp packet, syn flag set */
	int 	finflag;	/* tcp packet, fin flag set */
	int 	ackflag;	/* tcp packet, ack flag set */
	int 	resetflag;	/* tcp packet, reset flag set */
	struct	in_addr	src;	/* source ip address */
	u_short	sport;		/* source port */
	struct	in_addr	dst;	/* destination ip address */
	u_short	dport;		/* destination port */
	u_short	proto;		/* ip protocol */
	u_long	bytes;		/* bytes in ip datagram */
	u_long	obytes;		/* old bytes */
	unsigned char *p;	/* payload pointer (L7 if any) */
	int  psize;		/* size of payload */
};

/* global variables */
extern char *program_name;
extern char *device_name;
extern unsigned char *snapend;
extern int use_colors;
extern int alarm_flag;
extern int scr_interval;
extern int dns_timeout;
extern int n_entry;		/* counter for number of records */
extern int fflag;
extern int kflag;
extern int nflag;
extern int Nflag;
extern int eflag;
extern int page_size;
extern int resize_flag;
extern int addr_size;
extern int proto_size;
extern int bytes_size;
extern int cps_size;
extern int count_size;

extern int debug;

/* function prototypes */

/* util.c */
#ifdef	__STDC__
extern void error(int, char *, ...);
#else
extern void error();
#endif
extern char *copy_argv(char **);
extern char *getprotoname(u_short);
extern u_short getprotonum(char *);

extern void write_report(char *, int, int, int, int);
extern void zero_all_filters(void);

