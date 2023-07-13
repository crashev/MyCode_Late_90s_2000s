#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

int main(int argc,char *argv[]) {
    char linia[255];
    char znak;
    int in,out;
    int x=0,y=0;    
    char *r,*l;
if (argc<3) {
    printf("[%s] [in] [out] \n",argv[0]);
    return 0;
}
    
    in=open(argv[1],O_RDONLY);
    if (in<=0) {perror("open() ");return -1;}
    out=open(argv[2],O_CREAT|O_WRONLY|O_TRUNC,0666);
    if (out<=0) {perror("open() ");return -1;}
    
    strcpy(linia,"const char *kanaly[]={\n");
    write(out,linia,strlen(linia));
    bzero(linia,sizeof(linia));
    
    while (read(in,&znak,1)) {
	switch (znak) {
	    case '#': if (x>0) //write(out,linia,strlen(linia));
		      r = rindex(linia,'|')+1;
		      l = strtok(linia,"|");
		      if ( r && l ) {
		      write(out,"\"",1);
		      write(out,l,strlen(l));
		      write(out,":",1);
		      write(out,r,strlen(r));
		      write(out,"\",",2);
		      write(out,"\n",1);
		      }
		      bzero(linia,sizeof(linia));	
		      x=0;	
	              break;
	    case '{': bzero(linia,sizeof(linia));
	              x=0;
		      break;
	    default: linia[x++]=znak;
	    if ( (x+1) == 254) {printf("overflow in buf\n");return -1;}
	}
    }
    write(out,"0};",3);
    close(in);
    close(out);
    return 0;
}