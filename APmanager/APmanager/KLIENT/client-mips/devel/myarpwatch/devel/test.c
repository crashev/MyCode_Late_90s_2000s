#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
   struct sockaddr_in gw_addr;
   struct arpreq myreq;
   int sockfd, i;
   FILE *logfd;

   logfd = fopen("logfile", "w+");
   memset(&gw_addr, '\0', sizeof(gw_addr));
   gw_addr.sin_family = AF_INET;
   gw_addr.sin_addr.s_addr = inet_addr("192.168.0.3");

   memset(&myreq, '\0', sizeof(myreq));
   memcpy(&myreq.arp_pa, &gw_addr, sizeof(gw_addr));
   memcpy(&myreq.arp_dev, "eth0", sizeof(myreq.arp_dev));

   if ((sockfd = socket(AF_INET,SOCK_DGRAM, 0)) < 0) {
      perror("socket");
      exit(-1);
   }

   if (ioctl(sockfd,SIOCGARP,(unsigned char *)&myreq) < 0) {
      perror("ioctl");
      exit(-1);
   }

   for ( i = 0; i < 6; i++)
   fprintf(stdout,"%02x%s",(unsigned char)myreq.arp_ha.sa_data[i], i == 5 ?"\n": " ");

   return 0;
}
