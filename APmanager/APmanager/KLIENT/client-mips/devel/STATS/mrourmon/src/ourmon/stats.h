struct ourstats {
	/* hashsort */
	unsigned long hs_chits;	/* topn hashsort cache hits */
	unsigned long hs_cmiss; /* topn hashsort cache miss */
	unsigned long hs_bfirst; /* found in first hash bucket */
	unsigned long hs_bother; /* found in other hash bucket */
	unsigned long hs_bmiss;  /* not found in hash chain (insert) */
	unsigned long hs_coll;   /* insert collision */
	unsigned long hs_insert; /* insert count */
	unsigned long hs_ecount; /* number of empty buckets in array */
	unsigned long hs_hcounter; /* number of hash buckets total */
#define CSIZE 7
	unsigned long hs_csize[CSIZE];	/* chain length */

	/* hashport src only */
	unsigned long hp_chits;	/* topn hashport cache hits */
	unsigned long hp_cmiss; /* topn hashport cache miss */
	unsigned long hp_bfirst; /* found in first hash bucket */
	unsigned long hp_bother; /* found in other hash bucket */
	unsigned long hp_bmiss;  /* not found in hash chain (insert) */
	unsigned long hp_coll;   /* insert collision */
	 

	
};

extern struct ourstats os;

