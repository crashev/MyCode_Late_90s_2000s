/*

    Plik zawierajacy SPECYFIKACJE protkolu, wszelkie komendy protokolu
    rozpoznawane, obslugiwane i dodawane sa tutaj.
    (C) Komunix - Pawel Furtak '2005
    // Client    
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>       
#include <rrd.h>

#include <pthread.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <zlib.h>

#include "config.h"
#include "server.h"

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

struct client_struct *globalClient = NULL;

void
reply(BIO *bio, char *pcPtr)
{
    if (globalClient->stat)
        bio_printf(bio, "%s", pcPtr);
}



static int 
cmd_void(char **pcParam, int iParamCount)
{
	    
	return 0;
}



static int 
cmd_setname(char **pcParam, int iParamCount)
{
	if ((iParamCount<2)||(!*pcParam))
		    return -1;
	    
	printf("[%s]-[SETNAME]=> %s \n", globalClient->host, *pcParam);
	bzero(globalClient->name, sizeof(globalClient->name));
	strncpy(globalClient->name, *pcParam, sizeof(globalClient->name) - 1);

	bzero(globalClient->cversion, sizeof(globalClient->cversion));
	strncpy(globalClient->cversion, pcParam[1], sizeof(globalClient->cversion) - 1);

	return 0;
}


static int 
file_exist(char *pcFname) 
{
	    struct stat st;
	    return stat(pcFname, &st);
}


static int
prepare_stats_dir(char *pcPtr) 
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



static int
cmd_stats_traffic(char **pcParam, int iParamCount)
{
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
	char *updparams[3];
	char cPath[255];
	char cTmp[50];
	int  iRes = 0;
	
	bzero(cPath, sizeof(cPath));
	snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s/%s", STATS_DIR, *pcParam, globalClient->host, pcParam[1]);

	if (file_exist(cPath)<0) {
		printf("[%s] does not exist.. Creating!\n", cPath);    		
		optind = opterr = 0; rrd_clear_error();
		trafRRD[1]=cPath;
		iRes = rrd_create(12, trafRRD);
		if (iRes != 0) {
			printf("RRD error creating -> %s\n", rrd_get_error());
			return -1;
		}
	} else 	printf("[%s] Exist.. Creation skipped!\n", cPath);    	

		bzero(cTmp, sizeof(cTmp));
		snprintf(cTmp, sizeof(cTmp) - 1, "N:%s:%s", pcParam[2], pcParam[3]);
	        updparams[0]="rrdupdate";
		updparams[1]=cPath;
		updparams[2]=cTmp;
	
		optind = opterr = 0; 
		rrd_clear_error();
		return ( rrd_update(3, updparams));

}


static int
cmd_stats_iptraffic(char **pcParam, int iParamCount)
{
	char *iptrafRRD[33]= {
	  "rrdcreate",
	  "",
          "DS:20_in:DERIVE:600:0:12500000",
          "DS:20_out:DERIVE:600:0:12500000",
          "DS:21_in:DERIVE:600:0:12500000",
          "DS:21_out:DERIVE:600:0:12500000",
          "DS:22_in:DERIVE:600:0:12500000",
          "DS:22_out:DERIVE:600:0:12500000",
          "DS:25_in:DERIVE:600:0:12500000",
          "DS:25_out:DERIVE:600:0:12500000",
          "DS:80_in:DERIVE:600:0:12500000",
          "DS:80_out:DERIVE:600:0:12500000",
          "DS:110_in:DERIVE:600:0:12500000",
          "DS:110_out:DERIVE:600:0:12500000",
          "DS:119_in:DERIVE:600:0:12500000",
          "DS:119_out:DERIVE:600:0:12500000",
          "DS:143_in:DERIVE:600:0:12500000",
          "DS:143_out:DERIVE:600:0:12500000",
          "DS:443_in:DERIVE:600:0:12500000",
          "DS:443_out:DERIVE:600:0:12500000",
          "DS:993_in:DERIVE:600:0:12500000",
          "DS:993_out:DERIVE:600:0:12500000",
          "DS:995_in:DERIVE:600:0:12500000",
          "DS:995_out:DERIVE:600:0:12500000",
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
	char *updparams[3];
	char cPath[255];
	char cTmp[250];
	int  iRes = 0;
	
	bzero(cPath, sizeof(cPath));
	snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s/%s", STATS_DIR, *pcParam, globalClient->host,  pcParam[1]);

	if (file_exist(cPath)<0) {
		printf("[%s] does not exist.. Creating!\n", cPath);    		
		optind = opterr = 0; rrd_clear_error();
		iptrafRRD[1]=cPath;
		iRes = rrd_create(32, iptrafRRD);
		if (iRes != 0) {
			printf("RRD error creating -> %s\n", rrd_get_error());
			return -1;
		}
	} else 	printf("[%s] Exist.. Creation skipped!\n", cPath);    	

		bzero(cTmp, sizeof(cTmp));
		snprintf(cTmp, sizeof(cTmp) - 1, "N:%s", pcParam[2]);
	        updparams[0]="rrdupdate";
		updparams[1]=cPath;
		updparams[2]=cTmp;
		
		optind = opterr = 0; 
		rrd_clear_error();
		iRes = rrd_update(3, updparams);
		
		if (iRes!=0) 
			printf("RRD error creating -> %s\n", rrd_get_error());
		return iRes;


}


static int 
cmd_stats(char **pcParam, int iParamCount)
{
	char cPath[255];

	if ((iParamCount<2)||(!*pcParam))
		    return -1;


	bzero(cPath, sizeof(cPath));
	snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s", STATS_DIR, pcParam[1], globalClient->host);
	
	if (prepare_stats_dir(cPath)<0)
		    return -1;

	    
	if (!strcmp(*pcParam, "TRAFFIC"))
	    return(cmd_stats_traffic(pcParam+1, iParamCount-1));

	if (!strcmp(*pcParam, "IPTRAFFIC"))
	    return(cmd_stats_iptraffic(pcParam+1, iParamCount-1));
	    
	return -1;
}


unsigned long 
sum(char *pcFile)
{
	unsigned long ucSum = 0;
	unsigned char ucZnak = 0;
	int file;
	
	if ( (file = open(pcFile, O_RDONLY))<0 ) 
	    return 0;

	while (read(file, &ucZnak, 1))
	    ucSum = ucSum + ( ((ucZnak*2)^0x22) + (((ucZnak*2)%10)^0x45) );

	close(file);
	
	return ucSum;
    
}

/*
    XORed send
*/

