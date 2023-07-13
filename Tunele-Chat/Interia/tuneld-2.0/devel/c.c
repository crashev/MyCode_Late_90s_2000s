#include <stdio.h>
#include <time.h>


int main(void) {
    time_t time1,time2;
    double n;
    
    time1=time(NULL);
    sleep(2);
    time2=time(NULL);
    n=difftime(time2,time1);
    printf("Difftime: %d  \n",(int)n);
}