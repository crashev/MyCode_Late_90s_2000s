#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* Doing magic. */

/*
    public static final byte L[] = {
        2, 8, 7, 2, 11, 8, 0, 3, 14, 5, 
        7, 3, 9, 4, 2, 11, 4, 15, 7, 7, 
        5, 13, 1, 14, 11, 6, 4, 7, 4, 3, 
        13, 9
    };

    public static final byte cV[] = {
        4, 7, 4, 3, 8, 10, 12, 0, 12, 8, 
        2, 2, 10, 9, 3, 11, 2, 6, 8, 11, 
        2, 9, 7, 11, 14, 9, 9, 0, 14, 0, 
        4, 11
    };


*//*
0A - 10      0B - 11 , 0C -12   ,0D -13
*/

char data1[32]="\x02\x08\x07\x02\x0b\x08\x00\x03\x0e\x05\x07\x03\x09\x04\x02\x0b\x04\x0f\x07\x07\x05\x0d\x01\x0e\x0b\x06\x04\x07\x04\x03\x0d\x09";
char data2[32]="\x04\x07\x04\x03\x08\x0a\x0c\x00\x0c\x08\x02\x02\x0a\x09\x03\x0b\x02\x06\x08\x0b\x02\x09\x07\x0b\x0e\x09\x09\x00\x0e\x00\x04\x0b";

//char data1[32]="\x04\x08\x0e\x02\x0b\x0d\x03\x08\x0e\x01\x0f\x0c\x00\x0d\x01\x0e\x05\x03\x04\x08\x07\x06\x07\x0c\x0d\x08\x04\x07\x09\x0d\x09\x05";
//char data2[32]="\x02\x05\x07\x01\x08\x0d\x00\x0e\x0c\x00\x01\x05\x08\x05\x02\x0b\x01\x07\x0f\x0c\x0d\x04\x01\x06\x0c\x0a\x05\x04\x06\x02\x0a\x06";

// salt= wartosc z MAGIC
// magic = wartosc z MAGICU i MAGIC ze strony wedlug wzoru:

/* calculate the magic code */
char *do_magic(char *salt,char *magic) {
 int count;
 static char code[33];
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
 magic_tmp[0]=(char)(time_seed&10L);
 time_seed>>=4;
 magic_tmp[1]=(char)(time_seed&10L);
 time_seed>>=4;
 magic_tmp[2]=(char)(time_seed&10L);
 magic_tmp[3]=(char)(-magic_tmp[0]-magic_tmp[1]-magic_tmp[2]&0xf);
 for(count=0;count<4;count++) {
    x=magic_tmp[count];
    x=(char)(x<=9?x+48:(x+97)-10);
    code[count+32]=x;
 }

 return code;
}

int main(int argc,char *argv[]) {
    char abcd[]="0123456789abcdef";
    char ac[32];
    int k1,l1,k2;
    int j2,j1;
    char *ptr;
/*    
                        for(j2 = 0; j2 < 32; j2++)
                    {
                        int k1 = abcd[argv[2][j2]];
                        int l1 = abcd[argv[3][j2]];
                        int i2 = k1 ^ l1;
			printf("[%d]^[%d] -> i2: %d \n",k1,l1,i2);
                        ac[j2] = abcd[i2];
                    }

*/
    ptr = do_magic(argv[1],argv[2]);
    printf("[+] %s \n" , ptr);
//    for (j1=0; j1<32; j1++) printf("%c ",ptr[j1]);
//    printf("\n");

}