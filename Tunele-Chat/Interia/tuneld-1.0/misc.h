#include <stdlib.h>

struct client_struct {
    int fd;
    int stat;
    int toczat;
    char czathost[255];
    char user[255];
    char nick[50];
    char kanal[200];
    int port;
};
