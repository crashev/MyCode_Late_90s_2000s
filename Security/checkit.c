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
 struct ifreq *ifek;
 struct sockaddr_in *pp;
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

// ilosc interfejsow ktora odebralismy to ifc_len / sizeof(if_req)
ilosc=(conf.ifc_len/sizeof(struct ifreq));
printf("Mamy %d interfejsow\n",ilosc);
    printf("Interfejs\tHOST\t\t\t\tPROMISC\n");
for (i=0;i<ilosc;i++) {
    printf("%s[%d]\t\t",ifek->ifr_name,i);
//    printf("%i ",ifek->ifr_flags);

// union
// struct sockaddr ifru_addr;
// #define	ifr_addr	ifr_ifru.ifru_addr	/* address		*/
//
    pp=(struct sockaddr_in *)&ifek->ifr_addr;
    printf("%s\t\t",hostlookup(pp->sin_addr.s_addr));
    if (ifek->ifr_flags & IFF_PROMISC ) { printf("PROMISC MODE!\n"); } else { printf("NO PROMISC MODE!\n");}

  ifek++;
  }
return 0;
} 