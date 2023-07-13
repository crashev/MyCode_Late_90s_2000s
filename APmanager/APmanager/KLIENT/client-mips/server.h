#ifndef _SERVER_H
#define _SERVER_H

#define KEYFILE "client.pem"
#define PASSWORD "password"

extern int server_open_connection(const char *pcHost, int iPort);

#endif
