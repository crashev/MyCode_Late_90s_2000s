#include <stdio.h>
#include <unistd.h>
#include <rrd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>

int file_exist(char *pcFname) 
{
	    struct stat st;
	    return stat(pcFname, &st);
}

unsigned long getRXTX(char *ifc, int rxtx) {
     static char iface[50],read[50],write[50]; 
     int ret;
     FILE *in = fopen("/proc/net/dev", "r");
     if (!in)
         perror("can't open");
        fscanf(in, "%*[^\n]\n%*[^\n]"); /* skip two lines */

	while ( (ret = fscanf(in, " %16[^:]:%s %*s %*s %*s %*s %*s %*s %*s %s%*[^\n]\n",iface,read,write))==3) {
	    if (!strncmp(iface,ifc,strlen(ifc))) {
//		RXTX[0]=read;
//		RXTX[1]=write;
		if (rxtx==0) return atol(read); else return atol(write);
	    }
	}
    	
    fclose(in);
    return 0;
}


int main(int argc, char *argv[]) {
	char buf[255];
	char *test[]= {"", 
	"pawel.rrd",
	"--start",
	"N",
	"DS:speed:COUNTER:600:U:U",
	"RRA:AVERAGE:0.5:1:24", // usrednia 5 minutowe probki - 24 takich 3ma
	"RRA:AVERAGE:0.5:6:10", // usrednia 6*5m probki i trzyma takich 10
	NULL};
	char *test2[]= {"",
	  "myrouter.rrd",
          "DS:input:COUNTER:600:U:U",
          "DS:output:COUNTER:600:U:U",
          "RRA:AVERAGE:0.5:1:600",
          "RRA:AVERAGE:0.5:6:700",
          "RRA:AVERAGE:0.5:24:775",
          "RRA:AVERAGE:0.5:288:797",
          "RRA:MAX:0.5:1:600",
          "RRA:MAX:0.5:6:700",
          "RRA:MAX:0.5:24:775",
          "RRA:MAX:0.5:288:797",
	  NULL
	  };
	char *updparams[5];
	int result;
	
	argc = optind = opterr = 0; 
	*argv= NULL;
	rrd_clear_error();

if (file_exist(test2[1])<0) {
	printf("[%s] does not exist.. Creating!\n", test2[1]);    	
	result = rrd_create(12, test2);
	if (result != 0) {
		printf("RRD error creating -> %s\n", rrd_get_error());
		return 1;
	}
} else 	printf("[%s] Exist.. Creation skipped!\n", test2[1]);    	

	//	char *updparams[] = { "rrdupdate", filedir, "-t", template, rrdvalues, NULL };
	updparams[0]="rrdupdate";
	updparams[1]="myrouter.rrd";
//	updparams[2]="N:100:200";
	updparams[3]=NULL;
	updparams[4]=NULL;
	
	while (1) {
		bzero(buf, sizeof(buf));
		snprintf(buf, sizeof(buf) - 1, "N:%lu:%lu", getRXTX("eth0",0), getRXTX("eth0",1));
		printf("[BUF]: %s \n", buf);
		optind = opterr = 0; 
		rrd_clear_error();
		argc=0; argv=NULL;
		updparams[2]=buf;
		result = rrd_update(3, updparams);
		sleep(60*5); // sleep for 5 minutes
	}

    return 0;
}
