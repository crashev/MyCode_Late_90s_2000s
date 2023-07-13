
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <time.h>       

#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <zlib.h>

extern int errno;



#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}











int test_inflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 0;
    d_stream.next_out = uncompr;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    
    printf("[inflate]: %d \n", d_stream.total_out);
    return d_stream.total_out;
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
    char *cMainHost="127.0.0.1";
    int err;

    Byte *compr, *uncompr;
    uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
    uLong uncomprLen = comprLen;

    compr    = (Byte*)calloc((uInt)comprLen, 1);
    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);



    iSock = socket(PF_INET, SOCK_STREAM, 0);
    if (iSock<0) 
        return -1;

    sin.sin_family = PF_INET;
    sin.sin_port   = htons(iPort);
	printf("[DOWNLOAD-BINARY]: Connecting to: %s:%d \n", "127.0.0.1", iPort);
    
    hp = gethostbyname2(cMainHost, PF_INET);
    if (hp)
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    else {
	        close(iSock);
		printf("[DOWNLOAD-BINARY]: Error resolving host \n");
		return -1;
	}

    if (connect(iSock, (struct sockaddr *)&sin, sizeof(sin))<0) {
		close(iSock);
		printf("[DOWNLOAD-BINARY]: Error connecting to host: %s \n", strerror(errno));
		return -1;
    }


    iFile = open(pcPath, O_WRONLY|O_CREAT|O_TRUNC, 0777);
    if (iFile<0) {
	printf("[DOWNLOAD-BINARY]: Was not able to create %s -> %s \n", pcPath, strerror(errno));
	close(iSock);
	return -1;
    }

    printf("[DOWNLOAD-BINARY]: Downloading %d bytes!\n", iSize);
    
    bzero(cBuf, sizeof(cBuf));
    iC = 1;
    while ( (iCount < iSize) ) {
	iC = read(iSock, cBuf, sizeof(cBuf) - 1 );
	printf("[READ]: %d \n", iC);
	err=test_inflate(cBuf, iC, uncompr, uncomprLen);

	write(iFile, uncompr, err);
	bzero(cBuf, sizeof(cBuf));
	bzero(uncompr, uncomprLen);

	iCount += err;
	printf("[?]-> %d -> iC: %d\n", iCount, iC);
	usleep(500);	
    }
	printf("[DOWNLOAD-BINARY]: Done!\n");
    close(iFile);
    close(iSock);
    return 0;
}

int main(int argc, char *argv[]) {
    download_binary(atoi(argv[1]),"tutaj", atoi(argv[2]));
}
