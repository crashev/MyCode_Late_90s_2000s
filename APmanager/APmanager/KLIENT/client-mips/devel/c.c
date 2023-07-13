#include <stdio.h>
#include <unistd.h>
void usage(char *p) {
    printf("Usage: %s -a [ApName] -m [MainHost:Port] \n", p);
    exit(0);
}
static void 
putit(char *pcSrc, char *pcDst, int iSize) 
{
    bzero(pcSrc, iSize);
    strncpy(pcSrc, pcDst, iSize-1);
}

int main(int argc,char *argv[]) 
{
	char cApName[10], cMainHost[255];
	int opterr, daemon_mode, quiet, debug;
	int iCh = 0;
	char *pcProg = argv[0];
	opterr = 0;

	printf("cAp1: %s \n", cApName);
	printf("Sizeof cAp: %d \n", sizeof(cApName));
	putit(cApName, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", sizeof(cApName));
	printf("cAp2: %s \n", cApName);
		
return 0;
}
