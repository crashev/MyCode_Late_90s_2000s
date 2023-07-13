/*            
		 
*/
// Includes Here
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>

void Logo(void) {
printf("\n\n");
printf("+-----------------------------------+\n");
printf("|     Client 			    |\n");
printf("+-----------------------------------+\n\n");
}

int main(int argc,char *argv[]){
    fd_set readfds;
    struct sockaddr_un sockadres;
  char sockbuf[2048];
    struct hostent *hp;
    char *host;
    char buforek[1024]; /* Taki buforek na komendy powinien starczyc ;) */
    int port;
    int ret,x;
    int pipe;
Logo();

    pipe=socket(PF_UNIX,SOCK_STREAM,0);
    x=sizeof(sockadres);
/* Opisujemy nasz hoscik */
    sockadres.sun_family=PF_UNIX;
    strcpy(sockadres.sun_path,"../admin/admin.sock");

/* Polaczenie */
    if (connect(pipe,(struct sockaddr *) &sockadres, sizeof(sockadres)) < 0){
       perror("Somethink Wrong With Connection.....!\n\n");
       exit(0);
    } 

    printf("[:)] Connected .... \n");
/* Telnet Connection By Crashkiller '98 */

//  sprintf (sockbuf, "/bin/uname -a;/usr/bin/id\n");
//  write (pipe, sockbuf, strlen(sockbuf));

while (1)
    {
      FD_ZERO (&readfds);
      FD_SET (0, &readfds);
      FD_SET (pipe, &readfds);
      select (255, &readfds, NULL, NULL, NULL);
      if (FD_ISSET (pipe, &readfds))
        {
          memset (buforek, 0, 1024);
          ret = read (pipe, buforek, 1024);
          if (ret <= 0)
            {
              printf ("Connection closed by foreign host.\n");
              exit (-1);
            }
          printf ("%s", buforek);
//	  printf("\n------------------\n");
//	  for (x=0;x<=strlen(buforek);x++) 
//	  printf("%c -> %d -> %x \n",buforek[x],buforek[x],buforek[x]);
//	  printf("\n------------------\n");
//          exit(1);
        }
      if (FD_ISSET (0, &readfds))
        {
          memset (buforek, 0, 1024);
          read (0, buforek, 1024);
          write (pipe, buforek, strlen(buforek));
	  write (pipe,"\r\n",2);
        }
    }
close(pipe); 
}




