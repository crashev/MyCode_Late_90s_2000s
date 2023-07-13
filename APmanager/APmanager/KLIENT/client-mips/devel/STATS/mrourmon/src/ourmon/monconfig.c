/*
 * config.c - config info for ourmon utility
 * 
 * purpose: read/parse/store tag values from /etc/ourmon.conf configuration
 * file.
 * 
 * file syntax: ascii lines, one tag per line, may have N values depending on
 * tag skip lines/and comments, # in 1st column is comment
 *
 * All filters are configured with one line, the name of the filter
 * comes first.  optional parameters may follow on a per filter basis.
 * Filter spec is provided below.
 * 
 * tag syntax:
 * 
 * 0. pkts  
 *
 *	There is an implicit filter output that counts 	packets caught 
 *	and dropped.  These variables are zeroed
 *	at this point by the less than wonderful idea of
 *	zeroing out the BPF at write output file time.  
 *      The information comes from pcap (pcap_stats()).
 *	It allows us to determine if the filter/s are causing
 *	packet drops.  Note that two interfaces can be supported.
 *
 * 0.1 sysname.
 *
 *      Provide an optional label for the system.  If this
 * 	is provided.  E.g., "bob", then the pkts RRD will be named
 *	pkts.bob, and omupdate.pl will produce a unique per-probe
 *	pkts filter.   The resulting packets graph will have a label 
 *	like pkts.bob, and be per-probe.
 *	This allows a number of systems to run different
 *	filters and thus allow parallelism in the front-end.
 *	ourmon (the front-end) will place this sysname first
 *	(if provided) in the output mon.lite file.
 *
 * 1. fixed_ip
 * 
 *	syntax: fixed_ip
 *
 *      Fixed filter; i.e., keys are not dynamic/changeable.
 *	tcp/udp/icmp/remainder (4 keys).
 *	This filter is zeroed out at every write to output.
 *
 * 2. fixed_tcp3
 *
 *	syntax:  fixed_tcp3 tcp_port1  tcp_port2
 *
 *	Fixed filter.  captures and counts packets with tcp dest or
 *	src port equal to port1 or port2.  counts packets (extra)
 *	that do not belong to that filter spec of course.
 *	Note tcp_port1 for example should be a well-known port
 *	and will cause the count of packets with src OR dest port
 *	set to that value. This filter is zeroed out at every write 
 *	to output.
 *
 * 3. fixed_iprange1
 *
 *	syntax: iprange1  ipaddr1 ipnetmask1  ipaddr2 ipnetmask2 s1 s2
 *
 *	s1 and s2 are strings that must be supplied as they are used
 *	in the output. s1 should describe the 1st network, and s2
 *	should describe the 2nd network.
 *
 *	Measures total ip exchange in bytes between first ip network
 *	and second ip network.  Remainder to extra counter; i.e.,
 *	two numbers will be printed out.  More ipranges may be added
 *	later.  This is the semantic equal to tcpdump:
 *		net X and net Y
 *
 *  4. defunct ... 
 *
 *  5. fixed_cast 
 *	
 *	syntax:   fixed_cast  ip_addr  netmask
 *
 *	The focus of this filter is on dividing up packets
 *	into broadcast/multicast/unicast groups.  If the
 *	interface/s used are ethernet interfaces, then the
 *	filter bases its decisions on ethernet dst addresses.
 *	If the interface/s used are NOT ethernet interfaces,
 *	then IP addresses are used.  An IP address (which is used
 *	for guessing at the broadcast IP address) must be supplied.
 *	There are 4 buckets, multicast, unicast, broadcast
 *	and xtra.  Note that if we are running on ethernet,
 *	then decisions are 100% based on ethernet level information,
 *	and the results are not IP specific.  If non-ethernet interfaces
 *	are used,  the xtra bucket will capture all other traffic, which could
 *	include ARP, non-IP, and some directed broadcast, missed because
 *	the IP/netmask info above may miss some kinds of directed broadcast
 *	in a e.g., VLSM-based network.
 *
 * 6. fixed_size:  
 *
 *	syntax: fixed_size
 *
 *	4 buckets for measuring packet sizes (at layer 2):
 *	essentially tiny, small, medium, mtu-ish (1500)
 *
 *	tiny: <= 100
 *	small: < 500
 *	medium: < 1000
 *	large: <= 1500
 *
 *	Currently cannot reset bucket sizes.
 *
 * ------- above are static, remainder are dynamic -----------
 *
 *  7. topn_ip: 
 *	syntax: topn_ip N
 *
 *	N is the desired # of top N ip destinations to be output.
 *	N must be between 10..100 as a multiple of 10 (10, 20 ...100) 
 *      in order to produce 1 to 10 pics with 10 flows per pic.
 *	Information is sorted on a flow basis where a flow is roughly
 *	(ip source, ip dst, next protocol, L4 src port, L4 dst port).
 *	For ICMP L4 src port means the ICMP major code, and L4 dst
 *	port means the ICMP minor code.
 *
 *	The topn_ip list produces 4 lists:
 *	topn_ip
 *	topn_tcp
 *	topn_udp
 *	topn_icmp
 *
 *	The first item following the topn_ip, topn_tcp, etc., tag
 *	in the mon.lite file is the total count of flows during the
 *	sample period.  
 *
 *	topn_ip shows flows based on any ip protocol (tcp, udp, GRE, whatever).  
 *	TCP, UDP, and ICMP are specific to their ip layer protocol.  
 *	None of the graphs show "xtra" bytes.
 *
 *	In addition topn produces two additional mon.lite outputs.
 *
 *	hs_stat -  gives runtime stat information on the hashing algorithm
 *	used.  In general most of this information is only useful
 *	to an implementor.  However the hash list insert count is currently
 *	graphed by the back-end.  This is because inserts are caused by
 *	"new" flows, and inserts highly correlated with denial of service
 *	or scanning attacks as they generate great numbers of random 
 *	ip src, and ip dst addresses (or random ports).  Thus the
 *	back-end is currently graphing inserts.
 *
 *	The back-end also graphs the total number of flows seen by the topn
 *	mechanism.  This is also very useful for watching for random
 *	ip src/dst or port addresses, and can be useful in detecting
 *	scanning attacks.
 *
 *  8.  topn_syn:
 *      syntax: topn_syn N, N < 100. 
 *
 *	N should be a multiple of 10, 10..100.  N is the top N TCP
 *	syn senders.  A list of ip addresses is kept.  For each ip
 *	address we count the number of syn packets where the ip address
 *	is the source.  We sort on this count.  In addition, we count
 *	the number of fins seen where the ip address is considered as the
 *	source address.  Resets are also counted, for resets the ip destination
 *	is the target ip.  Thus we are considering the number of syns and fins
 *	a given IP sends, and also the number of resets a given IP receives. 
 *	The output thus shows syn, fin, and reset counts.  We sort on syns,
 *	and simply present fins/resets as information.
 *
 *	An additional two syn scanning oriented functions are available 
 *	if turned on.  
 *
 *	syntax: 
 *	topn_syn_wormfile  basedirectory
 *
 *	If the wormfile is specified, the topn_syn print function will also
 *	produce a companion file called "tcpworm.txt".  
 *	This file will be written in the basedirectory
 *	(e.g., /home/mrourmon/web.pages).  The file is a list
 *	of ip source addresses that have satisfied the following
 *	constraint.  syns_sent - fins > 40.  
 *	The above is of course a rude heuristic but seems to be useful
 *	for capturing distributed denial of service attacks.
 *
 *	topn_syn_homeip	  ipaddress	netmask
 *
 *	If homeip is provided, the wormfile output is also
 *	processed in RRDTOOL style to determine if the addresses belong to "us"
 *	(the home ip address range) or "them" (addresses do not
 *	belong in the home range).  An additional output is placed
 *	in the mon.lite file as follows:
 *
 *	tworm: totalcount: us_count : them_count :
 *
 *	Thus the ip addresses in tcpworm.txt are summarized in terms
 *	of a total count of IPs which is divided up into two subordinate counts
 *	of "us" versus "them".  One can thus determine if syn scanners
 *	are local, or remote, and how many total, or local scanning ips are active
 *	over time.  This data is captured by omupdate.pl, and can thus
 *	be basedlined via RRDTOOL.
 *
 *	tworm_weight  weight
 *
 *	tworm_weight is a value that ranges from 0..100.  It is used
 *	to pre-filter the counts given in the tworm summary based
 *	on the built-in worm weight that is roughly # of tcp control
 *	packets divided by the total # of tcp packets per ip source.
 *	Thus the tworm output can be filtered say at 80%, or only
 *	ip sources with worm weights greater than or equal to 80 will be counted.
 *	This pre-filter on the worm weight at this time ONLY affects
 *	the tworm mon.lite count and does not affect the contents of tcpworm.txt.
 *	(This may change later).  Thus the filter weight defaults to 0.
 *
 *	topn_syn_sort  0
 *
 *	This is a boolean variable.  If not given, the topn syn list is sorted
 *	by the total number of syns.  If supplied with value set to 0, the topn syn list is pre-sorted
 *	so that all 100% work weight ip sources are shoved to the top of the sort,
 *	and any remaining tuples with a smaller work weight are then sorted by
 *	syns.  E.g., if you had two "synning" IP sources in the syn list,
 *	one with 100%, and two with 99% but more syns.  The 100% "synning" ip source
 *	would come first.  The two 99% rated ip sources would follow sorted by
 *	their respective syn counts.
 *	
 *	topn_syn_wormdiff 20
 *
 *	This number is used as the magic number to determine that subset of IP srcs from
 *	the TCP syn tuple that are determined to belong in the tcpworm.txt output file.
 *	A magic equation is used for each IP source address.  Per IP src: SYNS - FINS > wormdiff.
 *	If this is true, the ip source in question is placed in the output tcpworm.txt file.
 *	That set of IP sources is deemed to be "noisy" for some reason, and often contains
 *	syn scanners and other forms of attacks, although it also contains noisy web servers
 *	and P2P clients.  Of course the latter may not be desired at a given site.
 *	Note: if this value is set to 0, one is effectively dumping the entire synlist.
 *
 *  9. topn_icmperror:
 *      syntax: topn_icmperror N, N < 100. 
 *
 *	This filter generates two different output lists.  One
 *	list called icmp_errorlist simply gives the top generators
 *	of ICMP errors based on IP src.  The second list is UDP
 *	focused, and uses a weighted function to rank systems
 *	according to UDP packets send/received times significant ICMP
 *	errors.  It is called the udperror_list.
 *
 *	N should be a multiple of 10, 10..100.  N is the top N IP
 *	source addresses that accrue ICMP errors back to them.
 *
 *	For the icmp_errorlist:
 *	A list of IP source addresses is kept.  For each IP
 *	address we count the number of icmp errors that include
 *	icmp redirects, unreachables (of any important kind), and ttl exceeded, 
 * 	where the ip dst of the icmp error messages is the same as that 
 *	of a known IP src ip address.  We sort on the total icmp error count.  
 *	The mon.lite output for the icmp_errorlist (with one tuple) is
 *	as follows:
 *
 *	icmp_errorlist: ip address: totalicmp_error_count: tcp_count:
 *		udp_count: icmp_ping_count: unreach_count: redirect_count:
 *		ttl_count: next tuple ...
 *
 *	This means for a given ip src address:
 *		total_icmp_error_count: all icmp errors returned to this addr
 *		tcp_count: total count of tcp packets sent from this addr
 *		udp_count: total count of udp packets sent from this addr
 *		icmp_ping_count:  pings sent (echo request only)
 *		unreach_count: ICMP unreachables returned to this address
 *		redirect_count: ICMP redirects returned to this address
 *		ttl_cou0nt: ICMP ttl exceeded returned to this address
 *
 *	The second list is a weighted UDP error list.  Its goal is to
 *	find UDP scanners based on ICMP errors returned to a given
 *	scanning IP source address.  The mon.lite output is as follows:
 *
 *	udperror_list: ip address: weight: udp_packets_sent: 
 * 		udp_packets_received:  total_icmp_errors: next tuple ...
 *
 *	This list is sorted according to a weight function. 
 *
 *	W = (udp_pkts_sent - udp_pkts_received) * total_icmp_errors.
 *	
 *	As before the ICMP errors are redirects, unreachables, and ttl
 *	exceeded.  It is likely that a UDP scanner is generating host
 *	unreachables, and possibly ICMP administrative prohibited 
 *	unreachables.  UDP pkts sent and recieved are both shown.
 *	Note: it is a bad sign if a system is sending UDP packets
 *	and not getting them back AND generating many ICMP errors. 
 *	It is possible that such a system is a media-based system
 *	that is simply misbehaving.  It is also possible that it is
 *	the slammer worm.
 *
 *  10. topn_port:
 *	syntax: topn_port N, N < 100.
 *
 *	N should be a multiple of 10, 10..100.  N is the top N ports
 *	src/dst for UDP or TCP flows.  Two lists are output sorted by
 *	bytes, one for TCP flows and one for UDP flows.  The number of
 *	flows seen during the period is the first number.  Subsequent
 *	numbers are 4-tuples of the form (byte, port, src_port_packet_count,
 *	dst_port_packet_count).  If a packet is sent to UDP dst port 512 or
 *	recv from UDP port 512, it counts against the port tuple 512.  
 *	The src/dst packet counters simply reflect the number of times a given
 *	port was seen.  
 *
 *  11. ip port scanning filters (2): 
 *      syntax: topn_scans N
 *              topn_port_scans N
 *
 *	N may be set in units of 10 from 10 to 100.
 *	The back-end produces a separate graph for each successive
 *	unit of 10. N specifies the top N ip address scanners.
 *
 *	topn_scans is a filter aimed at detection of IP destination 
 *	address scanning by a single IP source.  It produces tuples
 *	of the form: ip address: count, where count is the count
 *	of distinct ip destinations for that ip address during
 *	the sample period.  We sort on the count.  Thus this detects
 *	single ip src to many ip destination scans.
 *
 * 	topn_port_scans is aimed at scanning from one ip source
 *	of multiple TCP or UDP ports. 	topn_port_scans three
 *	outputs (three lists):
 *
 *	ip_portscan:
 *	tcp_portscan:
 *	udp_portscan:
 *
 *	Each has a 2-tuple consisting of an ip src address and a count.
 *	The count indicates the number of distinct L4 ports referenced
 *	by the ip src during the sample period.  tcp_portscan only
 *	counts tcp port scanners.  udp_portscan only counts udp port
 *	scanners.  ip_portscan counts both tcp and udp port scanners.
 *
 *	The underlying goal here is to detect manual scanning say
 *	done with nmap, or dynamic scanning done by viruses.  P2P
 *	apps may also be detected.
 *	
 *  12. bpf: 
 *
 *	Two possible forms:  a basic bpf spec, which has a fixed number.
 *	and a concatenated bpf which means a basic bpf specification,
 *	with > 1 bpf filters in the same RRD graph.  
 *
 *	The basic spec can be viewed as consisting of two graphs
 *		bpf bytes and extra (xtra); i.e., bytes that
 *		did not match the bpf graph.
 *	Each bpf specification must have a unique label which is
 *	used to label the graph.  
 *
 *	Each bpf string (the filter spec) must also have its own label.
 *
 *	Up to N (MAXBPFS) (a small number) of concatenated bpfs can
 *	be tacked onto a basic bpf, thus producing one rrd graph
 *	with up to MAXBPFS number of lines (plus the extra line) on
 *	the graph.
 *
 *	syntax:
 *
 *	The basic specification has this form:
 *
 *	bpf "label" "per-filter-label" "filter"
 *
 *	label - label for entire bpf specification
 *	per-filter-label - short name for the filter 
 *	filter - the bpf filter spec
 *
 *	The concatenated spec syntax:
 *
 *	bpf-next "per-filter-label" "filter"
 *
 * 	This form must follow the bpf form sequentially in
 *	the configuration file.
 *
 *	bpf-noxtra  turns off xtra tag (for now, simply
 *	doesn't store it.  still in perl backend)
 *
 * bpf-packets
 * bpf-bytes
 *
 *	The bpf graphs default to counting bytes (bpf-bytes).
 *	bpf-packets thus means that any bpf graphs that start
 *	after bpf-packets should count packets.  bpf-bytes 
 *	overrides bpf-packets mode.  It is not possible to
 *	change from packets to bytes *within* a bpf graph,
 *	but one can do it between graphs; e.g.,,
 *	
 *	bpf-packets
 *	bpf blah blah
 *	bpf-next
 *	bpf-bytes
 *	bpf blah blah
 *	bpf-noextra
 *
 * trigger_topn_ip value count dump_dirname 
 *
 * If set, and if the topn_ip flow mechanism is enabled first,
 * then it is possible to set a trigger on the count of flows
 * within topn_ip.  A trigger will dump <count> packets worth
 * of packets when the threshold value is reached to a filename
 * stored within the supplied directory name.  The name of the file
 * is determined dynamically when the trigger is turned on.  The name of the
 * file has format topn_ip.<timestamp>.dmp.  This file may be
 * read with tcpdump or any tcpdump compatible sniffer.  The threshold
 * mechanism is a simple integer threshold.  Packet dumping is turned
 * on in this case if the general flow count for all flows is greater
 * then the value V.  Packet dumping is turned off when either the
 * count C is exceeded or the trigger threshold flow count is less than
 * value -- which ever comes first.  Packet dumping after being triggered
 * cannot recur between trigger on and trigger off times which are based
 * on the threshold being crossed (either above it or below it).  For
 * example assume the packet capture count is 100000, and the value is 2000.  If at
 * mon.lite creation time, the value is 2000, a trigger_on event is
 * started and 100000 packets (barring trigger_off occuring because threshold < value)
 * will be stored.  If the count is exhausted and the threshold is
 * still > than v, no more packets will be stored in the file.
 * Thus it is not possible for one trigger_on event to cause more than
 * one output file.  Only one trigger on event is possible for the topn_ip
 * type.  However this does not prevent other triggers from working (say with tworm).
 * Also when this trigger is turned off, it may be retriggered later in which
 * case data will be put into a separate file according to the timestamp.
 *
 * Trigger_on and trigger_off events are put into mon.lite as elog
 * messages and are recorded in the back-end daily event log.  As a result
 * it is possible to more or less determine when the trigger event
 * occurred.  The trigger dump filename is passed to the event log.
 *
 * This particular trigger uses a dynamic BPF expression to determine
 * if packets should be stored.  At trigger_on time the trigger code
 * looks to see if tcp, udp, or icmp events are greater than the trigger,
 * if so, the BPF expression is made more narrow, e.g., "tcp".  If
 * this is NOT the case, the expression "ip" which is the default
 * for this expression is used.  The BPF expression is only made more
 * narrow if for example the TCP flow count per second is greater than
 * the threshold but the UDP and ICMP flow counts are both less than
 * the threshold.  This logic also holds for UDP and ICMP flow counts.
 * When in doubt the trigger expression is "ip".
 *
 * Note: this trigger is aimed at the total flow count, not any
 * individual single flow instance.  The flow count is the second
 * parameter in topn_ip, topn_tcp, etc. mon.lite outputs.
 *
 * Note: at this time ourmon has no way to determine if the 
 * dynamically created file has been created successfully
 * or if there is runtime storage room within the file system
 * in question.  If the file cannot be created an elog message
 * will be sent.  ourmon will not exit.
 *
 * Note: when you attempt to determine how many packets to store
 * in the count field, it is recommended that you base that number
 * on the average packets per second seen in your pkts/drops graph.
 * Also remember that you may have a "needle in a haystack" problem.
 * E.g., if you are storing all flows, there may be so much "noise"
 * that finding the data that caused a flow spike may not be easy.
 * In that case you may want a bigger number in order to capture more
 * "attack" data.
 *
 * trigger_worm value count dump_dirname
 * 
 * This trigger applies to the tworm total count threshold.  If the
 * total value for tworm counts (us+them) exceeds the supplied value,
 * a packet dump will begin for count packets in the dump_dirname in
 * a dynamically created filename.  The filename has the form:
 *
 * 	tworm.<timestamp>.dmp.
 *
 * The semantics for this trigger are similar to the semantics for
 * the preceeding trigger.  However only TCP SYN packets are stored.  
 * Note it can be useful to use the saved back-end raw tcpworm.txt here in
 * conjunction with the back-end tcpworm.pl program to try and
 * reproduce the port signature report at the trigger time.  This can
 * easily help narrow down the attacking instances in terms of 
 * IP source addresses or destination TCP port numbers.  Put another way,
 * it is useful to know the IP address of the attacker a priori or know
 * a port signature in the case of multiple attackers.
 */

