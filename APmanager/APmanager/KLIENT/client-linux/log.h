#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>

extern FILE *Flogfile;
extern int LOGLEVEL;

void debug(FILE *Fgdzie, char *pcFmt,...);
FILE *open_log_file(char *pcLogf);
int mprintf(const char *spcFormat, ...);

#endif
