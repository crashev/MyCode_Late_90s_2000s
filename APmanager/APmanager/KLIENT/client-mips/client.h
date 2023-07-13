#ifndef _client_h
#define _client_h

int config_download(char *pcHost, char *pcConfig);
int config_load(char *pcConfig);
int config_sync(void);

#endif
