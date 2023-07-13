/*
    Komunix (c) - Pawel Furtak '2005 <pawelf@bioinfo.pl>
*/
#include <stdio.h>
#include "common.h"
#include <openssl/err.h>


BIO *bio_err=0;
static char *pass;
static int password_cb(char *buf, int num, int rwflag, void *userdata);
static void sigpipe_handle(int x);

/* A simple error and exit routine*/
int 
err_exit(string)
  char *string;
  {
    fprintf(stderr,"%s\n",string);
    exit(0);
  }

/* Print SSL errors and exit*/
void
berr_exit(string)
  char *string;
  {
    BIO_printf(bio_err,"%s\n",string);
    ERR_print_errors(bio_err);
    //exit(0);
  }

/*The password code is not thread safe*/
static int 
password_cb(char *buf,int num,
  int rwflag,void *userdata)
  {
    if(num<strlen(pass)+1)
      return(0);

    strcpy(buf,pass);
    return(strlen(pass));
  }

static void 
sigpipe_handle(int x) {}

SSL_CTX *initialize_ctx(keyfile,password)
  char *keyfile;
  char *password;
  {
    SSL_METHOD *meth;
    SSL_CTX *ctx;
    
    if(!bio_err){
      /* Global system initialization*/
      SSL_library_init();
      SSL_load_error_strings();
      
      /* An error write context */
      bio_err=BIO_new_fp(stderr, BIO_NOCLOSE);
    }

    /* Set up a SIGPIPE handler */
    signal(SIGPIPE,sigpipe_handle);
    
    /* Create our context*/
    meth=SSLv23_method();
    ctx=SSL_CTX_new(meth);

    /* Load our keys and certificates*/
    if(!(SSL_CTX_use_certificate_chain_file(ctx, keyfile))) {
      berr_exit("Can't read certificate file");
      return NULL;
    }
    
    /* Private Key is encrypted with password */
    pass=password;
    SSL_CTX_set_default_passwd_cb(ctx,password_cb);
    if(!(SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM))) {
      berr_exit("Can't read key file");
	return NULL;
    }
    /* Load the CAs we trust*/
    if(!(SSL_CTX_load_verify_locations(ctx, CA_LIST,0))) {
      berr_exit("Can't read CA list");
      return NULL;
    }

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
    SSL_CTX_set_verify_depth(ctx,1);
#endif
    
    return ctx;
  }

     
void 
destroy_ctx(ctx)
  SSL_CTX *ctx;
  {
    SSL_CTX_free(ctx);
  }



int
bio_printf(BIO *bio, const char *spcFmt, ...)
{
	    char cBuf[2048];
    	    va_list va;
	    int iRet = 0;
	    if (!bio)
		return -1;
		    
    	    memset(cBuf, 0x0, sizeof(cBuf));
    	    va_start(va, spcFmt);
    	    vsnprintf(cBuf, sizeof(cBuf) - 1,spcFmt, va);
    	    va_end(va);
	
            if ( (iRet = BIO_puts(bio, cBuf))<0) {
	    	    return -1;
	        }
            if ( (iRet = BIO_flush(bio))<0) {
		    return -1;
	    }

	    return 0;
}


void
chop(char *pcPtr) 
{
    int iX = 0;
    for (iX = 0; iX<strlen(pcPtr); iX++) 
	    if ((pcPtr[iX]=='\r')||(pcPtr[iX]=='\n')) pcPtr[iX]=0;
}


