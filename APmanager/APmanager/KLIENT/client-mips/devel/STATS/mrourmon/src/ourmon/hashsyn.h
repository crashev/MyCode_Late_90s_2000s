 /*
  * tcpsyn.h
  *
  * go forth and syn no more.
  */

/*
 * notes on this data structure
 *	fins: a large amount of syns and a small amount of fins is a NEGATIVE indicator
 *		we either have an attacker or an ineffective protocol.
 *		equal amounts of syns/fins may be regarded as a good sign.
 *		NEGATIVE if small, and/or syns significantly greater.
 *
 *	rst: non-trivial rsts, are of course a bad sign. 
 *		NEGATIVE.
 *
 *	tcpfrom/tcpto are a measure of work.  small tcpto is a bad sign.
 *		work. NEGATIVE if from greater than to.
 *
 *	icmpcount: icmp errors that we see in the real world.  also a negative indicator.
 *		NEGATIVE.
 */
#define WEIGHTCOUNTER	float

#define MAXPORTS			10
struct synnode {
	unsigned long ipsrc; 		/* index */
	unsigned long syncount;		/* sort on this */
	unsigned long fincount;		/* mention this */
	unsigned long rstcount;		/* mention this */
	unsigned long sacount;		/* number of syn+ack pairs */
	unsigned long tcpfrom;		/* tcp packets sent from ipsrc */
	unsigned long tcpto;		/* tcp pkts sent to ip src */
	unsigned long icmpcount;	/* icmp errors sent back to the ip src */ 
	WEIGHTCOUNTER work;		/* work weight + a little fudge*/
	WEIGHTCOUNTER ww;		/* work weight with no fudge at all */
	int pcount;			/* number of ports counted */
	int dports[MAXPORTS];   	/* max dst ports */
	unsigned long dpcounts[MAXPORTS];   /* associated dst port counters */
	struct synnode *next;
};

/*
#define HSIZE 1031 
*/
#define THSIZE 16403
struct synlist{
	unsigned long entries;
	struct synnode *last;
	struct synnode *hashlist[THSIZE];
};

static unsigned long e_hash(unsigned char *, int size);

/**************************************************/
/* function prototypes */

void init_synlist(struct synlist *l);

struct synnode 
*search_srcsyn(struct synlist *l, unsigned long ipsrc, unsigned long ipother, int , 
	int , int, int, int, int);

int insert_srcsyn(struct synlist *l, unsigned long ipsrc, int , int , int, int, int);
void destroy_synlist(struct synlist *l);
void print_synlist(FILE *fd, struct synlist *l, int count, int wflag, 
	char *wormfile, int ipaflag, unsigned long usnet, 
	unsigned long usmask);
