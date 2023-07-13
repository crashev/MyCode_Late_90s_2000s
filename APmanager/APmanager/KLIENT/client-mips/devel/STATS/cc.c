#include <stdio.h>

int main(void) { 
    char n1[50],n2[50],n3[50],n4[50];
    int a[30];
    char *buf="eth0:9999 1000 2000 3000 0 4000\n\0";
    
    sscanf(buf,"eth0:9999 %*s %s", n1, n2);
    
    printf("n1: %s \n",n1);    
    printf("n2: %s \n",n2);    

}

