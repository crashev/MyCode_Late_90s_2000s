#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <rrd.h>
#include <unistd.h>
#include <stdarg.h>

int main() {
        char *trafRRD[13]= {
          "rrdcreate",
          "",
          "DS:input:DERIVE:600:0:12500000",
          "DS:output:DERIVE:600:0:12500000",
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

        int  iRes = 0;

//                printf("[%s] does not exist.. Creating!\n", cPath);
                optind = opterr = 0; rrd_clear_error();
                trafRRD[1]="to.rrd";
                iRes = rrd_create(12, trafRRD);
                if (iRes != 0) 
		{
                        printf("RRD error creating -> %s\n", rrd_get_error());
                        return -1;
		}
return 0;
}
