/* 
    Dead File
    Komunix (c) - Pawel Furtak '2005 <pawelf@bioinfo.pl>
*/

#include <stdio.h>
#include <string.h>

#include "client.h"
#include "log.h"

extern FILE *Flogfile;

/***** Config section ******/

/* 
    Config download
	host  	-  hostname of server with DATABASE
	config  -  name of config for this host, probably associeted with APname
	   
*/

int 
config_download(char *pcHost, char *pcConfig) 
{
    return 0;
}



int
config_load(char *pcConfig)
{
    FILE *Ff;
    
    if ( ! (Ff = fopen(pcConfig, "r")) ) {
	debug(Flogfile, "[!] Error opening config file: %s - file corrupted or first run?\n", pcConfig);
	debug(Flogfile, "[?] Its OK not to have local config file, however user's network wont be availble till server download proper config\n");
	return -1;
    }

    return 0;
}


int
config_sync(void)
{
    return 0;
}