static int
sendit(struct client_struct *myClient, char *pcFile)
{
    char cBuf[1024];
    struct sockaddr_in sin,csin;
    FILE *fPackage;
    int iSock;
    fd_set fds;
    struct timeval timeout;
    int iSelectRet;
    int iMaxfds;
    int iCpipe;
    int iY,iLen;
    char *pcFileTmp;
    
    /* Open listeining socket */
    bzero(&sin, sizeof(sin));
    bzero(cBuf, sizeof(cBuf));

    sin.sin_family 	= PF_INET;
    sin.sin_port 	= htons(0);
    sin.sin_addr.s_addr = INADDR_ANY;
    
    iSock = socket(PF_INET, SOCK_STREAM, 0);
    if (iSock<=0)
	return -1;
    set_socket_keepalive( iSock );

    if (bind(iSock, (struct sockaddr *)&sin, sizeof(sin))<0)
	goto endit;
    
    bzero(&sin, sizeof(sin));
    iLen = sizeof(sin);
    if (getsockname(iSock, (struct sockaddr *)&sin, &iLen)<0)
	goto endit;

    printf("[?]-[CONFIGURE] Binded to port %d \n", ntohs(sin.sin_port));
    listen(iSock,1);
    timeout.tv_usec = 0;
    timeout.tv_sec  = 30; // 30sec should be enough
    FD_ZERO(&fds);
    FD_SET(iSock, &fds);
    iMaxfds = getdtablesize();

    if (!myClient->stat)
	goto endit;

    pcFileTmp = rindex(pcFile,'/') + 1;
    bio_printf(myClient->io,"+CONFIGURE %d %lu %s\n", ntohs(sin.sin_port), sum(pcFile), pcFileTmp);

    if ( (iSelectRet = select(iMaxfds, &fds, NULL, NULL, &timeout))<0 )
	goto endit;
	
    if (!iSelectRet) 
	goto endit;

    if (FD_ISSET(iSock, &fds)) {
	    if ( ( iCpipe = accept(iSock, (struct sockaddr *)&csin, &iLen))<0) 
		goto endit;
	
	    set_socket_keepalive( iCpipe );
	
	    bzero(cBuf, sizeof(cBuf));
	    strncpy(cBuf, inet_ntoa(csin.sin_addr), sizeof(cBuf) - 1);
	    if (!strcmp(cBuf, myClient->host))
		    printf("[CONFIGURE-%s]: Accepted connection from : %s \n", myClient->host ,cBuf);
	    else  {
		    printf("[CONFIGURE-%s]: Access denied - port hijacker! %s \n", myClient->host, cBuf);
		    close(iCpipe);
		    goto endit;
	    }
	    fPackage = fopen(pcFile, "r");
	    if (fPackage) {
		bzero(cBuf, sizeof(cBuf));
		while (fgets(cBuf, sizeof(cBuf) - 1, fPackage)) {
		    iLen = strlen(cBuf);
		    for (iY=0; iY < iLen; iY++)
			    cBuf[iY]=cBuf[iY]^0x66^0x55^0x20;
		    write(iCpipe, cBuf, iLen);
		    bzero(cBuf, sizeof(cBuf));
		}
		printf("[CONFIGURE-%s]: End of transaction \n", myClient->host);
		fclose(fPackage);
		close(iCpipe);
	    }
    } 

endit:
    close(iSock);
    return 0;

}




