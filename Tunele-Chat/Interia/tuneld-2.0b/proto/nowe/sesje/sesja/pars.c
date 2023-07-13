#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc,char *argv[]) {
    int p,c=0,kk;
    char z,zz;
    int seg=1;    
    char nullbuf[45];
    printf("-= Analizator =-\n");

    
    if (argc<3) {
        printf("Usage: [%s] [plik] [seg]\n", argv[0]);
	return -1;
    }
    printf("Opening: %s \n", argv[1]);
    seg = atoi(argv[2]);
    if (seg==0) seg =1 ;
    
    if ( (p=open(argv[1], O_RDONLY)) < 0) {
	perror("open() ");
	return -1;
    }
    
    // jump to position
    for (kk=0; kk<seg;kk++) {
        lseek(p,0x49,SEEK_CUR);
    }
    while ( read(p,&z,1)) {
	if (z==0x20) break;
    }

    read(p,&z,1);
        
    while ( read(p,&z,1)) {
	printf("%d = [%x][%d] \n",c,(z & 0xff),(z & 0xff));
	if ((zz=='1')&&(z=='1')) break;
	c++;
	zz=z;

    }

    printf("\nDone.\n");
}