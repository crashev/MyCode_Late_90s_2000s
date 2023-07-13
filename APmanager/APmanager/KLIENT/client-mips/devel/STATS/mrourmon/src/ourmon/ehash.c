/* hash function
*/
static unsigned long __inline
ehash (unsigned char *name, int size)
{
	unsigned long h = 0, g;

	while (size--) {
		h = (h << 4) + *name++;
		if ( g = h & 0xF0000000 )
			h ^= g >> 24;
		h &= ~g; 
	}
	return(h);
}

