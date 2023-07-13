#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "dbmod.h"
#include "server.h"

//#define CONFIGS_DIR	"configs"


/*
select al.macaddr, al.hostip from access_lists al, access_points ap
where ap.apid = al.apid AND ap.apip='192.168.1.1';

  config_make_ethers("192.168.1.1");
  config_make_ethers("192.168.1.2");
  config_make_package("192.168.1.1");
  config_make_package("192.168.1.2");
*/

static int
exist(char *pcPath)
{
	struct stat st;
        return(stat(pcPath, &st));
}



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

int 
config_make_ethers(char *pcHostIp, char *pcName) 
{
	    PGconn *pgconn;
	    PGresult *pgres;
	    FILE *fEthers;
	    char cPath[255];
	    int iRow,iCol;
	    int iX,iY;
	    
	    bzero(cPath, sizeof(cPath));
	    snprintf(cPath, sizeof(cPath) - 8, "%s/%s-%s/ethers", CONFIGS_DIR, pcHostIp, pcName);

	    if ( (fEthers = fopen(cPath, "w"))==NULL )
		return -1;
	
	    pgconn = pgsql_polacz(DB_USER, DB_PASSWORD, DB_BAZA);    
	    if (!pgconn) {
		fclose(fEthers);
	    	pgsql_zakoncz(pgconn);
		return -1;
	    }
	    /* Ok now real SQL thingie */
	
	    pgres = pgsql_query(pgconn, "select al.macaddr, al.hostip from access_lists al, access_points ap where ap.id = al.aid AND ap.apip='%s' AND ap.apname='%s';", pcHostIp, pcName);

	    if (!pgres) {
		fclose(fEthers);
		pgsql_zakoncz(pgconn);
	        return -1;	
	    }

	
	    iRow = PQntuples(pgres);   // ile rekordow
	    iCol = PQnfields(pgres); // ile kolumn ma resultat
	
	for (iX = 0; iX<iRow; iX++) {
//	    for (iY=0; iY<iCol; iY++) {
		fprintf(fEthers, "%s,id:*\t%s\n",PQgetvalue(pgres, iX, 0), PQgetvalue(pgres, iX, 1));
//	    }
//		fprintf(fEthers,"\n");
	}
	PQclear(pgres);
	PQfinish(pgconn);
	fclose(fEthers);
	return 0;
}


int
config_make_clientsconfig(char *pcHostIp, char *pcName) 
{
	FILE *fPackage;
	FILE *fConfig;
	char cTmpBuf[1024];
	char cPath[255];

        bzero(cPath, sizeof(cPath));
        snprintf(cPath, sizeof(cPath) - 8, "%s/%s-%s", CONFIGS_DIR, pcHostIp, pcName);
        if (prepare_configs_dir(cPath)<0)
		return -1;

	strcat(cPath, "/clients.sh");
	/* if exist then lets make nice package */
	fPackage = fopen(cPath, "w");
	if (!fPackage)
	    return -1;
	    
	/* Headers */
	fprintf(fPackage,"#!/bin/sh\n#Package ver 1.0\n");

	bzero(cPath, sizeof(cPath));
	snprintf(cPath, sizeof(cPath) - 1,"%s/%s-%s/ethers", CONFIGS_DIR, pcHostIp, pcName);
	if ( (fConfig = fopen(cPath, "r")) == NULL) {
	    fclose(fPackage);
	    return -1;
	}
	fprintf(fPackage,"cat > /etc/ethers << EOF\n");
	bzero(cTmpBuf, sizeof(cTmpBuf));
	
	while (fgets(cTmpBuf, sizeof(cTmpBuf) - 1, fConfig)) {
	    fprintf(fPackage,"%s", cTmpBuf);
	    bzero(cTmpBuf, sizeof(cTmpBuf));
	}
	fprintf(fPackage,"EOF\n");
	fprintf(fPackage,"wifi;\n");
	fprintf(fPackage,"killall -9 dnsmasq;/etc/init.d/S50dnsmasq;\n");	
	fprintf(fPackage,"/etc/init.d/S44firewall;\n");	
	fclose(fConfig);
	fclose(fPackage);
	
	return 0;
}







int
config_make_config(char *pcHostIp, char *pcName) 
{
	FILE *fPackage;
	char cPath[255];

        bzero(cPath, sizeof(cPath));
        snprintf(cPath, sizeof(cPath) - 8, "%s/%s-%s", CONFIGS_DIR, pcHostIp, pcName);
        if (prepare_configs_dir(cPath)<0)
		return -1;
		

	strcat(cPath, "/package.sh");

	fPackage = fopen(cPath, "w");
	if (!fPackage)
	    return -1;
	    
	// Headers 
	fprintf(fPackage,"#!/bin/sh\n#Package ver 1.0\n");

	fclose(fPackage);
	
	return 0;
}


int
config_make_additional(char *pcHostIp, char *pcFile, char *pcDir, char *pcName) 
{
	FILE	 *fPackage;
	FILE	 *fAdd;
	char 	 cTmpBuf[1024];
	char 	 cPath[255];
	char     cPackagePath[255];
	DIR	 *dir;
	struct   dirent *dS;
	int 	 iLen;
	
	bzero(cPackagePath, sizeof(cPackagePath));		
        bzero(cPath, sizeof(cPath));
        snprintf(cPackagePath, sizeof(cPackagePath) - 1, "%s/%s-%s/%s", CONFIGS_DIR, pcHostIp, pcName, pcFile);
	

        if (exist(cPackagePath)<0) 
		    return -1; // no addons dir
    
	snprintf(cPath, sizeof(cPath) - 1,"%s/%s-%s/%s/", CONFIGS_DIR, pcHostIp, pcName, pcDir);
        if (exist(cPath)<0) 
		    return -1; // no addons dir
	
	/* list addons files and attach 'em */
	dir = opendir(cPath);
	if (!dir) 
	    return -1;
	
	/* if exist then lets make package+addons */
	fPackage = fopen(cPackagePath, "a");
	if (!fPackage)
	    return -1;
	    
	while ( (dS = readdir(dir)) ) 
	{
	    if ( (!strcmp(dS->d_name, "."))||(!strcmp(dS->d_name, "..")) )
		continue;
	    
	    bzero(cPath, sizeof(cPath));
	    snprintf(cPath, sizeof(cPath) - 1,"%s/%s-%s/%s/%s", CONFIGS_DIR, pcHostIp, pcName, pcDir, dS->d_name);
	    fAdd = fopen(cPath, "r");
	    if (!fAdd)
		break;
	    bzero(cTmpBuf, sizeof(cTmpBuf));
	    while ( (iLen = fread(cTmpBuf, 1, sizeof(cTmpBuf) - 1, fAdd)) ) {
		    fwrite(cTmpBuf, 1, iLen, fPackage);
		    bzero(cTmpBuf, sizeof(cTmpBuf));
	    }
	    fclose(fAdd);
	}
	
	fclose(fPackage);
	closedir(dir);
	return 0;
}


/*
    Funckja tworzaca skrypty QOSu
*/
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
