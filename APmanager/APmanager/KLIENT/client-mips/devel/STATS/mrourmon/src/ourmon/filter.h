/* per filter structure
*/

#ifndef IP_FILTER
#define IP_FILTER

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#ifndef IPCOUNTER
#define IPCOUNTER	1
typedef unsigned long COUNTER;
#endif

/* each filter data definition element has
 * #include, counter structure, configured variable
 */

#define FIXED_IPPROTO 1
struct fixed_ipproto {
	int	 configured;
	COUNTER  tcp_bytes;
	COUNTER  udp_bytes;
	COUNTER  icmp_bytes;
	COUNTER  xtra_bytes;
};
extern struct fixed_ipproto fixed_ipproto;

#define FIXED_TCP3   2
struct fixed_tcp3 {
	int configured;
	unsigned short tcp_port1;
	unsigned short tcp_port2;
	COUNTER  port1_bytes;
	COUNTER  port2_bytes;
	COUNTER  xtra_bytes;
};
extern struct fixed_tcp3 fixed_tcp3;

#define FIXED_IPRANGE1	3
struct fixed_iprange1 {
	int configured;
	struct in_addr ipaddr1;
	struct in_addr ipnet1;
	char s_ipnet1[1024];
	struct in_addr netmask1;
	struct in_addr ipaddr2;
	struct in_addr ipnet2;
	char s_ipnet2[1024];
	struct in_addr netmask2;
	COUNTER	 range1_bytes;
	COUNTER  extra_bytes;
};
extern struct fixed_iprange1 fixed_iprange1;

#define SPEC_IPPROTO  4
struct spec_ipproto {
	int	 configured;
	unsigned short proto1;
	unsigned short proto2;
	unsigned short proto3;
	COUNTER  proto1_bytes;
	COUNTER  proto2_bytes;
	COUNTER  proto3_bytes;
	COUNTER  xtra_bytes;
};
extern struct spec_ipproto spec_ipproto;

#define FIXED_CAST  5
struct fixed_cast {
	int	 configured;
	struct in_addr ipaddr1;  /* input, anded with netmask */
	struct in_addr netmask1;
	struct in_addr ipnet1;   /* computed all 1's bcast */
	struct in_addr ipnet0;   /* computed all 0's bcast */
	COUNTER  mcast_bytes;
	COUNTER  ucast_bytes;
	COUNTER  bcast_bytes;
	COUNTER  xtra_bytes;
};
extern struct fixed_cast fixed_cast;

#define FIXED_SIZE  6
struct fixed_size {
	int	 configured;
	COUNTER  tiny;
	COUNTER  small;
	COUNTER  medium;
	COUNTER  large;
};
extern struct fixed_size fixed_size;

/* bpf group filter spec:
 * logical structure:
 * name: there may be up to MAXBPF bpf invocations;
 *	i.e., this string is both a label for the whole thing and must be unique
 *	per use.
 * string1 - string4, there may be four bpf filters per graph,
 *	not all must be used.  Each must have its own label,
 *	followed by the bpf program in quotes.
 *
 * config form:
 * bpf uber-label label bpf-expression
 * bpf-next label bpf-expression
 *
 * mon.lite form:
 * bpf: name: #bpfs: label: count: label: count ... xtra: count
 */

#define MAXBPFSPECS	100	/* max. number of labels for all bpf graphs */
#define MAXBPFS		6	/* per bpf specs */
#define MAXLABEL 70    /* max chars in label */
#define MAXBPFLABEL 70    /* max chars in bpf label - too big?*/
struct bpf_spec {
	char label[MAXLABEL+1];
	int prog_count;		/* number of bpf programs */ 
	struct bpf_program fcodes[MAXBPFS];	/* max bpfs per bpf spec */
	char prog_label[MAXBPFS][MAXBPFLABEL+1];  /* per program label */
	COUNTER bpf_counts[MAXBPFS];  /* per bpf counter */
	COUNTER xtra_bytes;	      /* no bpf match count */
	int configured;
	int noxtra;	              /* turn bpf xtra off */	
	int bpfpackets;		      /* if on count packets, not bytes */
};
extern struct bpf_spec bpf_spec;

/************************************************** 
 * TOPN
 **************************************************/

#define MAXTOPN		100	/* hard topN maximum. can't have more */

#include "hashsort.h"

#define TOPN_IP	    20
struct topn_ip {
	int configured;
	int actualn;		/* specified value of topN < MAXTOPN */
	struct list iplist;	/* ip dst top N link structure */
};
extern struct topn_ip topn_ip;


#include "hashport.h"

#define MAXTOPPORTS	100	/* topN maximum. can't have more */

struct topn_port {
	int configured;
	int actualn;		/* specified value of topN < MAXTOPPORTS */ 
	struct plist tcplist;	/* tcp port top N link structure */
	struct plist udplist;	/* udp port top N link structure */
};
extern struct topn_port topn_port;

#include "hashsyn.h"

struct topn_syn {
	int configured;
	int sortbysyn;		/* sort by syn, else 100% first */
	int weight;		/* pre-filter for tworm */
	int wormdiff;		/* syns - fins > wormdiff create tcpworm.txt */
	struct in_addr ipaddr;    /* input, anded with netmask */
	struct in_addr netmask;
	char wormfile[1024];    /* basename + tcpworm.txt */
	int wflag;		/* wormfile actually set */
	int ipaflag;		/* ipaddr/netmask actually set */
	int actualn;		/* specified value of topN < MAXTOPN */
	struct synlist synlist;	/* top N link structure */
};
extern struct topn_syn topn_syn;

#include "hashicmp.h"

struct topn_icmperror {
	int configured;
	int actualn;		/* specified value of topN < MAXTOPN */
	struct icmplist icmplist;	/* top N link structure */
};
extern struct topn_icmperror topn_icmperror;

#include "hashscan.h"

struct topn_sip {
	int configured;
	int actualn;		/* specified value of topN < MAXTOPN */
	struct slist iplist;	/* ip dst top N link structure */
};

extern struct topn_sip topn_scans_input;
extern struct topn_sip topn_scans_output;
extern struct hashStats topn_scans_ip_stats;

extern struct topn_sip topn_scans_ports_input;
extern struct topn_sip topn_scans_ports_output;
extern struct hashStats topn_scans_ports_stats;

extern struct topn_sip topn_scans_ports_tcp_input;
extern struct topn_sip topn_scans_ports_tcp_output;
extern struct hashStats topn_scans_ports_tcp_stats;


extern struct topn_sip topn_scans_ports_udp_input;
extern struct topn_sip topn_scans_ports_udp_output;
extern struct hashStats topn_scans_ports_udp_stats;

extern char sysname[];
#define MAXSYSNAME 1025

#include "ircscan.h"

struct topn_irc {
	int configured;
	int cycles;
	struct irclist irclist;
}; 
extern struct topn_irc topn_irc;

#endif
