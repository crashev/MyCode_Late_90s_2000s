/* Doing magic. */

#include "main.h"
#include "conf.h"
#include "magic.h"


//char data1[32]="\x07\x09\x03\x0c\x08\x0f\x0a\x0e\x08\x08\x07\x0b\x01\x06\x03\x07\x03\x08\x0f\x09\x02\x04\x04\x0c\x0e\x0e\x00\x0c\x0a\x0a\x0e\x09";
//char data2[32]="\x04\x08\x05\x0e\x05\x0b\x07\x04\x0d\x09\x09\x07\x0e\x0d\x03\x0b\x06\x05\x08\x09\x00\x00\x0c\x02\x0f\x06\x05\x0c\x07\x0c\x0e\x05";
char data1[32]="\x02\x08\x07\x02\x0b\x08\x00\x03\x0e\x05\x07\x03\x09\x04\x02\x0b\x04\x0f\x07\x07\x05\x0d\x01\x0e\x0b\x06\x04\x07\x04\x03\x0d\x09";
char data2[32]="\x04\x07\x04\x03\x08\x0a\x0c\x00\x0c\x08\x02\x02\x0a\x09\x03\x0b\x02\x06\x08\x0b\x02\x09\x07\x0b\x0e\x09\x09\x00\x0e\x00\x04\x0b";

/* calculate the magic code */
char *do_magic(char *salt,char *magic) {
 int count;
 static char code[37];
 char salt_tmp[8],magic_tmp[32],x,y;
 char tab1[32],tab2[32];
 unsigned long long time_seed;
 struct timeval tv;

 memcpy(salt_tmp,salt,sizeof salt_tmp);
 memcpy(magic_tmp,magic,sizeof magic_tmp);
 memcpy(tab1,data1,sizeof data1);
 memcpy(tab2,data2,sizeof data2);
 bzero(code,sizeof code);

 for(count=0;(count&0x20)==0;count++) {
    tab1[count]=(char)(tab1[count]+count&0xf);
    tab2[count]=(char)(tab2[count]-count&0xf);
 }

 for(count=0;count<32;count++) {
    x=magic_tmp[count];
    y=salt_tmp[count&0x7];
    x=(char)(x<=57?x-48:(x-97)+10);
    y=(char)(y<=57?y-48:(y-97)+10);
    x=(char)((x^tab1[count]^y)+tab2[count]&0xf);
    x=(char)(x<=9?x+48:(x+97)-10);
    code[count]=x;
 }

 gettimeofday(&tv,NULL);
 time_seed=tv.tv_sec;
 time_seed=(time_seed*1000)+(tv.tv_usec/1000);
 magic_tmp[0]=(char)(time_seed&15L);
 time_seed>>=4;
 magic_tmp[1]=(char)(time_seed&15L);
 time_seed>>=4;
 magic_tmp[2]=(char)(time_seed&15L);
 magic_tmp[3]=(char)(-magic_tmp[0]-magic_tmp[1]-magic_tmp[2]&0xf);
 for(count=0;count<4;count++) {
    x=magic_tmp[count];
    x=(char)(x<=9?x+48:(x+97)-10);
    code[count+32]=x;
 }

 return code;
}