/*
 * Copyright (c) 2001,2002,2003,2004 Portland State University 
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <pcap.h>

#include "./filter.h"
#include "stats.h"
#include "trigger.h"

#define MAXLINE	1536

/* global filter variables *************************** */
struct fixed_ipproto fixed_ipproto;
struct fixed_tcp3 fixed_tcp3;
struct fixed_iprange1 fixed_iprange1;
struct spec_ipproto spec_ipproto;
struct fixed_cast fixed_cast;
struct fixed_size fixed_size;
struct bpf_spec bpfs[MAXBPFSPECS];
char sysname[MAXSYSNAME];
struct ourstats os;

struct topn_ip topn_ip;
struct topn_syn topn_syn;
struct topn_icmperror topn_icmperror;
struct topn_port topn_port;

struct topn_sip topn_scans_input;
struct topn_sip topn_scans_output;
struct hashStats topn_scans_ip_stats;
 
struct topn_sip topn_scans_ports_input;
struct topn_sip topn_scans_ports_output;
struct hashStats topn_scans_ports_stats;

struct topn_sip topn_scans_ports_udp_input;
struct topn_sip topn_scans_ports_udp_output;
struct hashStats topn_scans_ports_udp_stats;

struct topn_sip topn_scans_ports_tcp_input;
struct topn_sip topn_scans_ports_tcp_output;
struct hashStats topn_scans_ports_tcp_stats;

