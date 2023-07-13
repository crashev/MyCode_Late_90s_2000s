//
// Checks For Elements Under Xwin That Are In Use
//
//
// Compile : gcc -o xcheck xcheck.c -lX11

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>

Display *x;
    char co[600];
    char naco[600];
void usage(char *cmd) {
         fprintf(stderr,"Usage : %s [host/ip] [-c co-zmienic] [-n na-co-zmienic]\n",cmd);
exit(-1);}
void fuckit(Window root);
void main(int argc,char *argv[]) { 
    char host[255];
  int i;
    if (argc<4) 
    { 
         fprintf(stderr,"Usage : %s [host/ip] [co zmienic] [na cozmienic]\n",argv[0]);
	 exit(-1);
    }
    if (strlen(argv[1])>254) 
    { 
	 fprintf(stderr,"Host Too Long!\n");
	 exit(-1);
    }
    strcpy(host,argv[1]);
    strcat(host,":0");
    strcpy(co,argv[2]);
if (argc>4) {
    strcat(co," ");
    strcat(co,argv[3]);
}
if (argc==4) strcpy(naco,argv[3]); else strcpy(naco,argv[4]);
if (argc>5) {
    strcat(naco," ");
 strcat(naco,argv[5]);
}
        printf("Trying To Open Display On : %s\n",host);
    x = XOpenDisplay(host);
       
if (x==NULL) 
    {
   printf("Cant Connect To Xopen Display On : %s \n",host);
   exit(10);
    }  else { 
    printf("We have access control on : %s \n",host); 
    }
    printf("Zmieniamy    : %s \n",co);
    printf("Zmieniamy Na : %s \n",naco);
    // ------------------------------
printf("Elements On Screen : \n");
fuckit(DefaultRootWindow(x));
printf("\n");
XCloseDisplay(x);
}
void fuckit(Window root) {
    char *windowname;
    unsigned int nchildren;
    Window parent,*children;
int i;
 XQueryTree(x,root,&root,&parent,&children,&nchildren);
if (nchildren==0) return;

for (i=0;i<nchildren;i++) {
XFetchName(x,children[i],&windowname);
if (windowname) { printf("%s  , ",windowname);
 if (strcmp(windowname,co)==0) { XStoreName(x,children[i],naco); }}
fuckit(children[i]);
}
}