/*
    Client
    
    No additional info availble for now
    
    - Added PONG check - if no PING/PONG mechanism vivible for PINGPONGTIMEOUT
    then we assume we lost connection to MAIN SERVER

    Komunix (c) - Pawel Furtak '2005 <pawelf@bioinfo.pl>
        
*/
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>

// SSL stuff
#include "common.h"
#include "config.h"
#include "client.h"
#include "log.h"
#include "server.h"
#include "tevents.h"
#include "cprotocol.h"
#include "release.h"
#include "version.h"

#include "mywatch.h"

#define ERROR_TIMEOUT 2
/* 
    NETWORK_TIMEOUT - czas po ktorym uznajemy ,ze MAIN SERVER padl i zamykamy
    polaczenie, jest to czas ktory uplynal od momentu otrzymania jakiejkolwiek
    komendy od serwera - zakladamy zawsze ,ze ten czas jest znaczenie
    wiekszy od czasow ustalonych w PING/PONG TIMEOUT
    
    !!! tutaj czas podany jest w minutach!!!
*/



    FILE *Flogfile = NULL;
    extern int errno;
    int LOGLEVEL=0;
    int DAEMON=1;		// Daemon mode - yes or no
    char cMainHost[255];
    int 	iMyStatus = 0; // 0 - not connected, 1 - conneted,without SSL, 2 - fully SSL-connected
    int 	iSock;

/*
static int
mprintf(const char *spcFormat, ...)
{
            static char scBuf[1024];
	    va_list ap;
            memset(scBuf, 0x0, sizeof(scBuf));
            va_start(ap, spcFormat);
            vsnprintf(scBuf, sizeof(scBuf) - 1, spcFormat, ap);
            va_end(ap);
        
            return (printf("%s", scBuf));
} 
*/
void 
breakup(int signo)
{
	mprintf("[SIGNAL]: Got signal %d \n", signo);
	switch (signo) {
	    case SIGINT: 
			exit(0);
			break;
	}	
}


static SSL * 
server_openssl_channel(int cSock, SSL_CTX *ctx)
{
	    BIO 	*sbio = NULL;
	    SSL 	*ssl  = NULL;

	    ssl = SSL_new(ctx);
	    if (!ssl) {
		    mprintf("[!] Critical error could not create SSL object \n");
		    return NULL;
	    }
	
	    sbio = BIO_new_socket(cSock, BIO_NOCLOSE);
	    if (!sbio) {
		mprintf("[!] Critical error - could not create BIO object \n");
		SSL_free(ssl);
		return NULL;
	    }
	
	    SSL_set_bio(ssl, sbio, sbio);
	    if (SSL_connect(ssl)<=0) {
		mprintf("[!] Critical error - Could not create SSL channel-connection\n");
		BIO_free(sbio);
		SSL_free(ssl);
		return NULL;
	    }
	
		return ssl;
}

static void
server_closessl_channel(SSL *ssl) {
	    int iRet;
	
	    iRet = SSL_shutdown(ssl);
	    switch (iRet) {
		case 1: break; // success
		case 0:
		case -1:
	    default:
		mprintf("[!] SSL_shutdown() failed \n");
		break;
	    }
	
	    SSL_free(ssl);
}




static void 
putit(char *pcSrc, char *pcDst, int iSize) 
{
	    bzero(pcSrc, iSize);
    	    strncpy(pcSrc, pcDst, iSize);
}


static int
prepare_config_dir() 
{
	    struct stat st;
	    if (stat(CONFIG_DIR, &st)<0) {
	    if (mkdir(CONFIG_DIR, 0666)<0) {
			mprintf("[!] Error creating config dir: %s \n", strerror(errno));
			return -1;
		}
		mprintf("[:)] Created %s \n", CONFIG_DIR);
		return 0;
	    } else if (!S_ISDIR(st.st_mode)) {
		printf("[!!] %s exist but its not a DIRECTORY! \n", CONFIG_DIR);
		return -1;
	    }    

	    mprintf("[!] Using %s as my config directory \n", CONFIG_DIR);
	    return 0;
	
}


static void 
banner() 
{
	printf("[?] v.%d.%d.%d\n", MAJOR_VRSN, MINOR_VRSN, RELEASE);
	printf("[?] (c) KOMUNIX, %s\n", RELEASE_DATE);
	printf("[?] Author: Pawel Furtak <pawelf@bioinfo.pl>\n");
}



static void 
usage(char *p) 
{
	banner();
        printf("Usage: %s -a [ApName] -m [MainHost:Port] -d [loglevel]\nLOGLEVEL=> 0-nothing/1-stdout/2-logfile if availle\n", p);
        exit(0);
}