extern struct trigger_cap trigger_cap[MAXTRIGGERS];

struct topn_irc topn_irc;

int bpfpackets = 0;	/* bpf graph is in bytes mode if 0, packets if 1 */

/*************************************************/

extern int debug;

/*
 * function prototype declarations
 */
static          switchOnOp();
int             resolveAddress(char *addr, struct sockaddr_in * to);
char*		extractString();

void
configInit()
{
	int i;

	fixed_ipproto.configured = FALSE;
	fixed_tcp3.configured = FALSE;
	fixed_iprange1.configured = FALSE;
	fixed_cast.configured = FALSE;
	fixed_size.configured = FALSE;
	topn_ip.configured = FALSE;
	topn_port.configured = FALSE;
	topn_syn.configured = FALSE;
	topn_syn.wormfile[0] = '\0';
	topn_syn.wflag = FALSE;
	topn_syn.weight = 0;
	topn_syn.ipaflag = FALSE;
	topn_syn.sortbysyn = TRUE;
	topn_syn.ipaddr.s_addr = 0;
	topn_syn.netmask.s_addr = 0;
	topn_syn.wormdiff = 20;
	topn_icmperror.configured = FALSE;
        topn_scans_input.configured = FALSE;
        topn_scans_output.configured = FALSE;
        topn_scans_ports_input.configured = FALSE;
        topn_scans_ports_output.configured = FALSE;
        topn_scans_ports_tcp_input.configured = FALSE;
        topn_scans_ports_tcp_output.configured = FALSE;
        topn_scans_ports_udp_input.configured = FALSE;
        topn_scans_ports_udp_output.configured = FALSE;
	topn_irc.configured = FALSE;

	bzero(sysname, MAXSYSNAME);

	bzero(&trigger_cap, sizeof(struct trigger_cap[MAXTRIGGERS]));

	for (i = 0; i < MAXBPFS; i++) {
		bpfs[i].configured = FALSE; 	
	}
}

