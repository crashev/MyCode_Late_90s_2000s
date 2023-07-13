/* 

    Client Protocol
    
*/
#ifndef _CPROTOCOL_H
#define _CPROTOCOL_H

#include <common.h>

extern int do_cmd(char *pcBuffor, BIO *bio);
extern unsigned long sum(char *pcFile);

#endif
