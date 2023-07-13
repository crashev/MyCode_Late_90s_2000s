#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

void debug(FILE *gdzie,char *fmt,...) {
    char buf[4048];
    char timebuf[30];
    va_list ap;
    time_t t;
    struct tm *tm;

        t=time(NULL);
	bzero(timebuf,sizeof(timebuf));
	tm=localtime(&t);
	strftime(timebuf, sizeof(timebuf)-1, "%H:%M:%S", tm);
        memset(buf, 0x0, sizeof(buf));
        va_start(ap, fmt);
        vsnprintf(buf, 4040, fmt, ap);
        va_end(ap);
        fprintf(gdzie, "%s -> %s", timebuf, buf);
        fflush(gdzie);
}