/*
 * read the config file and load its values
 * 
 * return: 0: ok -1: fatal error (although we return)
 */
int
configRead(configName, bpfop, snaplen, netmask)
char *configName;	/* name of config file */
int  bpfop;		/* bpf op */
int  snaplen;		/* bpf snaplen */
unsigned long netmask;  /* bpf netmask */
{
	FILE           *fd;
	char            linebuf[1024];
	char            opbuf[1024];
	char           *opword;
	char           *result;
	int             lineNumber = 1;
	int 		i, j;
	char 		*ptr;
	int 		ccount;

	fd = fopen(configName, "r");
	if (fd == NULL) {
		fprintf(stderr, "ourmon: open of config file %s failed\n",
		       configName);
		exit(1);
	}

	configInit();

	/*
	 * while not at EOF read a line, parse it
	 */
	while (feof(fd) == 0) {
		result = fgets(linebuf, MAXLINE, fd);
		if (result == NULL)
			break;

		ccount = strlen(linebuf);
		linebuf[ccount-1] = 0;

		/*
		 * skip comment
		 */
		if (linebuf[0] == '#') {
			lineNumber++;
			continue;
		}
		/*
		 * skip empty lines
		 */
		if (linebuf[0] == '\n' || linebuf[0] == '\r') {
			lineNumber++;
			continue;
		}
		/*
		 * skip whitespace in line
		 */
		while (*result == ' ' || *result == '\t') {
			result++;
		}
		/*
		 * find first token
		 */
		strncpy(opbuf, result, 1024);
		opword = strtok(opbuf, " \t");
		if (opword == NULL || (strlen(opword) == 0))
			continue;
		/*
		 * sscanf(result, "%s %s", op, value); printf("op value %s
		 * %s\n", op, value);
		 */
		switchOnOp(bpfop, snaplen, netmask, opword, result, lineNumber);
		lineNumber++;
	};
	fclose(fd);

	if (debug) {
		configDump();
	}

	/*
	 * semantic checks if any
	 */
	return (0);

}  /* end configRead() */

