/*
    SERWER
    (c) Komunix - Pawel Furtak
    
    TODO:
    - synchronizacja czasowa
    - admin port na pare portow
    want_read/want_write
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include <fcntl.h>

#include <errno.h>
#include <time.h>

#include <libgen.h> // basename

#include "common.h"
#include "server.h"
#include "protocol.h"
#include "log.h"
#include "aprotocol.h"
#include "release.h"
#include "version.h"



	extern int errno;

	extern struct client_struct clients[ MAXCLIENTS];
	int iCount = 0; 			// clients counter
	int iSQLConnected = 0; 			// SQL Server Connection status
	FILE *Flogfile = NULL;

	int asock; // gniazda obslugujace polaczenia
        int asockpipe=0;

	int LOGLEVEL = 1; //0 - nothing, 1 - stdout, 2 - file
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
	    case SIGSEGV:
	    case SIGINT: 
			close(asock);
			exit(0);
			break;
	}	
}


static void 
close_conn(int iTmp)
{
        int iRet;
    
        if (clients[iTmp].stat>1) {	
		iRet = SSL_shutdown(clients[iTmp].ssl);
        if (!iRet) {
		shutdown(clients[iTmp].fd,1);
		SSL_shutdown(clients[iTmp].ssl);
        }
        switch (iRet) {
		case 1: break;
		case 0:
		case -1:
		default: mprintf("[%s] Error shutting down SSL channel - Probably peer exited before \n", clients[iTmp].host);
			 break;
	    }
	}    
	
	SSL_free(clients[iTmp].ssl);	
	close(clients[iTmp].fd);
	clients[iTmp].stat = 0;
	bzero(&clients[iTmp], sizeof(clients[iTmp]));    

}



    
static int 
find_free() 
{
        int iCX;
    
        for (iCX=0; iCX<MAXCLIENTS; iCX++) 
		    if (clients[iCX].stat == 0) return iCX;
    
	return -1; // no free availble
}
                



static int
bio_mprintf(BIO *bio, const char *spcFmt, ...)
{
        static char cBuf[2048];
        va_list va;
        int iRet;
    
        memset(cBuf, 0x0, sizeof(cBuf));
        va_start(va, spcFmt);
        vsnprintf(cBuf, sizeof(cBuf) - 1,spcFmt, va);
        va_end(va);
    

        if ( (iRet = BIO_puts(bio, cBuf))<0) {
		    mprintf("[?] BIO_puts() error: %d \n", iRet);
	    	    return -1;
	        }
        if ( (iRet = BIO_flush(bio))<0) {
    		    mprintf("[?] BIO_flush() error: %d \n", iRet);
		    return -1;
		}

    return 0;

}



static int
prepare_configs_dir(char *pcPtr) 
{
	    struct stat st;
	    if (stat(pcPtr, &st)<0) {
	    if (mkdir(pcPtr, 0777)<0) {
			mprintf("[!] Error creating config dir: %s \n", strerror(errno));
			return -1;
		}
		mprintf("[:)] Created %s \n", pcPtr);
		return 0;
	    } else if (!S_ISDIR(st.st_mode)) {
		mprintf("[!!] %s exist but its not a DIRECTORY! \n", pcPtr);
		return -1;
	    }    

	    mprintf("[!] Using %s as my configs directory \n", pcPtr);
	    return 0;
	
}




static void 
banner(char *pcPtr) 
{
	    mprintf("[?] %s v.%d.%d.%d\n", basename(pcPtr), MAJOR_VRSN, MINOR_VRSN, RELEASE);
	    mprintf("[?] (c) KOMUNIX, %s\n", RELEASE_DATE);
	    mprintf("[?] Author: Pawel Furtak <pawelf@bioinfo.pl>\n");

}



static void 
log_host(int iX, const char *spcFormat, ...)
{
            char scBuf[1024];
	    char scPath[255];
	    char cTimebuf[50];
	    
	    FILE *fPlik;
	    va_list ap;
	    time_t t;
	    struct tm *tm;
    
            memset(scBuf, 0x0, sizeof(scBuf));
            va_start(ap, spcFormat);
            vsnprintf(scBuf, sizeof(scBuf) - 1, spcFormat, ap);
            va_end(ap);
	    mprintf(scBuf);
	    // IP-NazwaAp - zawsze beda dwa tworzone dla kazdego AP
	    // IP-NazwaAp i IP-Unknown
	    bzero(scPath, sizeof(scPath));
	    snprintf(scPath, sizeof(scPath) - 1, "%s/%s-%s", LOGS_DIR, clients[iX].host, clients[iX].name);
	    if ( (fPlik=fopen(scPath, "a"))==NULL ) {
		mprintf("[!][%s]-> Error opening LOG_HOST FILE! - %s\n", clients[iX].host, scPath);
		return;
	    }
	    
	    t = time(NULL);
	    bzero(cTimebuf,sizeof(cTimebuf));
	    tm = localtime(&t);
	    strftime(cTimebuf, sizeof(cTimebuf)-1, "%D->%H:%M:%S", tm);

	    /* logujemy */
	    chop(scBuf);
	    fprintf(fPlik,"[%s] = %s \n",cTimebuf, scBuf);
	    fclose(fPlik);
}




