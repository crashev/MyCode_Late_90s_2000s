/*

    Plik zawierajacy SPECYFIKACJE protkolu, wszelkie komendy protokolu
    rozpoznawane, obslugiwane i dodawane sa tutaj.
    Komunix (c) - Pawel Furtak '2005 <pawelf@bioinfo.pl>
    // Client    
*/

#define LINUX_INTEL 0x3
#define LINUX_MIPS  0x8
#define SYSTEM LINUX_MIPS


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <time.h>       
#include <zlib.h>
#include <pthread.h>

#include "log.h"
#include "config.h"
#include "server.h"

BIO *globalBIO = NULL;
extern char cMainHost[255];
extern    int 	iMyStatus; // 0 - not connected, 1 - conneted,without SSL, 2 - fully SSL-connected
extern    int   iSock;

static int 
reply(const char *spcFormat, ...)
{
    	static char scBuf[1024];
	va_list ap;
        memset(scBuf, 0x0, sizeof(scBuf));
        va_start(ap, spcFormat);
        vsnprintf(scBuf, sizeof(scBuf) - 1, spcFormat, ap);
        va_end(ap);

	if (bio_printf(globalBIO, "%s", scBuf))
		return (BIO_flush(globalBIO));
	else
		return -1;
//	return (bio_printf(BIO *bio, const char *spcFmt, ...));
	
}

/*
    Funkcja sprawdzajaca czy binarka jest przznaczona na dany procesor!
*/


int 
checkCPU(char *cPath) 
{
        int x;
        unsigned char b;

        if ( (x = open(cPath, O_RDONLY))<0 ) 
                return -1;
        
        lseek(x, 0x12, SEEK_SET);
        read(x, &b, 1);
	close(x);
        return (int)b;
}



static int 
cmd_void(char **pcParam, int iParamCount)
{
	    
	return 0;
}


static int 
cmd_pong(char **pcParam, int iParamCount)
{
	    
	reply("+PONG\n");
	return 0;
}


