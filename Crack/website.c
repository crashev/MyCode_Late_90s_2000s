// 	Crack For WebSite Extractor 8.35 by crashev'2002
// To download the WebSite Extractor program go to: 
// http://www.internet-soft.com
// 
// For Education Purposes ONLY - BUY THIS SOFTWARE AND SUPPORT CODERS!!
//

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <mem.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>

#define file2crack "webextra.exe"
#define version "WebSite Extractor 8.35"

char c0de1[] = "\x90\x90";      // NOP NOP
char c0de2[] = "\xE9\xA7\x00"; // jmp 
char c0de3[] = "\x90";         // NOP for align
long OF;

int main(void) {
  int  pipe;
  

printf("\n-= Crack For : %s =-\n",version);

	pipe=open(file2crack,O_RDWR);
if (pipe==-1) 
{
	fprintf(stderr,"Cant read file %s \nReason: \n    - file in use\n    - file not found\n",file2crack);
	exit(-1);
}
printf("File %s opened ..\n",file2crack);

OF=0x000CD1BF;lseek(pipe,OF,SEEK_SET);write(pipe,c0de1,strlen(c0de1));
OF=0x000CD1D0;lseek(pipe,OF,SEEK_SET);write(pipe,c0de2,3);
OF=0x000CD1D5;lseek(pipe,OF,SEEK_SET);write(pipe,c0de3,strlen(c0de3));

printf("File %s succesfull cracked ..\n",file2crack);
return 0;
}
