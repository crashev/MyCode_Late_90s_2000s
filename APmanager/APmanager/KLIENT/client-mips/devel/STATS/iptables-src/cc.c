#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int main() {
    int iC;
	struct IpClientStruct {
	    unsigned long in;
	    unsigned long out;
	    char cIP[15];
	};
	char *name;
//	struct 	    IpClientStruct **IpClient;
	struct 	    IpClientStruct IpClient[10];
	struct 	    IpClientStruct **IpClient2;

	IpClient2 = (struct IpClientStruct **)calloc(4, sizeof(struct IpClientStruct *));
	
	for (iC=0; iC<4; iC++) {
	    IpClient2[iC] = (struct IpClientStruct *)calloc(1, sizeof(struct IpClientStruct));
	}	
	bzero(&IpClient, sizeof(IpClient));	
//	bzero(&IpClient2, sizeof(4*sizeof(struct IpClientStruct *)));	

//	IpClient = (struct IpClientStruct **)malloc(4 * sizeof(struct IpClientStruct));
//	bzero((struct IpClientStruct **)IpClient, (4* sizeof(struct IpClientStruct)));
//	for (iC=0; iC<4; iC++)
//	    bzero((struct IpClientStruct *)&IpClient[iC], sizeof(struct IpClientStruct));

//	strcpy(((struct IpClientStruct *)&IpClient2[0])->cIP, "192.168.0.1");
//	strcpy(((struct IpClientStruct *)&IpClient2[1])->cIP, "192.155.140.130");

	strcpy(IpClient2[0]->cIP, "192.168.0.1");
	strcpy(IpClient2[1]->cIP, "192.155.140.130");

	for (iC=0; iC<4; iC++) {
	    printf("[%s][IN: ][OUT: ]\n", IpClient2[iC]->cIP);
	}

	for (iC=0; iC<4; iC++) {
	    if (IpClient2[iC])
	        free(IpClient2[iC]);
	}	
	free(IpClient2);

	
    return 0;
}
