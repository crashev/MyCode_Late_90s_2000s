#include <stdio.h>

char *StatsTraffic()
{
        static char scRetBuf[1024];
        static char iface[50],read[50],write[50]; 
        int ret;
	FILE *in = fopen("/proc/net/dev", "r");
        if (!in)
	     return NULL;

        bzero(scRetBuf, sizeof(scRetBuf));
    
	fscanf(in, "%*[^\n]\n%*[^\n]"); /* skip two lines */

	while ( (ret = fscanf(in, " %16[^:]:%s %*s %*s %*s %*s %*s %*s %*s %s%*[^\n]\n",iface,read,write))==3) {
	    if (!strncmp(iface,"eth",3)) {
	        strncat(scRetBuf,"STATS NazwaHosta TRAFFIC ", sizeof(scRetBuf)-1);
		strncat(scRetBuf,iface, sizeof(scRetBuf));strcat(scRetBuf," ");
		strncat(scRetBuf,read, sizeof(scRetBuf));strcat(scRetBuf," ");
		strncat(scRetBuf,write, sizeof(scRetBuf));
		strncat(scRetBuf,"\n", sizeof(scRetBuf) - 1);
	    }
	}
        fclose(in);
    
        return scRetBuf;
}


int main() {
	printf("\n%s \n", StatsTraffic());
	return 0;
}


