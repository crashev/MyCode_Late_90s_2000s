#ifndef _APROTOCOL_H
#define _APROTOCOL_H
#include <common.h>
#include "server.h"

extern int do_admincmd(char *pcBuffor, int iSock);
extern int do_adminreply(int iX, char *pcBuf, int aSock);
extern void switch0(void);

#endif
