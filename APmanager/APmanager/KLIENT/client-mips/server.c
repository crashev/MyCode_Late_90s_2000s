/*
    FIXME: Add external connect timeout?
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

int 
server_open_connection(const char *pcHost, int iPort)
{
	struct sockaddr_in sin;
	struct hostent *hp = NULL;
	int iSock=-1;
	
	memset(&sin, 0x0, sizeof(sin));
	
	iSock = socket(PF_INET, SOCK_STREAM, 0);
	if (iSock<0) 
	    return -1;
	    
	sin.sin_family = PF_INET;
	sin.sin_port = htons(iPort);
	
	hp = gethostbyname2(pcHost, PF_INET);
	if (hp)
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
	else
	{
	        close(iSock);
		return -2;
	}
	
	if (connect(iSock, (struct sockaddr *)&sin, sizeof(sin))<0) {
		close(iSock);
		return -3;
	}

    	return iSock;
}