int main(int argc, char *argv[]) 
{
	int 	iProcessCommand = 0;
        int 	iRet, iRet2, iSelectRet;
	int 	iMaxFds;
	char 	cNetBuf[2048];
	struct  timeval timeout;
	char 	cPath[255];
	
	int	iCzas;
	time_t  myTime;
	fd_set  fds;
	SSL	*ssl  	 = NULL;
	SSL_CTX *ctx  	 = NULL;
	BIO	*ssl_bio, *bio = NULL;

	char cApName[255];
	int iMainHostPort;
	int iCh = 0;
	char *pcProg = argv[0];
	char *pcPtr = NULL;
	
	
	    
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, breakup);
//	signal(SIGABRT, breakup);
//	signal(SIGUSR1, breakup);
//	signal(SIGUSR2, breakup);
//	signal(SIGINT, breakup);


	/* Start MYwatch */
/*
	if (!fork()) {
	    mywatch("eth1");
        }
	if (!fork()) {
	    mywatch("vlan1");
        }
        if (!fork()) {
	    mywatch("vlan0");
        }
*/
/*        if (!fork()) {
	    mywatch("eth1");
//	    mywatch("eth0");
	    exit(0);
        }
*/
/*        if (!fork()) {
	    mywatch("vlan0");
	    exit(0);
        }
*/
	/* Default settings */
		
	putit(cApName, APNAME, sizeof(cApName)-1);
	putit(cMainHost, MAIN_SERVER_HOST, sizeof(cMainHost)-1);
	iMainHostPort = MAIN_SERVER_PORT;

	/* Parse options */	

	while ((iCh = getopt(argc, argv, "a:m:d:h")) != -1) {
		switch (iCh) {
		case 'a':
			if (strlen(optarg)>2)
			    putit(cApName, optarg, sizeof(cApName)-1);
			break;
			
		case 'm':
			if (strchr(optarg,':')) {
			    pcPtr = strtok(optarg,":");
			    if (strlen(pcPtr)>5)
				putit(cMainHost, pcPtr, sizeof(cMainHost)-1);
			    pcPtr = strtok(NULL, ":");
			    if ( (strlen(pcPtr)>2) ) {
				for (iRet = 0; iRet<strlen(pcPtr); iRet++)
				    if (!isdigit((int)pcPtr[iRet])) 
					break;
    				
				    if (iRet==strlen(pcPtr))
					    iMainHostPort = atoi(pcPtr);
			    }
			} else {
			    if (strlen(optarg)>5)
				    putit(cMainHost, optarg, sizeof(cMainHost)-1);
			}
			break;

		case 'd':
			if (strlen(optarg)>0)
			LOGLEVEL=atoi(optarg);
			break;

			
		case 'h':
			usage(pcProg);
			break;	/* NEVER REACHED */
		}
	} // while

        if (LOGLEVEL>0) 
		DAEMON=0;

	mprintf("[APNAME: %s ][MAINHOST: %s ][MAINPORT: %d ][LOGLEVEL: %d]\n",cApName, cMainHost, iMainHostPort, LOGLEVEL);
	argc -= optind;
	argv += optind;
	
        mprintf("[?] Starting client server - sweeet \n");
	prepare_config_dir();
	
	mprintf("[?] Initializing SSL engine...\n");
	ctx = initialize_ctx(KEYFILE, PASSWORD);
	if (!ctx)
	    return -1;
	
	
        if (! (Flogfile = open_log_file(LOGFILE)) ) {	
		mprintf("[!] Error opening logfile - debug LOG wont be availble \n");
        }

        mprintf("[?] Opening configuration file \n");
        if ( (iRet = config_load(CONFIG_LOCAL)) < 0 ) {	
		mprintf("[!] Error opening LOCAL config file [%s] - skipping \n", CONFIG_LOCAL);

        } else {
		mprintf("[!] Config file loaded successfully\n");

        }

	
	
	/* Check certs */
	// check_certs()
	
	if (DAEMON)
	    if (daemon(1,1)<0) {
		perror("[!] Daemon stopping -> ");
		return -1;
	    }

