#include	<stdlib.h>
#include	<stdio.h>
#include	<errno.h>
//#include	<file.h>
#include	<signal.h>

#define ERROR_EXIT exit(errno)

static unsigned char b64_bin[256];	/* => 0..63, pad=64, inv=65, else ign */

/***/

/* setup B64 (ascii) => binary table */
static void init_b64bin(void)
{
	int i,j;

	for(i = 0; i <= 32; i++) b64_bin[i] = 66;	/* ignore controls */
	for(i = 33; i < 127; i++) b64_bin[i] = 65;	/* printable => inv */
	for(i = 127; i <= 160; i++) b64_bin[i] = 66;	/* ignore controls */
	for(i = 161; i < 256; i++) b64_bin[i] = 65;	/* printable => inv */

	j = 0;
	for(i = 'A'; i <= 'Z'; i++, j++) b64_bin[i] = j;
	for(i = 'a'; i <= 'z'; i++, j++) b64_bin[i] = j;
	for(i = '0'; i <= '9'; i++, j++) b64_bin[i] = j;
	b64_bin['+'] = j++;
	b64_bin['/'] = j++;

	b64_bin['='] = 64;
}


/* get next 6-bit group, or pad */
static int GNC_b64bin (unsigned int *ip,FILE *finp)
{
	int c;

	do {
		c = fgetc(finp);
		if(c == EOF) return EOF;
		*ip = b64_bin[c & 0xFF];
	} while(*ip > 65);

	return 0;
}

/* decode => stdout */
static void do_decode (FILE *finp, FILE *fout)
{
int	done;
unsigned int v6;
unsigned int v24;		/* room for >= 24 bits */
char	c;

#define ERROR exit(2)

	done = 0;
	while(GNC_b64bin(&v6,finp) != EOF) {
		if(done) ERROR;		/* bytes following pad */

		if(v6 > 63) ERROR;
		v24 = v6;
		if(GNC_b64bin(&v6,finp) == EOF) ERROR;
		if(v6 > 63) ERROR;
		v24 = (v24 << 6) | v6;

		if ( ((v24 >> (2*6-8)) & 0xFF) != '\r' )
			fputc((v24 >> (2*6-8)) & 0xFF,fout);

		if(GNC_b64bin(&v6,finp) == EOF) ERROR;
		if(v6 > 64) ERROR;	/* invalid */
		if(v6 == 64) {			/* pad after 1 char */
			done = 1;
			if(GNC_b64bin(&v6,finp) == EOF) ERROR;
			if(v6 != 64) ERROR;
		} else {
			v24 = (v24 << 6) | v6;

			if ( ((v24 >> (3*6-16)) & 0xFF) != '\r' )
				fputc((v24 >> (3*6-16)) & 0xFF,fout);

			if(GNC_b64bin(&v6,finp) == EOF) ERROR;
			if(v6 > 64) ERROR;	/* invalid */
			if(v6 == 64) {		/* pad after 2 char */
				done = 1;
			} else {
				if ( (((v24 << 6) | v6) & 0xFF) != '\r' )
					fputc(((v24 << 6) | v6) & 0xFF,fout);

			}
		}
	}

	exit(0);
}

main(int argc, char **argv)
{
FILE	*finp,*fout;

	if ( argc < 3 )
		{
		printf("Usage:\nb64decode <inpfile> <outfile>\n");
		exit(0);
		}

	if ( !(finp = fopen(argv[1],"r")) )
		{
		perror("Open input file");
		exit(0);
		}

	if ( !(fout = fopen(argv[2],"w")) )
		{
		perror("Open output file");
		exit(0);
		}


	init_b64bin ();

	do_decode (finp,fout);
}
