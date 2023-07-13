 /*
  * hashport.h 
  */
  
#ifndef PORTCOUNTER
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

struct pnode {
	unsigned short port; 
	unsigned long bytecount;
	unsigned long srccounter;
	unsigned long dstcounter;
	struct pnode *next;
};

#define HSIZE 1031 
struct plist{
	unsigned long entries;
	struct pnode *last;
	struct pnode *hashlist[HSIZE];
};

static unsigned long e_hash(unsigned char *, int size);

/**************************************************/
/* function prototypes */

struct pnode *
search_srcport(struct plist *l, USHORT srcport, ULONG bytecount);

struct pnode *
search_dstport(struct plist *l, USHORT dstport, ULONG bytecount);

int insert_srcport(struct plist *l, USHORT srcport, ULONG bytecount);
int insert_dstport(struct plist *l, USHORT dstport, ULONG bytecount);
void destroy_plist(struct plist *l);
void init_plist(struct plist *l);
void print_tcpplist(FILE *fd, struct plist *l, int tcpflag, int count);
void print_udpplist(FILE *fd, struct plist *l, int udpflag, int count);

