#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int p;
    char c;
    int i;
    if (argc<2) {
	printf("[%s] [plik do zaanalizowania] \n",argv[0]);
	return -1;
    }
    
    printf("[+] Analysing %s \n",argv[1]);
    if ( (p = open(argv[1],O_RDONLY))<0) {
	perror("[-open] ");
	return -1;
    }
    
    read(p,&c,1);
    printf("[1] Dlugosc wczytania: %d[%x] \n",c,c);
    for (i=0; i<c; i++) {
	read(p,&c,1);
    }

    read(p,&c,1);
    printf("[2] Dlugosc wczytania: %d[%x] \n",c,c);

    return 0;
}