/*
 * hashscan.h
 */
#ifndef DHASHSCAN_H
#define DHASHSCAN_H

#ifndef IPCOUNTER
typedef unsigned long  COUNTER;
#endif

struct sipkey {
	unsigned long ipdst;
	unsigned long ipsrc;
	unsigned short srcport;
	unsigned short dstport;
	unsigned short ipproto;
};

/* an individual doubly-linked list element.
 * it has a key and a counter as data
 */
struct snode {
	struct snode *hnext;		/* chain ptr on hash list */
	struct sipkey ipkey;
	unsigned long counter;		/* byte count */
};

/* a list itself.  it has pointers to the linked-list
 * and a count of elements AND a hash access mechanism
 * for fast searching, based on node keys.
 */
struct slist {
	unsigned long count;	/* entries in hashlist */
	unsigned long total;	/* total bytes in list */
	/* TBD. not needed
	*/
	unsigned long tcpcount;
	unsigned long udpcount;
	unsigned long icmpcount;
	/* hash access */
	struct snode *last;	/* cached pointer to last entry in hashlist */
/*
#define SHASHSIZE	1021
*/
#define SHASHSIZE	32717
	struct snode *hashlist[SHASHSIZE];
};

/*This structure is intended to keep statistics about hash usage
 *in the hash list structures
 */
struct hashStats{
	unsigned long count; 	/*total attempted insertions*/
	unsigned long hits;     /*already inserted data */		
	unsigned long misses;  /*newly inserted data*/
	unsigned long collisions;/*inserted into existing chain*/
	unsigned long maxChain;  /*longest chain*/
};

void initStats(struct hashStats *stats);
void printStats(FILE *fd, char* prefix, struct hashStats *stats);

/**************************************************/
/* function prototypes */

/*These are several functions that extract a hash key from
 *and ipkey.
 */
typedef unsigned long (*func)(struct sipkey*);
extern unsigned int debug2;
unsigned long hashSrcDst(struct sipkey*);
unsigned long hashSrc(struct sipkey*);
unsigned long hashSrcPort(struct sipkey*);

/* This is a hashlist search function that is parametrized to
 * uses a hashing function passed in as a parameter 
 */
struct snode *
searchKeyNode(struct slist *l, struct sipkey *ipkey, 
	   COUNTER counter, func searchKeyFunction);

/* This digests a topn_style hashlist down to a hashlist that
 * reports only on SRCIP digested information
 */
void processTopNScans(struct slist *input, struct slist *output,
			struct hashStats *stats);


/* This is probably redundant, but since I only use it for debugging
 * it should not reduce performance much.
 */
char *intoa2(unsigned int);

/* This is an insert function that is parameterized by a 
 * hashing function
 */
int
insertKeyNode(struct slist *l, struct sipkey *ipkey, 
	   COUNTER counter, func searchKeyFunc, struct hashStats *stats);

/* deallocate all allocated nodes in list l.  do NOT deallocated l
 * itself.
 */
void
destroyScanList(struct slist *l);

/* called before list can be used
*/
void
initScanList(struct slist *l);

/*
 * debug walk of list
 */
static void
debugScanList(struct slist *l);

/*For debugging, dumps an ipkey to stdout*/
void dumpIpkey(struct sipkey *ik);

int printSingleIPList(FILE *fd, struct slist *l, int anyflag, int tcpflag, int udpflag, int icmpflag, int count, char* prefix);

#endif 