static int 
isalreadyconnected(char *pcAddr) 
{
    int iX;
/*
	for (iX = 0; iX< MAXCLIENTS; iX++) 	
	    if (!strcmp(clients[iX].host, pcAddr))
		return 1;
*/		
    return 0;
}





int main(int argc, char *argv[]) 
{
            char cBuf[2048];
	    char cTmpBuf[255];
            fd_set fds;
	    struct sockaddr_in cin;
    	    struct sockaddr_un cun;
            int iLen = sizeof(cin);
	    int lsock, csock;
    	    int iX, iRet, iRet2, iSelectRet;
            int iMaxfds;
	    time_t myTime;
	    int iCzas, iCzasFlood;
	    struct timeval timeout;
	    int iTmp;
	
        // SSL stuff
            BIO 	*sbio = NULL, *ssl_bio = NULL;
	    SSL_CTX *ctx  = NULL;
    	    SSL 	*ssl  = NULL;

	    signal(SIGPIPE, SIG_IGN);
	    signal(SIGHUP, breakup);
//	    signal(SIGABRT, breakup);
//	    signal(SIGUSR1, breakup);
//	    signal(SIGUSR2, breakup);
//	    signal(SIGINT, breakup);
//	    signal(SIGCHLD, breakup);

            for (iX = 0; iX< MAXCLIENTS; iX++) 	
		bzero(&clients[iX], sizeof(clients[iX]));
	    mprintf( "[+] Startin APCenterd! :) %s - %d\n", RELEASE_DATE, RELEASE);
	    banner(argv[0]);
	
        if (! (Flogfile = open_log_file(LOGFILE)) ) {	
		mprintf("[!] Error opening logfile - debug LOG wont be availble \n");
        }

	prepare_configs_dir(CONFIGS_DIR);
	prepare_configs_dir(STATS_DIR);
	prepare_configs_dir(LOGS_DIR);
	
	bzero(cTmpBuf, sizeof(cTmpBuf));
	strcpy(cTmpBuf, ADMIN_SOCKET);
	strtok(cTmpBuf,"/");
        prepare_configs_dir(cTmpBuf);
	
        mprintf( "[+] Initializing SSL engine \n");
        mprintf( "[?] Loading KEYFILE ...\n");
        ctx = initialize_ctx(KEYFILE, PASSWORD);
        mprintf( "[?] Setting DH PARAMS ...\n");
	//    load_dh_params(ctx, DHFILE);
    
        mprintf( "[+] Opening listening socket... \n");
    
        if ( (lsock = open_listen_socket(SERWER_PORT))<0) {
		// handle erro
		mprintf("[!] Blad: %s \n", strerror(errno));
		return -1;
	}
        mprintf("[?] Socket opened on port: %d \n", SERWER_PORT);
	
	unlink(ADMIN_SOCKET);
	if ( (asock = open_admin_socket(ADMIN_SOCKET))<0) {
		// handle erro
		mprintf("[!] Blad: %s \n", strerror(errno));
		close(lsock);
		if (Flogfile) 
		    fclose(Flogfile);
		return -1;
	}
        mprintf("[?] Admin Socket opened path-> %s \n", ADMIN_SOCKET);

    
        
        while (1) 
        {
		FD_ZERO(&fds);
		for (iX = 0; iX< MAXCLIENTS; iX++) 
		    if (clients[iX].stat) FD_SET(clients[iX].fd, &fds);
		    FD_SET(lsock, &fds); // Add incomming port
		    FD_SET(asock, &fds); // Add admin socket
		    if (asockpipe>0) FD_SET(asockpipe, &fds);
		    
		iMaxfds = getdtablesize();
		timeout.tv_usec = 0;
		timeout.tv_sec	= 6;
		if ( (iSelectRet = select(iMaxfds, &fds, NULL, NULL, &timeout))<0 ) {
		    mprintf("[!] SELECT ERROR!! \n");
		    continue;
		}	
    
	/* Obsluga nowego polaczenia - admin socket */
	
	if (FD_ISSET(asock, &fds)) {
		bzero(&cun, sizeof(cun));
		iLen=sizeof(cun);
		mprintf("[ADMIN] Connection to admin socket!\n");
		if (asockpipe==0) {
		if ( (asockpipe = accept(asock, (struct sockaddr *)&cun, &iLen))<0) {
		    mprintf("[ADMIN] Accept error on new connection - admin socket \n");
		    close(asockpipe);
		    asockpipe=0;
		    switch0();
	//	    goto dalej;
		} } else 
		    {
			mprintf("[ADMIN] Port BUSY, admin already connected \n");
			iTmp=accept(asock, (struct sockaddr *)&cun, &iLen);
			write(iTmp, "=BUSY\n", 6);
			close(iTmp);
	//		goto dalej;
    		    }
	    
	}
	
	/* Obsluga nowego polaczenia - klientow*/
	
	if (FD_ISSET(lsock, &fds)) {
		bzero(&cin, sizeof(cin));
		iLen=sizeof(cin);
		mprintf("[??] Polaczenie -\n");
		if ( (csock = accept(lsock, (struct sockaddr *)&cin, &iLen))<0) {
		    mprintf("[!!] Accept error on new connection \n");
		    close(csock);
		    goto dalej;
		} 
		
		mprintf("[??] Polaczenie - %s \n", inet_ntoa(cin.sin_addr));

		if (!access_list(&cin)) {
			mprintf("[%s]-> ACCESS DENIED!! \n", inet_ntoa(cin.sin_addr));
		        close(csock);
			goto dalej;
		}
		if (isalreadyconnected(inet_ntoa(cin.sin_addr))) {
			mprintf("[%s]-> ALREADY CONNECTED!!!! \n", inet_ntoa(cin.sin_addr));
		        close(csock);
			goto dalej;
		}		
		iRet = find_free();
		if (iRet < 0) {
		    mprintf("[!!] No free slots availble at this time! \n");
		    close(csock);
		    goto dalej;
		}
	    
	        clients[iRet].fd = csock;
	        clients[iRet].stat = 1; // active connection, awaiting SSL handshake
	        strncpy(clients[iRet].host, inet_ntoa(cin.sin_addr), 
							    sizeof(clients[iRet].host)-1);
		set_socket_keepalive( clients[iRet].fd );
	    
	        set_socket_nonblock( clients[iRet].fd );
	        sbio = BIO_new_socket(clients[iRet].fd, BIO_NOCLOSE);
	        ssl = SSL_new(ctx);
	        SSL_set_bio(ssl, sbio, sbio);
		clients[iRet].ssl = ssl;
			
		//FD_SET(clients[ret].fd, &fds);
	    
		iCount++;
	}
	
	dalej:    
	
	/* Sprawdzanie zdarzen - admin port */
	if (iSelectRet)
		if ( (asockpipe>0)&&( FD_ISSET(asockpipe, &fds)) ) {
		    bzero(cBuf, sizeof(cBuf));
		    if (read(asockpipe, cBuf, sizeof(cBuf) - 1)<=0) {
			mprintf("[ADMIN]: Connection closed !\n");
			close(asockpipe);
			asockpipe=0;
			switch0();
		    } else {
			do_admincmd(cBuf, asockpipe);
		    }
		}
	
	
        /* Sprawdzanie zdarzen - klienci*/

	if (iSelectRet)
	    for (iX = 0; iX< MAXCLIENTS; iX++)
		if ( (clients[iX].stat>0) && (FD_ISSET(clients[iX].fd, &fds)) )
		    switch (clients[iX].stat) {
			case 1: 
				/* SSL handshake */
				mprintf("[%s] SSL Handshake\n", clients[iX].host);
			
					
				/* Accept SSL connection attached to socket */
				if ( (iRet = SSL_accept(clients[iX].ssl)) <=0 ) {
				    
				    iRet2 = SSL_get_error(clients[iX].ssl, iRet);
			    		    
				    mprintf("[%s] SSL Accept error \n", clients[iX].host);
				    mprintf("[%s] Error Code: ret: %d - ret2: %d \n", clients[iX].host,
											iRet, iRet2);
				    mprintf("[%s] ", clients[iX].host);
				
				    switch (iRet2) {
					case SSL_ERROR_WANT_READ:
//					    mprintf("SSL_ERROR_WANT_READ - blocking write....\n");
					    clients[iX].sstat = 1;
					    usleep(100);
					    break;
					case SSL_ERROR_WANT_WRITE:
					    mprintf("SSL_ERROR_WANT_WRITE - SWEEET... \n");
					    clients[iX].sstat = 2;
					    break;
					case SSL_ERROR_SSL:
					    mprintf("SSL_ERROR_SSL - HANDSHAKE FUCKED - GO AWAY \n");
					    close_conn(iX);
					    break;
					case SSL_ERROR_SYSCALL:
					    mprintf("WTF: %s \n", strerror(errno));
					    close_conn(iX);
					    break;
					default: 
					    mprintf("Other error \n");
					    close_conn(iX);
					    break;
				    }
				    break;
				}
				
				clients[iX].sstat = 0;
				mprintf("[%s] SSL Handshake accepted \n", clients[iX].host);
				
				/* Creating buffered reads/writes */
				
				clients[iX].io   = BIO_new(BIO_f_buffer());
				      ssl_bio   = BIO_new(BIO_f_ssl());
				    BIO_set_ssl(ssl_bio, clients[iX].ssl, BIO_CLOSE);
				    BIO_push(clients[iX].io, ssl_bio);
				     		 			
		    		clients[iX].stat   = 2; // SSL Authenticated
				clients[iX].ppong  = time(NULL);
				clients[iX].ppongt = time(NULL); 
				strcpy(clients[iX].name, "Unknown");	
				strcpy(clients[iX].cversion, "Unknown");	

				if (SETTIME)
					settime(clients[iX].io);
				break;
			case 2:
				    /* Handling events via encryped SSL channel */
				    again:
				    bzero(cBuf, sizeof(cBuf));
				    iRet = BIO_gets(clients[iX].io, cBuf, sizeof(cBuf)-1);
				    switch (SSL_get_error(clients[iX].ssl, iRet)) {
					case SSL_ERROR_NONE: 
					if (SHOULDLOG) 
					    log_host(iX, "[?]-[RECIV][%s](%s) => %s", clients[iX].host, clients[iX].name, cBuf);
					if (cBuf[0]=='+') {
						log_host(iX, "[?]-[COMND][%s] ->DO-CMD_RETURN: %d \n", clients[iX].host, do_cmd(cBuf, &clients[iX]));			
						
					    } else if(cBuf[0]=='=') {
						log_host(iX, "[?]-[REPLY][%s] ->DO-CMD_RETURN: %s \n", clients[iX].host, cBuf);			
						if (asockpipe) {
						    chop(cBuf);
						    do_adminreply(iX, cBuf, asockpipe);
						}					    						
					    } // else probably some garbage
	    
					    	 
					    clients[iX].ppong = time(NULL);
					    break;
					case SSL_ERROR_WANT_READ:
//					    mprintf("SSL_ERROR_WANT_READ - blocking write....\n");
					    usleep(300);
					    goto again;
					    break;

					default: 
					     mprintf("[%s]-> Error reading Connection closed? \n", clients[iX].host);
					     close_conn(iX);
					     break;

				    }
				    
				break;
		    } // switch ()
	
	
	// GOt here after select() timeout
/*
	if ( iSelectRet==0 )
	for (iX=0; iX<MAXCLIENTS; iX++)
	    if (clients[iX].stat == 2) {
		if ( (iRet = BIO_puts(clients[iX].io, "Elo kliencie\n"))<0)
			    mprintf("[?] BIO_puts() error: %d \n", iRet);
		if ( (iRet = BIO_flush(clients[iX].io))<0)
			    mprintf("[?] BIO_flush() error: %d \n", iRet);
	    }
*/	
	/* PING/PONG mechanism to check if there are DEAD clients */    

	for (iX=0; iX<MAXCLIENTS; iX++)
	    if (clients[iX].stat == 2) {

		myTime=time(NULL);
		iCzas      = (int)difftime(myTime, clients[iX].ppong);
		iCzasFlood = (int)difftime(myTime, clients[iX].ppongt);
	
	/* 
	    Wysylamy PING jesli czas od ostatniego kontaktu > PINGTIMEOUT 
	    Trzeba jeszcze zadbac o to by nie floodowac klienta +PING komendami
	    wiec wysylka +PING odbywac sie bedzie co minimum PINGPONGTIMEOUT    
	*/
		
		if (iCzas>PINGTIMEOUT && iCzas<PONGTIMEOUT
	    			    && iCzasFlood>PINGPONGTIMEOUT) {
			mprintf("[?] Time to SEND +PING to %s[%s] \n", 
					    clients[iX].name, clients[iX].host); 
		
			bio_mprintf(clients[iX].io, "+PING\n");
			clients[iX].ppongt = time(NULL);
		}
		if (iCzas>PONGTIMEOUT) {
			mprintf("[!] PINGPONG TIMEOUT -> %s[%s] \n", 
					    clients[iX].name, clients[iX].host); 
		
		        close_conn(iX);
		}

	    }


//        waitpid(-1,NULL,WNOHANG); //    waitpid(-1,NULL,WNOHANG);
//	perror("[WAITPID] ");
	
	
    } // while ( 1 )
    
    return 0;
}