static int 
cmd_settime(char **pcParam, int iParamCount)
{
	// +SETTIME    
        struct timeval tv;
	struct timezone tz;
	if ((iParamCount<1)||(!*pcParam))
	    return -1;
	    	
	gettimeofday(&tv, &tz);

	if  ( ((tv.tv_sec-atoi(*pcParam))>2)||((atoi(*pcParam)-tv.tv_sec)>2) ) {
	        printf("[?] Setting time to %d \n", atoi(*pcParam));
		tv.tv_sec=atoi(*pcParam);
		return (settimeofday(&tv, &tz));
	}
	return 0;
	
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
    XORed download
*/

static int
download(int iPort, char *pcPath)
{
    struct sockaddr_in sin;
    int iSock;
    struct hostent *hp = NULL;
    char cBuf[1024];
    int iFile;
    int iLen, iY;
    
    iSock = socket(PF_INET, SOCK_STREAM, 0);
    if (iSock<0) 
        return -1;

    sin.sin_family = PF_INET;
    sin.sin_port   = htons(iPort);
	mprintf("[CONFIGURE]: Connecting to: %s:%d \n", cMainHost, iPort);
    
    hp = gethostbyname2(cMainHost, PF_INET);
    if (hp)
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    else {
	        close(iSock);
		mprintf("[CONFIGURE]: Error resolving host \n");
		return -1;
	}

    if (connect(iSock, (struct sockaddr *)&sin, sizeof(sin))<0) {
		close(iSock);
		mprintf("[CONFIGURE]: Error connecting to host: %s \n", strerror(errno));
		return -1;
    }


    iFile = open(pcPath, O_WRONLY|O_CREAT|O_TRUNC, 0777);
    if (iFile<0) {
	mprintf("[CONFIGURE]: Was not able to create %s -> %s \n", pcPath, strerror(errno));
	close(iSock);
	return -1;
    }
    
    bzero(cBuf, sizeof(cBuf));
    while ( (iLen = read(iSock, cBuf, sizeof(cBuf) - 1 )) ) {
    	for (iY=0; iY < iLen; iY++)
	    cBuf[iY]=cBuf[iY]^0x66^0x55^0x20;
	write(iFile, cBuf, strlen(cBuf));
	bzero(cBuf, sizeof(cBuf));
	
    }

    close(iFile);
    close(iSock);
    return 0;
}




static int
download_binary(int iPort, char *pcPath, int iSize)
{
    struct sockaddr_in sin;
    int iSock;
    struct hostent *hp = NULL;
    char cBuf[1024];
    int iFile;
    int iC=0, iCount=0;

    iSock = socket(PF_INET, SOCK_STREAM, 0);
    if (iSock<0) 
        return -1;

    sin.sin_family = PF_INET;
    sin.sin_port   = htons(iPort);
	mprintf("[DOWNLOAD-BINARY]: Connecting to: %s:%d \n", cMainHost, iPort);
    
    hp = gethostbyname2(cMainHost, PF_INET);
    if (hp)
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    else {
	        close(iSock);
		mprintf("[DOWNLOAD-BINARY]: Error resolving host \n");
		return -1;
	}

    if (connect(iSock, (struct sockaddr *)&sin, sizeof(sin))<0) {
		close(iSock);
		mprintf("[DOWNLOAD-BINARY]: Error connecting to host: %s \n", strerror(errno));
		return -1;
    }


    iFile = open(pcPath, O_WRONLY|O_CREAT|O_TRUNC, 0777);
    if (iFile<0) {
	mprintf("[DOWNLOAD-BINARY]: Was not able to create %s -> %s \n", pcPath, strerror(errno));
	close(iSock);
	return -1;
    }

    mprintf("[DOWNLOAD-BINARY]: Downloading %d bytes!\n", iSize);
    
    bzero(cBuf, sizeof(cBuf));
    iC = 1;
    while ( (iCount < iSize) && (iC>0) ) {
	iC = read(iSock, cBuf, sizeof(cBuf) - 1 );
	write(iFile, cBuf, iC);
	bzero(cBuf, sizeof(cBuf));
	iCount += iC;

    }
	mprintf("[DOWNLOAD-BINARY]: Done!\n");
    close(iFile);
    close(iSock);
    return 0;
}



static int 
cmd_configure(char **pcParam, int iParamCount)
{
    char cPath[255];
    unsigned long ulSum;
        
    if (iParamCount<3)
	return -1;

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s/%s", CONFIG_DIR, pcParam[2]);

    /* liczymy sume, jesli sie zgadza to znaczy,ze obecna konfiguracja jest OK */
    ulSum = sum(cPath);

    if ( ulSum == atol(pcParam[1])) {
	mprintf("[! CONFIGURE] Seems that my configuration is UP2DATE...skipping..:)\n");
	reply("=[! CONFIGURE] Seems that my configuration is UP2DATE...skipping..:)\n");
	return 0;
    }

    unlink(cPath);
    /* Downloading configuration */	
    download(atoi(*pcParam), cPath);

    ulSum = sum(cPath);
    if ( ulSum == atol(pcParam[1])) {
	mprintf("[CONFIGURE]: Suma %x == %x \n", ulSum, atol(pcParam[1]));
	mprintf("[CONFIGURE]: Installing new configuration \n");

	system(cPath);
	if (iMyStatus)
	if (bio_printf(globalBIO, "=[!] New configuration installed properly !! :) -> %s\n", pcParam[2]))
        BIO_flush(globalBIO);
    } else {
	mprintf("[CONFIGURE]: Suma %x != %x \n", ulSum, atol(pcParam[1]));
    }
    return 0;
}





static int 
cmd_update(char **pcParam, int iParamCount)
{
    char cPath[255];
    unsigned long ulSum;
    int fOut;
    gzFile file;
    int iSize = 0, iC = 0, iCount = 0;
    int iErr = 0;
    Byte *uncompr;
    uLong uncomprLen = 1024*sizeof(int);

    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
    if (!uncompr) {
	mprintf("[UPDATE]: Error allocating uncompr \n");
	return -1;
    }    
    
    if (iParamCount<3)
	return -1;

    bzero(cPath, sizeof(cPath));snprintf(cPath, sizeof(cPath) - 1, "APclientd");

    ulSum = sum(cPath);
    if ( ulSum == atol(pcParam[1])) {
	mprintf("[UPDATE]: Suma %x == %x, it looks like we'r UP2DATE..skipping... \n", ulSum, atol(pcParam[1]));
	reply("=[!] Seems that My software is UP2DATE...skipping..:)\n");
	return -1;
    }
    bzero(cPath, sizeof(cPath));snprintf(cPath, sizeof(cPath) - 1, "APclientd.tmp");
    unlink(cPath);

    iSize = atoi(pcParam[2]);
    if (iSize<=0)
	return -1;

    /* Downloading configuration */	
    download_binary(atoi(*pcParam), cPath, iSize);
    
    ulSum = sum(cPath);
    if ( ulSum == atol(pcParam[1])) {
	mprintf("[UPDATE]: Suma %x == %x \n", ulSum, atol(pcParam[1]));
	mprintf("[UPDATE]: Installing new APclientd.Unpacking... \n");

        if ( (file = gzopen(cPath, "rb"))==NULL )
		return -1;

        bzero(cPath, sizeof(cPath));
	snprintf(cPath, sizeof(cPath) - 1, "APclientd.tmp2");

        fOut = open(cPath, O_WRONLY|O_CREAT, 0777);
	if (fOut<=0) {
	    gzclose(file);
	    return -1;
        }
/*	
	while (iCount < iSize) {
	    if ( (iC = gzread(file, uncompr, (unsigned)uncomprLen))<0 ) {
    		mprintf("gzread err: %s\n", gzerror(file, &iErr));
		fclose(fOut);
		gzclose(file);
		if (uncompr)
		    free(uncompr);
		return -1;
	    }
*/
	while ( (iC = gzread(file, uncompr, (unsigned)uncomprLen - 1)) ) {
    
	    if (write(fOut, uncompr, iC)<0) {
	    	mprintf("gzread err: %s\n", gzerror(file, &iErr));
		close(fOut);
		gzclose(file);
		if (uncompr)
		    free(uncompr);
		return -1;
	    }
	    iCount += iC;
	    bzero(uncompr, uncomprLen);
	}

	
	
	close(fOut);
	gzclose(file);
	if (uncompr)
	    free(uncompr);
	if (rename("APclientd.tmp2", "APclientd.tmp")<0)
		return -1;
	iC = checkCPU("APclientd.tmp");
	if (iC != SYSTEM) {
	    mprintf("[!] Downloaded binary is not for this OS(ID: 0x%x) \n", iC);
	    reply ("=[!] Downloaded binary is not for this OS(ID: 0x%x) \n", iC);
	    return -1;
    	}

	reply("=[!] UPDATE COMPLETED !!! :)\n");
	shutdown(iSock, 2);
	system("killall -9 APclientd && rm APclientd && mv APclientd.tmp APclientd && /etc/init.d/S46APclientd");
	exit(0);
    } else {
	mprintf("[UPDATE]: Suma %x != %x \n", ulSum, atol(pcParam[1]));
    }
    return 0;
}



static int 
cmd_forceconfigclients(char **pcParam, int iParamCount)
{
    char cPath[255];
    
    bzero(cPath, sizeof(cPath)); snprintf(cPath, sizeof(cPath) - 1, "%s/clients.sh", CONFIG_DIR);
    if (iMyStatus) {
	if (bio_printf(globalBIO, "+CONFIGCLIENTS %lu\n", sum(cPath)))
	    	    BIO_flush(globalBIO);
	 else
	    return -1;
    }
    return 0;

}


static int 
cmd_forceconfigbase(char **pcParam, int iParamCount)
{
    char cPath[255];
    
    bzero(cPath, sizeof(cPath)); snprintf(cPath, sizeof(cPath) - 1, "%s/package.sh", CONFIG_DIR);
    if (iMyStatus)
    {
	if (bio_printf(globalBIO, "+CONFIGME %lu\n", sum(cPath)))
	    	    BIO_flush(globalBIO);
	 else
	    return -1;
    }
    return 0;

}

static int 
cmd_forceupdate(char **pcParam, int iParamCount)
{
    if (iMyStatus)
    {
	if (bio_printf(globalBIO, "+UPDATEME %lu\n", sum("APclientd")))
	    	    BIO_flush(globalBIO);
	 else
	    return -1;
    }
    return 0;
}


static int 
cmd_showclients(char **pcParam, int iParamCount)
{
	FILE *fClients;
	char cTmpBuf[255];

	if ( (fClients = popen("/sbin/wl assoclist", "r"))==NULL)
		return -1;

	reply("=START CLIENTS\n");

	bzero(cTmpBuf, sizeof(cTmpBuf));
	while (fgets(cTmpBuf, sizeof(cTmpBuf) - 1, fClients)) 
	{
		reply("=%s", cTmpBuf);    
		bzero(cTmpBuf, sizeof(cTmpBuf));
	}
	
	pclose(fClients);

	reply("=END CLIENTS\n");

        return 0;
}


static int 
cmd_showconnections(char **pcParam, int iParamCount)
{
	FILE *fConn;
	char cTmpBuf[255];
	if ( (fConn = fopen("/proc/net/ip_conntrack", "r"))==NULL)
		return -1;
	reply("=START CONNECTIONS\n");
	
	bzero(cTmpBuf, sizeof(cTmpBuf));
	while (fgets(cTmpBuf, sizeof(cTmpBuf) - 1, fConn)) 
	{
		reply("=%s", cTmpBuf);    
		bzero(cTmpBuf, sizeof(cTmpBuf));
	}
	fclose(fConn);
	
	reply("=END CONNECTIONS\n");

        return 0;
}



static int 
cmd_catfile(char **pcParam, int iParamCount)
{
	FILE *fConn;
	char cTmpBuf[255];
	
	if (iParamCount<1)
		return -1;
	if ( (fConn = fopen(*pcParam, "r"))==NULL)
		return -1;
	reply("=START CATFILE %s\n", *pcParam);
	
	bzero(cTmpBuf, sizeof(cTmpBuf));
	while (fgets(cTmpBuf, sizeof(cTmpBuf) - 1, fConn)) 
	{
		reply("=%s", cTmpBuf);    
		bzero(cTmpBuf, sizeof(cTmpBuf));
	}
	fclose(fConn);
	
	reply("=END CATFILE %s\n", *pcParam);

        return 0;
}


static int 
cmd_exec(char **pcParam, int iParamCount)
{
	FILE *fClients;
	char cBuf[500];
	char cTmpBuf[255];
	int iX;
	
        bzero(cBuf, sizeof(cBuf));
	for (iX=0; iX< iParamCount; iX++) {
	    if (pcParam[iX]) 
	    {
		strncat(cBuf, pcParam[iX], sizeof(cBuf) - 1);	    
		strncat(cBuf, " ", sizeof(cBuf) - 1);	    
    	    }
	}

	if ( (fClients = popen(cBuf, "r"))==NULL)
		return -1;

	reply("=START EXEC %s\n", cBuf);

	bzero(cTmpBuf, sizeof(cTmpBuf));
	while (fgets(cTmpBuf, sizeof(cTmpBuf) - 1, fClients)) 
	{
		reply("=%s", cTmpBuf);    
		bzero(cTmpBuf, sizeof(cTmpBuf));
	}
	
	pclose(fClients);

	reply("=END EXEC %s\n", cBuf);

        return 0;
}



/*
    Protocol Commands
*/

static struct protoCMD
{
        char command[60];
	int (*function)(char **, int);
} protoCMDs[] =
{
      { "+PING",          	 cmd_pong,      	     },
      { "+SETTIME",       	 cmd_settime,   	     },
      { "+CONFIGURE",     	 cmd_configure, 	     },
      { "+UPDATE",       	 cmd_update, 	  	     },
      { "+FORCECONFIGCLIENTS",   cmd_forceconfigclients,     },
      { "+FORCECONFIGBASE", 	 cmd_forceconfigbase,        },
      { "+FORCEUPDATE",    	 cmd_forceupdate,	     },

      { "+SHOWCLIENTS", 	 cmd_showclients,	     },
      { "+SHOWCONNECTIONS", 	 cmd_showconnections,	     },
      { "+CATFILE", 		 cmd_catfile,		     },
      { "+EXEC",		 cmd_exec,		     },
};






int do_cmd(char *pcBuffor, BIO *bio)
{
        int  iAllCmds = (sizeof(protoCMDs) / sizeof(struct protoCMD));
        int  iX = 0;
        char **ap, *pcParams[10];
	int	    iParams=0;
    
	  globalBIO = bio;
          chop(pcBuffor);	
       	  if (strlen(pcBuffor)<1)
	    return 0;

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
    
	        reply("= [-] Unknown command \n");
    
        return -1;
}

