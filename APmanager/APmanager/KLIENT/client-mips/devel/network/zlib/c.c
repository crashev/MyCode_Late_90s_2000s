#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <stdlib.h>

#include <sys/types.h>
#include <fcntl.h>



int main() {
    char cPath[255];
    char cBuf[1024];
    FILE *fIn;
    gzFile file;
    int iSize = 0, iC = 0, iCount = 0;
    char *ptr;
    char temp[20];
    int iErr = 0;
    Byte *uncompr;
    uLong uncomprLen = 1024*sizeof(int);
    int fOut;
    
    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
    if (!uncompr) {
	printf("[UPDATE]: Error allocating uncompr \n");
	return -1;
    }    

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "APclientd");

    /* First pack the file */
    if ( (fIn = fopen(cPath, "rb"))==NULL )
	return -1;
    bzero(temp, sizeof(temp));
    strcpy(temp,"APclientd.XXXXXX");
    ptr = mktemp(temp);
    if (!ptr)	
	return -1;
	
    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "%s", ptr);

    file = gzopen(cPath, "wb");
    if (file == NULL) {
	fclose(fIn);
	return -1;
    }
    
    bzero(cBuf, sizeof(cBuf));
	fseek(fIn, 0x0, SEEK_END);
        iSize = ftell(fIn);
	fseek(fIn, 0x0, SEEK_SET);
        
    while ( iCount < iSize ) {
        iC = fread(cBuf, 1, sizeof(cBuf) - 1, fIn);
	gzwrite(file, cBuf, iC);
        printf("gzputs err: %s\n", gzerror(file, &iErr));
	iCount += iC;
    }
    gzclose(file);
    fclose(fIn);


    iSize = iC = iCount = 0;
    
    /* rozpakowujemy */
    
    if ( (file = gzopen(cPath, "rb"))==NULL )
	return -1;

    bzero(cPath, sizeof(cPath));
    snprintf(cPath, sizeof(cPath) - 1, "APclientd.unpacked");

    fOut = open(cPath, O_WRONLY, 0777);
    if (fOut<= 0) {
        gzclose(file);
        return -1;
    }
    iC =1 ;	
    while ( iC = gzread(file, uncompr, (unsigned)uncomprLen) ) {

	    printf("gzread err: %s [iC: %d]\n", gzerror(file, &iErr), iC);
	    
	    if (write(fOut, uncompr, iC)<0) {
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
    

}