static
switchOnOp(bpfop, snaplen, netmask, op, linebuf, line)
	int	       bpfop;
	int 	       snaplen;
	unsigned long  netmask;
	char           *op;
	char           *linebuf;
	int             line;
{
	char            dc[MAXLINE];
	char            v1[MAXLINE];
	char            v2[MAXLINE];
	char            v3[MAXLINE];
	char            v4[MAXLINE];
	char            v5[MAXLINE];
	char            v6[MAXLINE];
	int             rc;
	struct sockaddr_in sa;
	char*		p;
	extern char *   device_name;

	/* mn ifname ip */
	if (strcmp(op, "fixed_ipproto") == 0) {
		fixed_ipproto.configured = TRUE;
	} else if (strcmp(op, "sysname") == 0) {
		sscanf(linebuf, "%s %s", dc, v1);
		strncpy(sysname, v1, MAXSYSNAME-1);
	} else if (strcmp(op, "fixed_tcp3") == 0) {
		fixed_tcp3.configured = TRUE;
		sscanf(linebuf, "%s %s %s", dc, v1, v2);
		fixed_tcp3.tcp_port1 = atoi(v1);
		fixed_tcp3.tcp_port2 = atoi(v2);
		if (fixed_tcp3.tcp_port1 == 0 || fixed_tcp3.tcp_port2 == 0) {
			fprintf(stderr, "fixed_tcp3 needs two non-zero port arguments\n");
			fprintf(stderr, "syntax: fixed_tcp3 port1 port2\n");
			exit(1);
		}
	} else if (strcmp(op, "fixed_iprange1") == 0) {
		fixed_iprange1.configured = TRUE;

		sscanf(linebuf, "%s %s %s %s %s %s %s", dc, v1, v2, v3, v4, v5, v6);
		/* labels first
		*/
		strncpy(fixed_iprange1.s_ipnet1, v5, 1024);
		strncpy(fixed_iprange1.s_ipnet2, v6, 1024);

		/* now 4 "ip" addresses, two of which should actually
		 * be subnet masks
		 */
		rc = resolveAddress(v1, &sa);

		if (rc <= 0) {
			fprintf(stderr, "fixed_iprange1: invalid ip address 1 %s\n", v1);
			exit(1);
		}
		/* in network byte order
		*/
		fixed_iprange1.ipaddr1.s_addr = sa.sin_addr.s_addr;

		rc = resolveAddress(v2, &sa);
		if (rc <= 0) {
			fprintf(stderr, "fixed_iprange1: invalid mask 1 %s\n", v2);
			exit(1);
		}
		/* in network byte order
		*/
		fixed_iprange1.netmask1.s_addr = sa.sin_addr.s_addr;

		rc = resolveAddress(v3, &sa);
		if (rc <= 0) {
			fprintf(stderr, "fixed_iprange1: invalid ip address 2 %s\n", v1);
			exit(1);
		}
		/* in network byte order
		*/
		fixed_iprange1.ipaddr2.s_addr = sa.sin_addr.s_addr;

		rc = resolveAddress(v4, &sa);
		if (rc <= 0) {
			fprintf(stderr, "fixed_iprange1: invalid netmask 2 %s\n", v1);
			exit(1);
		}
		fixed_iprange1.netmask2.s_addr = sa.sin_addr.s_addr;

		/* in network byte order
		*/

		fixed_iprange1.ipnet1.s_addr = fixed_iprange1.ipaddr1.s_addr
					& fixed_iprange1.netmask1.s_addr;
		fixed_iprange1.ipnet2.s_addr = fixed_iprange1.ipaddr2.s_addr
					& fixed_iprange1.netmask2.s_addr;

	} else if (strcmp(op, "fixed_cast") == 0) {
		fixed_cast.configured = TRUE;

		sscanf(linebuf, "%s %s %s", dc, v1, v2);

		/* now 2 "ip" addresses, ip addr and net mask
		 */
		rc = resolveAddress(v1, &sa);

		if (rc <= 0) {
			fprintf(stderr, "fixed_cast: invalid ip address %s\n", v1);
			exit(1);
		}
		/* in network byte order
		*/
		fixed_cast.ipaddr1.s_addr = sa.sin_addr.s_addr;

		rc = resolveAddress(v2, &sa);
		if (rc <= 0) {
			fprintf(stderr, "fixed_cast: invalid mask %s\n", v2);
			exit(1);
		}
		/* in network byte order
		*/
		fixed_cast.netmask1.s_addr = sa.sin_addr.s_addr;

		/* now compute all 0's and all 1's bcast
		*/

		fixed_cast.ipnet0.s_addr = fixed_cast.ipaddr1.s_addr
					& fixed_cast.netmask1.s_addr;

		fixed_cast.ipnet1.s_addr = fixed_cast.ipnet0.s_addr
					| ~fixed_cast.netmask1.s_addr;

	} else if (strcmp(op, "fixed_size") == 0) {
		fixed_size.configured = TRUE;
	/* bpf label bpf-label bpf-expr */
	} else if (strcmp(op, "bpf") == 0) {
		p = extractString(linebuf, v1, 1024);
		p = extractString(p, v2, 1024);
		p = extractString(p, v3, 1024);
		bpf_config(FALSE, bpfop, snaplen, netmask, FALSE, v1, v2, v3);
	} else if (strcmp(op, "bpf-packets") == 0) {
		bpfpackets = 1;		/* now count packets */
	} else if (strcmp(op, "bpf-bytes") == 0) {
		bpfpackets = 0;		/* now count bytes */
	/* bpf-next bpf-label bpf-expr */
	} else if (strcmp(op, "bpf-next") == 0) {
		p = extractString(linebuf, v1, 1024);
		p = extractString(p, v2, 1024);
		bpf_config(FALSE, bpfop, snaplen, netmask, TRUE, NULL, v1, v2);
	/* this is a variation on bpf-next internally
	*/
	} else if (strcmp(op, "bpf-noxtra") == 0) {
		bpf_config(TRUE, bpfop, snaplen, netmask, TRUE, NULL, 
			NULL, NULL);
	} else if (strcmp(op, "topn_ip") == 0) {
		topn_ip.configured = TRUE;
		sscanf(linebuf, "%s %s", dc, v1);
		topn_ip.actualn = atoi(v1);
		if (topn_ip.actualn > MAXTOPN) {
			fprintf(stderr, "topn_ip topN limit is %d\n",
				MAXTOPN);
			exit(1);
		}
		/* initialize the list mechanism
		*/
		initList(&topn_ip.iplist);

	/* trigger_topn_ip count dirname for dump
	*/
        } else if (strcmp(op, "trigger_topn_ip") == 0) {
                if(topn_ip.configured == FALSE) {
                        fprintf(stderr, "trigger_topn_ip need top_ip configured\n");
                        exit(1);
                }
                sscanf(linebuf, "%s %s %s %s", dc, v1, v2, v3);
 
                if( trigger_config(TOPNIP, v1, v2, v3) <  0 ) {
                        fprintf(stderr,"trigger_topn_ip configuration failed\n");
                        exit(1);
                }

                /* parse the bpf expression.  The default expression
		 * is "ip".   which doesn't matter but oh well.
		*/
                if ( trigger_bpf_config(TOPNIP, bpfop, snaplen, netmask, NULL) < 0 ) {
                        fprintf(stderr,"trigger_topn_ip bpf parse failed\n");
                        exit(1);
                }

	} else if (strcmp(op, "topn_syn") == 0) {
		topn_syn.configured = TRUE;
		sscanf(linebuf, "%s %s", dc, v1);
		topn_syn.actualn = atoi(v1);
		if (topn_syn.actualn > MAXTOPPORTS) {
			fprintf(stderr, "topn_syn topN limit is %d\n",
				MAXTOPPORTS);
			exit(1);
		}
		/* initialize the list mechanism
		*/
		init_synlist(&topn_syn.synlist);
/*
 *	topn_syn_wormfile  basedirectory + tcpworm.txt
 */
	} else if (strcmp(op, "topn_syn_wormfile") == 0) {
		char *fn = "tcpworm.txt";
		int l = strlen(fn);
		
		if (topn_syn.configured == FALSE) {
			fprintf(stderr, "topn_syn_wormfile: topn_syn needs to turned on\n");
			exit(1);
		}
		sscanf(linebuf, "%s %s", dc, v1);
		
		if (strlen(v1) > (1022-l)) {
			fprintf(stderr, "topn_syn path too long\n");
			exit(1);
		}
		sprintf(topn_syn.wormfile, "%s/%s", v1, fn);
		topn_syn.wflag = TRUE;
/*
 *	tworm_weight 0..100
 */
 	 
	} else if (strcmp(op, "tworm_weight") == 0) {
		
		if (topn_syn.wflag == FALSE) {
			fprintf(stderr, "tworm_weight: topn_syn.wormfile needs to be set\n");
			exit(1);
		}
		sscanf(linebuf, "%s %s", dc, v1);
		
		topn_syn.weight = atoi(v1);
		if ((topn_syn.weight < 0) || (topn_syn.weight > 100)) {
			fprintf(stderr, "tworm_weight value must be between 0 and 100 inclusive\n");
			exit(1);
		}
 
	} else if (strcmp(op, "topn_syn_wormdiff") == 0) {
		
		if (topn_syn.wflag == FALSE) {
			fprintf(stderr, "tworm_weight: topn_syn.wormfile needs to be set\n");
			exit(1);
		}
		sscanf(linebuf, "%s %s", dc, v1);
		
		topn_syn.wormdiff = atoi(v1);
		if ((topn_syn.wormdiff < 0)) {
			fprintf(stderr, "topn_syn.wormdiff must be non-negative.\n");
			exit(1);
		}

	} else if (strcmp(op, "topn_syn_sort") == 0) {
		
		if (topn_syn.configured == FALSE) {
			fprintf(stderr, "topn_syn_sort: topn_syn needs to be set before this switch is set.\n");
			exit(1);
		}
		sscanf(linebuf, "%s %s", dc, v1);
		
		topn_syn.sortbysyn = atoi(v1);
		if ((topn_syn.sortbysyn < 0) || (topn_syn.sortbysyn > 1)) {
			fprintf(stderr, "topn_syn_sort value must be 0 or 1 inclusive.\n");
			exit(1);
		}

/*
 *	topn_syn_homeip	  ipaddress	netmask
 */
	} else if (strcmp(op, "topn_syn_homeip") == 0) {

		if (topn_syn.configured == FALSE) {
			fprintf(stderr, "topn_syn_homeip: topn_syn needs to turned on\n");
			exit(1);
		}
		if (topn_syn.wflag == FALSE) {
			fprintf(stderr, "topn_syn_homeip: topn_syn_wormfile needs to be set first\n");
			exit(1);
		}
		sscanf(linebuf, "%s %s %s", dc, v1, v2);

		/* now 2 "ip" addresses, ip addr and net mask
		 */
		rc = resolveAddress(v1, &sa);

		if (rc <= 0) {
			fprintf(stderr, "topn_syn_homeip: invalid ip address %s\n", v1);
			exit(1);
		}
		/* in network byte order
		*/
		topn_syn.ipaddr.s_addr = sa.sin_addr.s_addr;

		rc = resolveAddress(v2, &sa);
		if (rc <= 0) {
			fprintf(stderr, "topn_syn_homeip: invalid mask %s\n", v2);
			exit(1);
		}
		/* in network byte order
		*/
		topn_syn.netmask.s_addr = sa.sin_addr.s_addr;

		topn_syn.ipaflag = TRUE;

/*
 *      trigger_worm N count tworm_dump_file
 */
        } else if(strcmp(op, "trigger_worm") == 0) {
                if(topn_syn.configured == FALSE){
                        fprintf(stderr,"trigger_worm: topn_syn needs to turned on\n");
                        exit(1);
                }
                if (topn_syn.wflag == FALSE) {
                        fprintf(stderr,"trigger_worm: tcp worm file needs to be set\n");
                        exit(1);
                }
                sscanf(linebuf, "%s %s %s %s", dc, v1, v2, v3);

                if( trigger_config(TWORM, v1, v2, v3) < 0 ) {
                         fprintf(stderr,"trigger_worm: configure failed\n");
                         exit(1);
                }

                if( trigger_bpf_config(TWORM, bpfop, snaplen, netmask, NULL) < 0 ) {
                        fprintf(stderr,"trigger_worm: bpf parsing failed\n");
                        exit(1);
                }
 
	} else if (strcmp(op, "topn_irc") == 0) {
		topn_irc.configured = TRUE;
		sscanf(linebuf, "%s %s", dc, v1);
		topn_irc.cycles = atoi(v1);
		if (topn_irc.cycles > 60) {
			fprintf(stderr, "topn_irc 10 is current max n");
			exit(1);
		}
		/* initialize the list mechanism
		*/
		initIrc(&topn_irc.irclist);

	} else if (strcmp(op, "topn_icmperror") == 0) {
		topn_icmperror.configured = TRUE;
		sscanf(linebuf, "%s %s", dc, v1);
		topn_icmperror.actualn = atoi(v1);
		if (topn_icmperror.actualn > MAXTOPPORTS) {
			fprintf(stderr, "topn_icmperror topN limit is %d\n",
				MAXTOPPORTS);
			exit(1);
		}
		/* initialize the port list mechanism
		*/
		init_icmplist(&topn_icmperror.icmplist);
	} else if (strcmp(op, "topn_port") == 0) {
		topn_port.configured = TRUE;
		sscanf(linebuf, "%s %s", dc, v1);
		topn_port.actualn = atoi(v1);
		if (topn_port.actualn > MAXTOPPORTS) {
			fprintf(stderr, "topn_port topN limit is %d\n",
				MAXTOPPORTS);
			exit(1);
		}
		/* initialize the port list mechanism
		*/
		init_plist(&topn_port.tcplist);
		init_plist(&topn_port.udplist);

	} else if (strcmp(op, "topn_scans") == 0) {
		sscanf(linebuf, "%s %s", dc, v1);
		topn_scans_input.actualn = atoi(v1);
		topn_scans_output.actualn = topn_scans_input.actualn;
		topn_scans_input.configured = 1;
		topn_scans_output.configured = 1;
		if (topn_scans_input.actualn > MAXTOPN) {
			fprintf(stderr, "topn_scans topN limit is %d\n", 
				MAXTOPN);
			exit(1);
               	}
		/* initialize the list mechanism
		*/
		initScanList(&topn_scans_input.iplist);
		initScanList(&topn_scans_output.iplist);

	} else if (strcmp(op, "topn_port_scans") == 0) {
		sscanf(linebuf, "%s %s", dc, v1);
		topn_scans_ports_input.actualn = atoi(v1);
		
		topn_scans_ports_output.actualn = 
			topn_scans_ports_input.actualn;
		topn_scans_ports_input.configured = 1;
		topn_scans_ports_output.configured = 1;

		topn_scans_ports_tcp_output.actualn = 
			topn_scans_ports_tcp_input.actualn =
			topn_scans_ports_input.actualn;
		topn_scans_ports_input.configured = 1;
		topn_scans_ports_output.configured = 1;
		
		topn_scans_ports_udp_output.actualn = 
			topn_scans_ports_udp_input.actualn =
			topn_scans_ports_input.actualn;
		
		topn_scans_ports_input.configured = 1;
		topn_scans_ports_output.configured = 1;

		topn_scans_ports_tcp_input.configured = 1;
		topn_scans_ports_udp_input.configured = 1;
		
		topn_scans_ports_udp_output.configured = 1;
		topn_scans_ports_tcp_output.configured = 1;
		
		if (topn_scans_ports_input.actualn > MAXTOPN) {
			fprintf(stderr, "topn_port_scans topN limit is %d\n", 
				MAXTOPN);
			exit(1);
               	}

		/* initialize the list mechanism
		*/
		initScanList(&topn_scans_ports_input.iplist);
		initScanList(&topn_scans_ports_output.iplist);

		initScanList(&topn_scans_ports_tcp_input.iplist);
		initScanList(&topn_scans_ports_tcp_output.iplist);
	
		initScanList(&topn_scans_ports_udp_input.iplist);
		initScanList(&topn_scans_ports_udp_output.iplist);
	
	} else {
		fprintf(stderr, "switchOnOp(): line [%d], unknown config op (%s) in config file\n", line, op);
		exit(1);
	}
}