/* 
    Pack and send data via bianry channel
*/


static int
sendit_binary(struct client_struct *myClient, char *pcFile)
{
    char cBuf[1024];
    struct sockaddr_in sin,csin;
    FILE *fPackage;
    int iSock;
    int iLen;
    fd_set fds;
    struct timeval timeout;
    int iSelectRet;
    int iMaxfds;
    int iCpipe;
    int iCount, iSize, iC;

    /* Open listeining socket */
    bzero(&sin, sizeof(sin));
    bzero(cBuf, sizeof(cBuf));

    sin.sin_family 	= PF_INET;
    sin.sin_port 	= htons(0);
    sin.sin_addr.s_addr = INADDR_ANY;

    fPackage = fopen(pcFile, "r");
    if (!fPackage)
	return -1;
    
    iSock = socket(PF_INET, SOCK_STREAM, 0);
    if (iSock<=0) {
	fclose(fPackage);
	return -1;
    }
    set_socket_keepalive( iSock );
	
    
    if (bind(iSock, (struct sockaddr *)&sin, sizeof(sin))<0)
	goto endit;
    
    bzero(&sin, sizeof(sin));
    iLen = sizeof(sin);
    if (getsockname(iSock, (struct sockaddr *)&sin, &iLen)<0)
	goto endit;

    printf("[?]-[UPDATE-%s] Binded to port %d \n",myClient->host ,ntohs(sin.sin_port));
    listen(iSock,1);
    timeout.tv_usec = 0;
    timeout.tv_sec  = 30; // 30sec should be enough
    FD_ZERO(&fds);
    FD_SET(iSock, &fds);
    iMaxfds = getdtablesize();

	
    fseek(fPackage, 0x0, SEEK_END);
    iSize = ftell(fPackage);
    fseek(fPackage, 0x0, SEEK_SET);

    if (!myClient->stat)
	goto endit;

    bio_printf(myClient->io,"+UPDATE %d %lu %d\n", ntohs(sin.sin_port), sum(pcFile), iSize);


    if ( (iSelectRet = select(iMaxfds, &fds, NULL, NULL, &timeout))<0 )
	goto endit;
	
    if (!iSelectRet) 
	goto endit;

    if (FD_ISSET(iSock, &fds)) {
	    if ( ( iCpipe = accept(iSock, (struct sockaddr *)&csin, &iLen))<0) 
		goto endit;

            set_socket_keepalive( iCpipe );

	    bzero(cBuf, sizeof(cBuf));
	    strncpy(cBuf, inet_ntoa(csin.sin_addr), sizeof(cBuf) - 1);
	    if (!strcmp(cBuf, myClient->host))
		    printf("[UPDATE-%s]: Accepted connection from : %s \n", myClient->host, cBuf);
	    else  {
		    printf("[UPDATE-%s]: Access denied - port hijacker! %s \n", myClient->host, cBuf);
		    close(iCpipe);
		    goto endit;
	    }
	    if (fPackage) {
		bzero(cBuf, sizeof(cBuf));
	

		iCount = iC = 0;
		while ( iCount < iSize ) {
		    iC = fread(cBuf, 1, sizeof(cBuf) - 1, fPackage);
		    iLen = write(iCpipe, cBuf, iC);
		    if (iLen==-1) break;
		    bzero(cBuf, sizeof(cBuf));
		    iCount += iC;
		}	

	
		printf("[UPDATE-%s]: End of transaction \n", myClient->host);
		close(iCpipe);
	    }
    } 

endit:
    fclose(fPackage);
    close(iSock);
    return 0;

}




