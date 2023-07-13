//
//
//
// Ten program za pomoca SIOCGIFCONF pobiera liste interfejsow wystepujacych
// w systemie a pozniej sprawdza czy ktorys z nich nie ma PROMISC mode
//
// crashev@crashev.ebox.pl

#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>

#include <netdb.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/types.h>

#include <linux/socket.h>
// for check promisc mode :
//
// 0x00008913  SIOCGIFFLAGS      struct ifreq *           // I-O
// 0x00008914  SIOCSIFFLAGS      const struct ifreq *
//
// So lets go.


char * hostlookup(unsigned long in)
{
	static char buforek[1024];
	struct hostent * he;
	struct in_addr i;
	
	i.s_addr = in;
	he = gethostbyaddr((char *)&in, sizeof(in), AF_INET);
	if (he)
		strcpy(buforek, he->h_name);
	else 
		strcpy(buforek, inet_ntoa(i));
	return buforek;
}

int main(void) {
 struct ifconf conf;
 struct ifreq *ifek,ifek2;
 struct sockaddr_in *pp;
 struct etheraddr *eth;
 u_char *hwaddr;
 int stat;
 char buf[8048];
 int m=-1,i,ilosc;
//getting all interfaces

m=socket(AF_INET,SOCK_STREAM,0); // allokujemy sockecik ktorym sprawdzimy
                                    // interfejsy
if (m==-1) {perror("cant open socket ");exit(-1);}
conf.ifc_len=sizeof(buf);
conf.ifc_buf=buf;

stat=ioctl(m,SIOCGIFCONF,&conf);
if (stat==-1) {perror("Somethink goes wrong ");exit(-1);}
ifek=conf.ifc_req;
//close(m);
// ilosc interfejsow ktora odebralismy to ifc_len / sizeof(if_req)
ilosc=(conf.ifc_len/sizeof(struct ifreq));
printf("Mamy %d interfejsow\n",ilosc);
    printf("Interfejs\tHOST\t\t\t\tHWADDR\n");
for (i=1;i<=ilosc;i++) {
    bzero(&ifek2, sizeof(ifek2));
//    ifek2.ifr_ifindex = i;
    strncpy(ifek2.ifr_name, ifek->ifr_name, sizeof(ifek2.ifr_name)-1);
    ioctl(m, SIOCGIFHWADDR, &ifek2);
    perror("ioctl");
    printf ("\n%s \t\t ",ifek->ifr_name);
    hwaddr = (unsigned char *)&ifek2.ifr_hwaddr.sa_data;
//    ioctl(m, SIOCGIFHWADDR, &ifek2);
//    perror("ioctl");
    //printf ("\nInterfejs nr. %i to %s. -> ",ifek2.ifr_ifindex,ifek2.ifr_name);


// union
// struct sockaddr ifru_addr;
// #define	ifr_addr	ifr_ifru.ifru_addr	/* address		*/
//
    pp=(struct sockaddr_in *)&ifek->ifr_addr;
    eth=(struct etheraddr *)&ifek->ifr_hwaddr;
    printf("%s\t\t",hostlookup(pp->sin_addr.s_addr));
// ethernet like interface
if (ifek2.ifr_hwaddr.sa_family==1)
    printf("[%x:%x:%x:%x:%x:%x] \n",hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

  ifek++;
  }
return 0;
} 