/*
 * compile bpf expression into fcode bpf byte array
 *
 * note: errexit: exits at config time, does not exit at runtime
 *
 * return:
 *	< 0: compile error.
 *	otherwise: ok.
 */
int
local_pcap_compile_nopcap(
		int snaplen_arg, 
		int linktype_arg, 
		struct bpf_program *program,
                char *buf, 
		int optimize, 
		unsigned long mask, int errexit)
{
        pcap_t *p;
        int ret;
                        
        p = pcap_open_dead(linktype_arg, snaplen_arg);
        if (p == NULL)
                return (-1);
        ret = pcap_compile(p, program, buf, optimize, mask);
	if (ret < 0 && errexit) {
		fprintf(stderr, "expr: (%s): pcap syntax error: %s\n",
			buf,
			pcap_geterr(p));
		exit(1);
	}
        pcap_close(p);
        return (ret);
}

static int last_bpf_index = 0;
/* global so runtime analysis routines
 * can learn if there is any point in
 * calling bpf_filter
 */
int any_bpf = 0;

/* struct bpf_spec bpfs[MAXBPFSPECS]; */

bpf_config(int xtraflag, int bpfop, int snaplen, unsigned long netmask, int ntag, 
	char *label, char* blabel, char* bpfexpr)
{
	int i;
	struct bpf_spec *b;
	int gotone = 0;		/* gotone is the fcode index */

	if (debug) {
		printf("ntag %d, label %s bl %s bpf %s\n", ntag,
			label, blabel, bpfexpr);
	}
	if (!ntag) {
		/* check for array exhaustion
		*/
		if (last_bpf_index >= MAXBPFSPECS) {
			fprintf(stderr, "out of bpf specs, max %d\n",
				MAXBPFSPECS);
			exit(1);
		}
		/* using the label, search for a free bpf structure 
		*/
		b = &bpfs[0];
		for ( i = 0; i < MAXBPFSPECS; i++) {
			if (b->configured == FALSE) {
				break;
			}
			if (strcmp(label, b->prog_label[0]) == 0) {
				fprintf(stderr, 
					"ourmon: bpf label %s already used\n",
		       			label);
				exit(1);
			}
			b++;
		}
		/* allocate i and store labels
		*/
		last_bpf_index = i;
		any_bpf = TRUE;
		b->configured = TRUE;
		strncpy(b->label, label, MAXLABEL-1);
		strncpy(b->prog_label[0], blabel, MAXBPFLABEL-1);
		b->prog_count = 1;
		b->bpfpackets = bpfpackets;  /* set mode bytes/packet cnt */
	}
	/* deal with concatenation case; that is, we must have
	 * a previous bpf_spec with an available bpf fcode in it
	 */
	else {
		if (any_bpf == FALSE) {
			fprintf(stderr, "%s must be used after a previous bpf tag\n", bpfop);
			exit(1);
		}
		/* we must have a current bpf spec, check for fcode
		 * array exhaustion.
		 */
		b = &bpfs[last_bpf_index];

		/* if xtra flag is set, we do not compute
		 * the xtra (remainder) bytes
		 */
		if (xtraflag) {
			b->noxtra = TRUE;
			return;
		}
		/* it cannot be the 1st one.  we check the fcode
		 * to see if it is set.
		*/
		for ( i = 1; i < MAXBPFS; i++) {
			if (b->fcodes[i].bf_insns == NULL) {
				gotone = i;
				break;
			}
		}
		/* gotone is the fcode index
		*/
		if (gotone == 0) {
			fprintf(stderr, "all bpf slots in bpf spec used up, max is %d\n", MAXBPFS);
			exit(1);
		}
		/* ok we have a free fcode slot
		*/
		strncpy(b->prog_label[gotone], blabel, MAXBPFLABEL-1);
		b->prog_count++;	/* count of fcodes in use */
	}
	/* compile the bpf expression
	 * note: if fatal error, this routine does not
	 * return.
	 */
	(void) local_pcap_compile_nopcap(snaplen, bpfop,
	    &b->fcodes[gotone],  bpfexpr, 0, netmask, 1); 

	if (debug) {
		printf("fcodes[%d] len: %d\n", 
			gotone, b->fcodes[gotone].bf_len);
	}
}

