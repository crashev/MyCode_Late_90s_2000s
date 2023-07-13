#include "common.h"
#include "server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include <sys/time.h>
#include <time.h>       


void 
load_dh_params(ctx,pcFile)
  SSL_CTX *ctx;
  char *pcFile;
{
        DH  *ret = NULL;
        BIO *bio = NULL;

        if ((bio=BIO_new_file(pcFile, "r")) == NULL)
          berr_exit("Couldn't open DH file");

        ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
        BIO_free(bio);
    
        if(SSL_CTX_set_tmp_dh(ctx, ret)<0)
          berr_exit("Couldn't set DH parameters");
}



void 
generate_eph_rsa_key(ctx)
  SSL_CTX *ctx;
{
        RSA *rsa = NULL;

        rsa = RSA_generate_key(512, RSA_F4, NULL, NULL);
    
        if (!SSL_CTX_set_tmp_rsa(ctx,rsa))
          berr_exit("Couldn't set RSA key");

        RSA_free(rsa);
}
    
  
  

void
set_socket_keepalive( int iSock )
{
        int iFlag=1;
        struct linger linger;

        linger.l_onoff  = 0; 
        linger.l_linger = 0;

        setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));
        setsockopt(iSock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
        setsockopt(iSock, SOL_SOCKET, SO_KEEPALIVE, &iFlag, sizeof(iFlag));

}




int
set_socket_nonblock(int iSock)
{

        if (fcntl(iSock, F_SETFL, O_NONBLOCK)<0) {
            printf("[!] Cant setup NONBLOCK mode, skipping... \n");
            return -1;
        }

        return 0;
}




int
open_listen_socket(int iPort)
{
        struct sockaddr_in sin;
        int iRura;

        bzero(&sin, sizeof(sin));

        sin.sin_family      = PF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port        = htons(iPort);

        if ( (iRura = socket(PF_INET, SOCK_STREAM, 0))<0) {
            // my_error()
            return -1;
        }

        /* Set socket in NONBLOCK mode */
        set_socket_nonblock(iRura);

        if ( bind(iRura, (struct sockaddr *)&sin, sizeof(sin))<0) {
            return -1;
        }

        listen(iRura, 10);

        return iRura;
}


int 
access_list(struct sockaddr_in *tu)
{ 
     static char cAddr[255];
     static char cBuf[255];
     FILE *fAccess;
     
     bzero(cAddr, sizeof(cAddr));
     strncpy(cAddr,inet_ntoa(tu->sin_addr),sizeof(cAddr)-1);
     
     if ( (fAccess = fopen(ACCESS_LIST,"r"))==NULL )
        return ACCESS_LIST_DEFAULTPOLICY;

     bzero(cBuf, sizeof(cBuf));

     while (fgets(cBuf, sizeof(cBuf) - 1, fAccess)) {
	 chop(cBuf);
	 if (!strcmp(cBuf, cAddr)) {
	    fclose(fAccess);
	    return 1;	
	 }
         bzero(cBuf, sizeof(cBuf));
     }
     
     fclose(fAccess);     
     return 0;
}



/* 
    Administravia stuff
*/

int 
open_admin_socket(char *pcLocalSock)
{
		struct sockaddr_un sck;
    		int iPip;
		int iFlag=1;
		
		bzero(&sck, sizeof(sck));
		sck.sun_family=AF_UNIX;
		strncpy(sck.sun_path, pcLocalSock, sizeof(sck.sun_path));	

	        iPip = socket(PF_LOCAL, SOCK_STREAM, 0);	
		if (iPip<0) {
		    	perror("[!] socket()-local");
		        return -1;
        	}
		setsockopt(iPip, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));

		if (bind(iPip,(struct sockaddr *)&sck,sizeof(sck))<0) {
		        perror("  - bind()-lokal ");
			return -1;
        	}
		
		listen(iPip,5);
		return iPip;
}

int 
settime(BIO *bio)
{
        struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return (bio_printf(bio, "+SETTIME %d\n", tv.tv_sec));
}
