#include <stdio.h>
#include <string.h>
char *czat_privmsg_crypt(char *nick, char *p, int length ) {
    char *tmp;
    int klucz=strlen(nick);
    int x,i;

//    debug(log,"[xalloc] czat_privmsg_crypt xalloc	\n");
//    debug(log,"[crypt-length]: %d \n",length);
//    length = strlen(p);
//    if ( !(tmp = xalloc(strlen(p)+1)) )
//    if ( !(tmp = xalloc(length+1)) )
//       { debug(log,"[xalloc] null xalloc in czat_privmsg_crypt\n"); return NULL; }
    x=0;
    for (i=0;i<length;i++) {
	    	tmp[i]=p[i]^(nick[x]^88);
		if (x++==klucz-1) x=0;
    }

    return tmp;
}

int main(void) {
//stukipuki HVH@R
//wichura000 -?=1E^MRG sIYYD\;JPICIVGJIXXL^ C792"
    printf("Test: %s \n",czat_privmsg_crypt("wichura000","-?=1E^MRG sIYYD\;JPICIVGJIXXL^ C792",48));
}