void *
ConfigureHost(void *bio)
{
    struct client_struct *myClient = (struct client_struct *)bio;
    char cPath[255];
    
    /*
      Ok now we should have package lying there somewhere 
      Now we will transfer i to a client	
    */

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s/package.sh", CONFIGS_DIR, myClient->host, myClient->name);
    
    if (file_exist(cPath)<0)
	return NULL;

    /* Open port and send file */
    sendit(myClient, cPath);
    
    return NULL;
    
}





void *
ConfigureHostClients(void *bio)
{
    struct client_struct *myClient = (struct client_struct *)bio;
    char cPath[255];
    
    /*
      Ok now we should have package lying there somewhere 
      Now we will transfer i to a client	
    */
    

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s/clients.sh", CONFIGS_DIR, myClient->host, myClient->name);

    if (file_exist(cPath)<0)
	return NULL;


    /* Open port and send file */
    sendit(myClient, cPath);
    
    return NULL;
    
}






void *
UpdateHost(void *bio)
{
    struct client_struct *myClient = (struct client_struct *)bio;
    char cPath[255];
    char cBuf[1024];
    FILE *fIn;
    gzFile file;
    int iSize = 0, iC = 0, iCount = 0;
    int iErr;
    char *ptr;
    char temp[20];

    char cVersion[20];
    int iMajor;
    /* Sprawdzanie architektury */
    bzero(cVersion, sizeof(cVersion));
    strncpy(cVersion, myClient->cversion, sizeof(cVersion)-1);
    ptr = strtok(cVersion,".");
    if (ptr)
	iMajor = atoi(ptr);
    else 
	iMajor = 0;
    printf("[?] (%s-%s) iMajor: %d \n", globalClient->name, globalClient->host, iMajor);

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%d/APclientd", APCLIENT_DIR, iMajor);
    
    /* First pack the file */
    if ( (fIn = fopen(cPath, "rb"))==NULL )
	return NULL;
    bzero(temp, sizeof(temp));
    strcpy(temp,"APclientd.XXXXXX");
    ptr = mktemp(temp);
    if (!ptr)	
	return NULL;
	
    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%s", APCLIENT_DIR, ptr);

    file = gzopen(cPath, "wb");
    if (file == NULL) {
	fclose(fIn);
	return NULL;
    }
    
    bzero(cBuf, sizeof(cBuf));
	fseek(fIn, 0x0, SEEK_END);
        iSize = ftell(fIn);
	fseek(fIn, 0x0, SEEK_SET);
        
    while ( iCount < iSize ) {
        iC = fread(cBuf, 1, sizeof(cBuf) - 1, fIn);
	if ( gzwrite(file, cBuf, iC) != iC ) {
	    printf("gzputs err: %s\n", gzerror(file, &iErr));
	    break;
	}
	iCount += iC;
    }
    gzclose(file);
    fclose(fIn);
    
    /* Open port and send file */
    sendit_binary(myClient, cPath);
    unlink(cPath);
    
    return NULL;
    
}




static int 
cmd_configme(char **pcParam, int iParamCount)
{
    pthread_t pth;
    char cPath[255];
    unsigned long ulSuma;
    
    if (iParamCount<1)
	return -1;

    if (config_make_config(globalClient->host, globalClient->name)<0) {
	printf("[!][%s-%s] Make config failed! \n", globalClient->host, globalClient->name);
	return -1;
    }

    if (config_make_additional(globalClient->host, "package.sh", "baseaddons", globalClient->name)<0) 
	printf("[!][%s-%s] Make-additonal config-base failed! Skipping this\n", globalClient->host, globalClient->name);
	    
    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s/package.sh", CONFIGS_DIR, globalClient->host, globalClient->name);
    ulSuma = sum(cPath);
    
    if (ulSuma==0)
	return -1;
	
    if (ulSuma==atol(*pcParam)) {
	printf("[! CONFIGME][%s-%s] Configuration is  Up2DATE - skipping..!\n", globalClient->host, globalClient->name);
	reply(globalClient->io, "=Your Base-Configuration is UP2DTAE, NOT SENDING UPDATE COMMANDS\n");
        return -1;
    }


    reply(globalClient->io, "= OK, MAKING CONFIGURATION PACKAGE FOR YOU\n");
    if (pthread_create(&pth, NULL, ConfigureHost, (void *) globalClient)!=0) {
	perror("[PTHREAD] ");
    }
    return 0;
}