while ( 1 ) 
{
	    iProcessCommand = 0;
	    /* Retryings */
	    
	    switch (iMyStatus) {
		case 0: 
			/* Not connected, trying again */
		
			mprintf("[?] Trying to connect to MAINSERVER again \n");
			if ( (iSock = server_open_connection(cMainHost, 
									iMainHostPort))<0) {
			    mprintf("[!] Connecting to MAINSERVER failed :%s \n", 
									strerror(errno));
			sleep(CONNECT_RETRY);
			} else {
			
			    mprintf("[!] Connected to MAINSERVER %s[%d](iSock: %d)\n", 
						    cMainHost, iMainHostPort, iSock);
			    iMyStatus = 1; 
			}
			
			continue;
		case 1: 
			/* Connected,but without SSL accept yet */

		        mprintf("[?] Trying to open SSL channel again \n");
			
			if ( (ssl = server_openssl_channel(iSock, ctx)) ) {
			    mprintf("[!] SSL Accepted - channel opened \n");
			
			    bio     = BIO_new(BIO_f_buffer());
			    ssl_bio = BIO_new(BIO_f_ssl());
			    BIO_set_ssl(ssl_bio, ssl, BIO_CLOSE);
			    BIO_push(bio, ssl_bio);
			    
//			    bio_printf(bio, "[%s] Melduje sie \n", cApName);
			    bio_printf(bio, "+SETNAME %s %d.%d.%d\n", cApName, MAJOR_VRSN, MINOR_VRSN, RELEASE);
			    bzero(cPath, sizeof(cPath)); snprintf(cPath, sizeof(cPath) - 1, "%s/package.sh", CONFIG_DIR);
			    bio_printf(bio, "+CONFIGME %lu\n", sum(cPath));
			    bzero(cPath, sizeof(cPath)); snprintf(cPath, sizeof(cPath) - 1, "%s/clients.sh", CONFIG_DIR);
			    bio_printf(bio, "+CONFIGCLIENTS %lu\n", sum(cPath));
			    bio_printf(bio, "+UPDATEME %lu\n", sum("APclientd"));
			    BIO_flush(bio);	
		    	    iMyStatus = 2;
			    myTime = time(NULL);	

			    mprintf("[?] Downloading latest config...\n");
			    // config_download()

			} else {
			    mprintf("[!] Failed to open SSL channel again - Shutting down connection\n");
			    shutdown(iSock, 2);
			    close(iSock);
			    iMyStatus = 0;
			    sleep(CONNECT_SSL_RETRY);
	
			}
			
			continue;
	    }
	    
	    
	    /* Fully Connected via SSL */
	    FD_ZERO(&fds);
	    FD_SET(iSock, &fds);
	    
	    iMaxFds = getdtablesize();
	    timeout.tv_usec = 0;
	    timeout.tv_sec = SELECT_TIMEOUT;
	    if ( (iSelectRet = select(iMaxFds, &fds, NULL, NULL, &timeout))<0 ) {
		    mprintf("[!] Error select(): %s \n", strerror(errno));
		    continue;
	    }    
	    
	    memset(cNetBuf, 0x0, sizeof(cNetBuf));

	    if (iSelectRet)
	    if (FD_ISSET(iSock, &fds)) {
		iRet = iRet2 = 0;
		iRet = BIO_gets(bio, cNetBuf, sizeof(cNetBuf) - 1);
		// BIO_should_retry()
		
//		mprintf("[!] iRet2 %d [iRet: %d] \n", iRet2, iRet);
				
		switch ( (iRet2=SSL_get_error(ssl, iRet)) ) {
		    case SSL_ERROR_NONE:
					iProcessCommand = 1;
					myTime = time(NULL);	
					break;
		    /* Zamykanie polczenia */
		    case SSL_ERROR_SYSCALL:
		    default:
					mprintf("[!] Unknown error: %d [iRet: %d] \n", iRet2, iRet);
					mprintf("[!] Ret: %d [%s]\n", iRet2, strerror(errno));
					server_closessl_channel(ssl);
					BIO_free(bio);
					close(iSock);
					iMyStatus = 0;
					sleep(ERROR_TIMEOUT);
					break;

		} 
	    }
	    
	    // 0 - selectTIMEOUT // 1 - selectEVENT
	    if (!iSelectRet) {
		iCzas = (int)difftime(time(NULL), myTime);	
		
		if (iCzas/60>= NETWORK_TIMEOUT) {
		printf("[!] Netowrk timeout reached - closing connection \n");
					server_closessl_channel(ssl);
					BIO_free(bio);
					close(iSock);
					iMyStatus = 0;
					sleep(ERROR_TIMEOUT);
					continue;
			
		}
	    }
	    
	    
	    /* Processing PROTOcol commands */
	    
	    if (iProcessCommand) {
//	    	mprintf("[!] Got from server(processing): (%d)-> %s", 
//						    strlen(cNetBuf), cNetBuf);

		if (cNetBuf[0]=='+') {
			mprintf("[COMMAND]: %s", cNetBuf);
		        mprintf("[COMMAND-DO_CMD]: %d \n", do_cmd(cNetBuf, bio));
	        } else if (cNetBuf[0]=='=') {
		    mprintf("[REPLY]: %s", cNetBuf);
		    // reply
		} else {
		    mprintf("[GARBAGE]: %s", cNetBuf);
		
		} // garbage
	    }
	
	    
	    
	    /* TIME EVENTS */
		    
	    TimeEvents(bio, cApName);

} // while( 1 )

	destroy_ctx(ctx);
	close(iSock);
	/* After downloading and settip up everything we'r going into LOOP */
	

        return 0;
}

