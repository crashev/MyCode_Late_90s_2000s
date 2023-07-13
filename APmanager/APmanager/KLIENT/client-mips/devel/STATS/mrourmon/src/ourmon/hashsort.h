/*
 * hashsort.h
 */
#ifndef DHASHSORT_H
#define DHASHSORT_H

#ifndef IPCOUNTER
typedef unsigned long  COUNTER;
#endif

struct ipkey {
	unsigned long ipdst;
	unsigned long ipsrc;
	unsigned short srcport;
	unsigned short dstport;
	unsigned short ipproto;
};

/* an individual single-linked list element.
 * it has a key and a counter as data
 */
struct node {
	struct node *hnext;		/* chain ptr on hash list */
	struct ipkey ipkey;
	unsigned long counter;		/* byte count */
};

/* a list itself.  it has pointers to the linked-list
 * and a count of elements AND a hash access mechanism
 * for fast searching, based on node keys.
 */
struct list {
	unsigned long count;	/* entries in hashlist */
	unsigned long total;	/* total bytes in list */
	unsigned long tcpcount;
	unsigned long udpcount;
	unsigned long icmpcount;
	/* hash access */
	struct node *last;	/* cached pointer to last entry in hashlist */
/* these numbers are all primes
*/
/* #define HASHSIZE	1031  
#define HASHSIZE	4091
#define HASHSIZE	7919
#define HASHSIZE	16403
#define HASHSIZE	32717
#define HASHSIZE	104729
*/
#define HASHSIZE	241441
	struct node *hashlist[HASHSIZE];
};

/**************************************************/
/* function prototypes */

/* searchNode
 *	for a given list, and a given key, find a node
 * 	in the list that matches that key
 *	returns:
 *		N - node pointer
 *		NULL - no such node
 */
struct node *
searchNode(struct list *l, struct ipkey *ipkey, COUNTER counter);

/* given a list, and a node (possible new), make sure node
 * is in proper sorted place in list; i.e., 
 * given node N and N-1, counter for N is <= N-1
 *
 * if foundflag is TRUE, n is in the list, and we sort up
 * else we must make a node, and sort N up from the tail
 * of the list.
 */
int
insertNode(struct list *l, struct ipkey *ipkey, COUNTER c);
 
void
searchInsertNode(struct list *l, struct ipkey *ipkey, COUNTER c);

/* deallocate all allocated nodes in list l.  do NOT deallocated l
 * itself.
 */
void
destroyList(struct list *l);

/* called before list can be used
*/
void
initList(struct list *l);

/*
 * debug walk of list
 */
void
debugList(struct list *l);

int printList(FILE *fd, struct list *l, int anyflag, int tcpflag, int udpflag, int icmpflag, int count); 

int printPrefixList(FILE *fd, struct list *l, int anyflag, int tcpflag, int udpflag, int icmpflag, int count, char* prefix); 

#endif 