static int 
cmd_configclients(char **pcParam, int iParamCount)
{
    pthread_t pth;
    char cPath[255];
    unsigned long ulSuma;
    
    if (iParamCount<1)
	return -1;

	
    if (config_make_ethers(globalClient->host, globalClient->name)<0) {
	printf("[CONFIGURE-%s-%s]: error making ethers \n", globalClient->host, globalClient->name);
	return -1;
    }

    if (config_make_clientsconfig(globalClient->host, globalClient->name)<0) {
	printf("[CONFIGURE-%s-%s]: error making package\n", globalClient->host, globalClient->name);
	return -1;
    }

    if (config_make_additional(globalClient->host, "clients.sh", "clientsaddons", globalClient->name)<0) 
	printf("[!][%s-%s] Make-additonal config-base failed! Skipping this\n", globalClient->host, globalClient->name);
    
	    
    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%s-%s/clients.sh", CONFIGS_DIR, globalClient->host, globalClient->name);
    ulSuma = sum(cPath);
    
    if (ulSuma==0)
	return -1;
	
    if (ulSuma==atol(*pcParam)) {
	printf("[! CONFIGCLIENTS][%s-%s] Configuration is  Up2DATE - skipping..!\n", globalClient->host, globalClient->name);
	reply(globalClient->io, "=Your CONFIGCLIENTS-Configuration is UP2DTAE, NOT SENDING UPDATE COMMANDS\n");
        return -1;
    }

    reply(globalClient->io, "= OK, MAKING CONFIGCLIENTS PACKAGE FOR YOU\n");
    if (pthread_create(&pth, NULL, ConfigureHostClients, (void *) globalClient)!=0) {
	perror("[PTHREAD] ");
    }
    return 0;
}





static int 
cmd_updateme(char **pcParam, int iParamCount)
{
    pthread_t pth;
    char cPath[255];
    char cVersion[20];
    char *ptr;
    int iMajor;
    unsigned long ulSum;    
    if (iParamCount<1)
	return -1;

    /* Sprawdzanie architektury */
    bzero(cVersion, sizeof(cVersion));
    strncpy(cVersion, globalClient->cversion, sizeof(cVersion)-1);
    ptr = strtok(cVersion,".");
    if (ptr)
	iMajor = atoi(ptr);
    else 
	iMajor = 0;
    printf("[?] (%s-%s) iMajor: %d \n", globalClient->name, globalClient->host, iMajor);

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%d/APclientd", APCLIENT_DIR, iMajor);
    ulSum = sum(cPath);
    if (ulSum == 0)
	return -1;
	
    if (ulSum == atol(*pcParam)) {
	printf("[!][%s-%s] HOST IS UP TO DATE, NOT SENDING UPDATE COMMANDS!\n", globalClient->host, globalClient->name);
        if (globalClient->stat)
		bio_printf(globalClient->io, "=Your software is UP2DTAE, NOT SENDING UPDATE COMMANDS\n");
        return -1;
    }
    if (sum(cPath)==0)
	return -1;
    reply(globalClient->io, "= OK, MAKING UPDATE-PACKAGE FOR YOU\n");
    if (pthread_create(&pth, NULL, UpdateHost, (void *) globalClient)!=0) {
	perror("[PTHREAD] ");
    }
    return 0;
}








/*
    Protocol Commands
*/

static struct protoCMD
{
        char command[20];
	int (*function)(char **, int);
} protoCMDs[] =
{
      { "+PONG",             cmd_void,           },
      { "+SETNAME",          cmd_setname,        },
      { "+STATS", 	     cmd_stats,          },
      { "+CONFIGME",   	     cmd_configme,  	 },
      { "+CONFIGCLIENTS",    cmd_configclients,  },
      { "+UPDATEME",         cmd_updateme,       },

};






int do_cmd(char *pcBuffor, struct client_struct *gClient)
{
        int  iAllCmds = (sizeof(protoCMDs) / sizeof(struct protoCMD));
        int  iX = 0;
        char **ap, *pcParams[10];
	int	    iParams=0;
    
          chop(pcBuffor);	
       	  if (strlen(pcBuffor)<1)
	    return 0;
	    
          globalClient = gClient;

          if (!globalClient)
	     return -1;

	  for (ap = pcParams; (*ap = strsep(&pcBuffor, " \t")) != NULL;iX++)
    	       if (**ap != '\0')
        	   if (++ap >= &pcParams[10])
            	    break;
        
		iParams=iX;
	        for (iX=0; iX<iAllCmds; iX++) {
			if (!strcmp(protoCMDs[iX].command, pcParams[0])) {
			    return protoCMDs[iX].function(pcParams+1, iParams-1);
			}
	        }
    
//	        reply("=[-] Unknown command \n");
    
        return 0;
}
