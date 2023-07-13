/*
    Funckja tworzaca skrypty QOSu
*/

#define CONFIGS_DIR "configs"


#include <stdio.h>
#include <fcntl.h>
#include "dbmod.h"
#include "config.h"
#include <sys/stat.h>
		
		
		
static int
prepare_configs_dir(char *pcPtr)
{
            struct stat st;
            if (stat(pcPtr, &st)<0) {
	            if (mkdir(pcPtr, 0777)<0) {
                            return -1;
    }
            return 0;
} else if (!S_ISDIR(st.st_mode)) {
        return -1;
}
return 0;
}
				


char *qos_file = 
"cat > /etc/init.d/S49QoS <<EOF\n\
#!/bin/sh\n \
INS=\"/sbin/insmod\" \n\
$INS ipt_CONNMARK\n\
$INS ipt_connbytes\n\
$INS cls_u32\n\
\n\
IPT=\"/usr/sbin/iptables\";\n\
TC=\"/usr/sbin/tc\";\n\
DEV=\"eth1\";\n\
\n\
$IPT -t mangle -F PREROUTING\n\
$IPT -t mangle -F POSTROUTING\n\
\n\
$TC qdisc del dev $DEV root\n\
$TC qdisc add dev $DEV root handle 1:0 htb r2q 1 default 1\n\
$TC class add dev $DEV parent 1:0 classid 1:1 htb rate 1024kbit burst 20k quantum 60000\n\
\n\
$IPT -t mangle -A PREROUTING -j CONNMARK --restore-mark\n\
$IPT -t mangle -A PREROUTING -m mark ! --mark 0 -j ACCEPT\n\
$IPT -t mangle -A PREROUTING -j CONNMARK --save-mark\n\
\n\
# Nasz licznik\n \
a=100;\n\
\n\
dodaj() {\n\
\n\
if [  \"$3\" != \"0\" ]; then\n\
    $TC class add dev $DEV parent 1:1 classid 1:$a htb rate ${3}kbit prio 0\n\
    $TC qdisc  add dev $DEV parent 1:$a handle $a:1 sfq perturb 10\n\
    $TC filter add dev $DEV protocol ip parent 1:0 prio 0 u32 match ip dst $1 flowid 1:$a\n\
    let a=$a+1\n\
fi\n\
    $IPT -t mangle -A POSTROUTING -p tcp -d $1 -m connbytes --connbytes $4:999999999 --connbytes-dir both --connbytes-mode bytes -j CONNMARK --set-mark $a\n\
\n\
    $TC class add dev $DEV parent 1:1 classid 1:$a htb rate ${2}kbit prio 0\n\
    $TC qdisc  add dev $DEV parent 1:$a handle $a:1 sfq perturb 10\n\
    $TC filter add dev $DEV protocol ip parent 1:0 prio 0 handle $a fw flowid 1:$a\n\
\n\
    let a=$a+1\n\
\n\
}\n\
\n\
";

int 
config_make_QoS(char *pcHostIp, char *pcName) 
{
	    PGconn *pgconn;
	    PGresult *pgres;
	    FILE *fQoS;
	    char cPath[255];
	    int iRow,iCol;
	    int iX,iY;
	    
	    bzero(cPath, sizeof(cPath));
	    snprintf(cPath, sizeof(cPath) - 8, "%s/%s-%s/clientsaddons", CONFIGS_DIR, pcHostIp, pcName);

            if (prepare_configs_dir(cPath)<0)
		return -1;
	    
	    strncat(cPath,"/QoS", sizeof(cPath) - 1);
	    if ( (fQoS = fopen(cPath, "w"))==NULL )
		return -1;
	    fprintf(fQoS,"%s", qos_file);
	    
	    pgconn = pgsql_polacz(DB_USER, DB_PASSWORD, DB_BAZA);    
	    if (!pgconn) {
		fclose(fQoS);
	    	pgsql_zakoncz(pgconn);
		return -1;
	    }
	    /* Ok now real SQL thingie */
	
	    pgres = pgsql_query(pgconn, "select al.hostip,qos.bw,qos.bw_max,qos.bw_traffic from access_lists al, access_points ap, queues qos where ap.id = al.aid AND qos.id = al.qid AND ap.apip='%s' AND ap.apname='%s';", pcHostIp, pcName);
	    
	    if (!pgres) {
		fclose(fQoS);
		pgsql_zakoncz(pgconn);
	        return -1;	
	    }

	
	    iRow = PQntuples(pgres);   // ile rekordow
	    iCol = PQnfields(pgres); // ile kolumn ma resultat
	
	for (iX = 0; iX<iRow; iX++) {
//	    for (iY=0; iY<iCol; iY++) {
		fprintf(fQoS, "dodaj %s\t%s\t%s\t%s\n",PQgetvalue(pgres, iX, 0), PQgetvalue(pgres, iX, 1)
		, PQgetvalue(pgres, iX, 2)
		, PQgetvalue(pgres, iX, 3)
		);
//	    }
	}
	PQclear(pgres);
	PQfinish(pgconn);
	fprintf(fQoS, "EOF\n");
	fclose(fQoS);
	return 0;
}

int main()
{
    int ret = config_make_QoS("83.145.178.186","PawqLinksysRouterv3");
    printf("[?] Ret: %d \n", ret);
}
