 /*
  * hashicmp.h
  *
  * ip srcs that generate icmp errors
  */

#define MAXUDPPORTS	10

/* a 5-tuple, ip src, total icmp errors, unreachables, redirects, ttl
*/
struct icmpnode {
	unsigned long ipsrc; 		/* index */
	unsigned long tcpcount;		/* total # of tcp packets */
	unsigned long udpcount;		/* total # of udp packets */
	unsigned long udprcv;		/* total # of udp packets recv */
	unsigned long icmpcount;	/* total # of icmp packets */
	unsigned long totalicmp;	/* sort on this */
	unsigned long unreach;		/* mention this */
	unsigned long redirect;		/* mention this */
	unsigned long ttl;		/* mention this */
	int pcount;			/* number of ports counted */
	int dports[MAXUDPPORTS];   	/* max dst ports */
	int dpcounts[MAXUDPPORTS];   	/* per port hit count */
	struct icmpnode *next;
};

#define ICMPHSIZE 32717 
struct icmplist{
	unsigned long entries;
	struct icmpnode *last;
	struct icmpnode *hashlist[ICMPHSIZE];
};

/**************************************************/
/* function prototypes */

void init_icmplist(struct icmplist *l);

struct icmpnode *search_srcicmp(struct icmplist *l, unsigned long ipsrc, 
	int tcpcount, int udpcount, int icmpcount,int,int,int,int, long udpport);

int insert_srcicmp(struct icmplist *l, unsigned long ipsrc, int tcpcount, int udpcount, int icmpcount, long udpport);
void destroy_icmplist(struct icmplist *l);
void print_icmplist(FILE *fd, struct icmplist *l, int count);