/*
 * debug - dump configuration variables
 */
configDump()
{
	int             i;
	struct bpf_spec *b;
	int j;

	printf("---------config dump-------------------\n");
	if (sysname[0]) {
          	printf("sysname: [%s]\n", sysname);
	}
	if (fixed_ipproto.configured) {
          	printf("fixed_ipproto.configured %d\n", fixed_ipproto.configured);
	}
	if (fixed_tcp3.configured) {
		printf("fixed_tcp3: port1 %d\n", fixed_tcp3.tcp_port1);
		printf("fixed_tcp3: port2 %d\n", fixed_tcp3.tcp_port2);
	}
	if (fixed_iprange1.configured) {
		printf("iprange1: ipaddr1 %s, ", 
			inet_ntoa((struct in_addr)fixed_iprange1.ipaddr1));
		printf("netmask1 %s\n", 
			inet_ntoa((struct in_addr)fixed_iprange1.netmask1) 
			);
		printf("ipaddr2 %s, ", 
			inet_ntoa((struct in_addr)fixed_iprange1.ipaddr2));
		printf("netmask2 %s\n", 
			inet_ntoa((struct in_addr)fixed_iprange1.netmask2) 
			);
		printf("ipnet1 %s, ", 
			inet_ntoa((struct in_addr)fixed_iprange1.ipnet1));
		printf("ipnet2 %s\n", 
			inet_ntoa((struct in_addr)fixed_iprange1.ipnet2));
		printf("labels %s %s\n",
			fixed_iprange1.s_ipnet1,
			fixed_iprange1.s_ipnet2);
	}

	if (topn_ip.configured) {
		printf("topn_ip: actualn %d\n", topn_ip.actualn);
	}

 	if (topn_syn.configured) {

                printf("topn_syn: actualn %d\n", topn_syn.actualn);

		if (topn_syn.wflag) {
			printf("topn_syn: wormfile <%s>\n",
				topn_syn.wormfile);
			printf("topn_syn_wormdiff %d\n", topn_syn.wormdiff);
		}

		if (topn_syn.ipaflag) {
			printf("topn_syn: ipaddr %s \n", 
				inet_ntoa((struct in_addr)topn_syn.ipaddr));
			printf("topn_syn: mask %s \n", 
				inet_ntoa((struct in_addr)topn_syn.netmask));
		}

		if (topn_syn.weight) {
			printf("topn_syn: weight %d\n", topn_syn.weight);
		}

                for ( j = 0; j < MAXTRIGGERS; j++) {
                        if (trigger_cap[j].configured) {
                                printf("trigger_cap value %d\n", trigger_cap[j].N_value);
				printf("trigger_cap count %d\n", trigger_cap[j].count);
				printf("trigger_cap filename %s\n", trigger_cap[j].filename);
                        }
                }

		printf("topn_syn_sort value %d\n", topn_syn.sortbysyn);
        }

 	if (topn_icmperror.configured) {
                printf("topn_icmperror: actualn %d\n", 
			topn_icmperror.actualn);
        }

	if (topn_port.configured) {
		printf("topn_port: actualn %d\n", topn_port.actualn);
	}

        if (topn_scans_input.configured) {   
                printf("topn_scans: actualn %d\n",
                       topn_scans_input.actualn);
        }

        if (topn_scans_ports_input.configured) {
                printf("topn_port_scans: actualn %d\n",
                       topn_scans_ports_input.actualn);
 
                printf("topn_port_scans_udp: actualn %d\n",
                       topn_scans_ports_udp_input.actualn);
 
                printf("topn_port_scans_tcp: actualn %d\n",
                       topn_scans_ports_tcp_input.actualn);
        }

	if (fixed_cast.configured) {
		printf("fixed_cast: bcast 1's %s \n", 
			inet_ntoa((struct in_addr)fixed_cast.ipnet1));
		printf("fixed_cast: bcast 0's %s \n", 
			inet_ntoa((struct in_addr)fixed_cast.ipnet0));
	}

	if (fixed_size.configured) {
		printf("fixed_size: ON \n");
	}

	/* bpf program dump */

	if (any_bpf) {
		b = &bpfs[0];
		for ( i = 0; i < MAXBPFSPECS; i++, b++) {
			if (b->configured == FALSE) {
				break;
			}
			printf("bpf_spec[%d] label %s\n", i, b->label);
			printf("bpf_spec[%d] programs %d\n", i, b->prog_count);
			printf("bpf_spec[%d] NO xtraflag %d\n", i, b->noxtra);
			printf("bpf_spec[%d] bpfpacket flag %d\n", i, b->bpfpackets);
			for ( j = 0; j < MAXBPFS; j++) {
				if (b->fcodes[j].bf_len == 0) {
					break;
				}
				printf("bpf_spec[%d] fcode[%d] len %d\n",
					i, j, b->fcodes[j].bf_len);
				printf("bpf_spec[%d] fcode[%d] label %s\n",
					i, j, b->prog_label[j]);
			}
		}
	}
	printf("---------config dump-------------------\n");
}

#ifdef TESTMAIN
int debug = 1;

char *program_name;
char *device_name = "lo0";	/* just for testing */
pcap_t *pd;

main(argc, argv)
int argc; 
char **argv;
{
	unsigned long netmask = htonl(0xff000000);  /* assume lo0 */

	configRead(argv[1],1,68,netmask);
	/* implicit
	configDump();
	*/

}
#endif

