/*
 * ircscan.h
 */
#ifndef IRCSCAN_H
#define IRCSCAN_H

#ifndef IPCOUNTER
typedef unsigned long  COUNTER;
#endif

/*
 * raw input data:
 *
 * JOIN
 *	ipsrc,		client or server
 *	ipdst,		server
 *	tcpdst,		port used
 *	char *channel   channel name desired
 *
 * PING
 *	ipsrc,		server 
 * 	ipdst,		client or server
 *	tcpdst,		port used
 *	char *recv1	server who sent ping 	
 *
 * PONG
 *	ipsrc,		client or server
 *	ipdst,		server
 *	tcpdst,		port used
 *	char *daemon    name of PONG sender
 */

/* output data
 *
 * any ip address has fundamental data,
 * and may point to other ip addresses in the last
 * or channels.
 */

/*
 * sort by total counter count
 */
struct ircip {
	unsigned long ipaddr;	
	unsigned long tcpdst;	    /* last seen dst port */
	COUNTER joinlhs;	    /* client > server */
	COUNTER joinrhs;	    /* server */
	COUNTER pinglhs;	    /* server */
	COUNTER pingrhs;	    /* client OR server */
	COUNTER ponglhs; 	    /* client OR server */
	COUNTER pongrhs;	    /* server */
	struct ircip *nextip;       /* hashlist of ip associations */
	struct ircip *chanip;       /* list of ips associated by channel */
};

struct bareip {
	unsigned long ipaddr;
	struct bareip *nextip;
};

/*
 * not keeping the key for "privacy" protection
 * of course any ASCII password is not terribly private.
 */
struct ircchan {
	char *name;	/* channel name unique key */
	COUNTER hits;   /* how many times uttered total, sort on this */
	COUNTER noips;  /* total number of ips in channel */
	struct ircchan *next;   /* lookup: hashlist of channels */
	/* note this is just a chain of allocated pointers
	 * also important: struct ircip
	*/ 
	struct bareip *chanip;	    /* list of associated ips */
};


/* can be longer but we will heuristically assume 
 * 32 chars as max length
 *
 * note: 1st char either # or &
 * end denoted usually by blanks,
 * formal end:
 * [SPACE, COMMA, NUL, Cntrl-G]
 */

#define CHARSPACE	' '
#define CHARCOMMA       ','
#define CHARNUL         0x0
#define CHARBELL        0x7

#define MAXCHANNAME 32

/* a list itself.  it has pointers to the linked-list
 * and a count of elements AND a hash access mechanism
 * for fast searching, based on node keys.
 */
struct irclist {
	unsigned long ipcount;	 /* entries in ip hashlist */
	unsigned long chancount; /* entries in chan hashlist */
	unsigned long totaljoins;  /* total joins */
	unsigned long totalpings;  /* total pings */
	unsigned long totalpongs;  /* total pongs */
	unsigned long totalprivmsgs;  /* total privmsgs */
/*
#define SHASHSIZE	1021
*/
#define IRCHASH	32717
	struct ircip *iphash[IRCHASH];
	struct ircchan *chanhash[IRCHASH];
};


#endif 

