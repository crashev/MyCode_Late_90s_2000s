/*
        Komunix (c) - Pawel Furtak '2005 <pawelf@bioinfo.pl>

*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "log.h"

FILE *open_log_file(char *pcLogf) {
    FILE *Ff = fopen(pcLogf, "a+");
    return (FILE *)Ff;
}

void debug(FILE *Fgdzie, char *pcFmt, ...) {
    char cBuf[4048];
    char cTimebuf[30];
    va_list ap;
    time_t t;
    struct tm *tm;

//#ifndef DEBUG
//    return;
//#endif
        if (!Fgdzie) return;
	t = time(NULL);
	bzero(cTimebuf,sizeof(cTimebuf));
	tm = localtime(&t);
	strftime(cTimebuf, sizeof(cTimebuf)-1, "%H:%M:%S", tm);
        memset(cBuf, 0x0, sizeof(cBuf));
        va_start(ap, pcFmt);
        vsnprintf(cBuf, 4040, pcFmt, ap);
        va_end(ap);
        fprintf(Fgdzie, "%s -> %s", cTimebuf, cBuf);
        fflush(Fgdzie);
}


int
mprintf(const char *spcFormat, ...)
{
            static char scBuf[1024];
	    va_list ap;
            memset(scBuf, 0x0, sizeof(scBuf));
            va_start(ap, spcFormat);
            vsnprintf(scBuf, sizeof(scBuf) - 1, spcFormat, ap);
            va_end(ap);
            switch (LOGLEVEL) {
        	case 0: return 0;
		case 1: return (printf("%s", scBuf));
		case 2: debug(Flogfile, "%s", scBuf);
			return 0;
		default: return -1;
	    }
} 
