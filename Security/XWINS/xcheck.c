// ---------------------------------------
// XServer Access Control Checker
// Coded By Crashkiller '99
// Thx to : #hackingpl
// --------------------------------------
// Compile : gcc -o xcheck xcheck.c -lX11

// To work we need includez and Xlib
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>

Display *x;
void main(int argc,char *argv[]) { 
int i;
char host[255];
   fprintf(stderr,"\n+----------------------------+\n");
     fprintf(stderr,"+--- X Access Checker V.1. --+\n");
     fprintf(stderr,"+--  Coded By Crashkiller   -+\n");
     fprintf(stderr,"+----------------------------+\n");

if (argc<2) { fprintf(stderr,"Usage : %s [host/ip]\n",argv[0]);exit(-1);}
if (strlen(argv[1])>254) { fprintf(stderr,"Host Too Long!\n");exit(-1);}
strcpy(host,argv[1]);
strcat(host,":0.0");
  printf("Trying To Open Display On : %s\n",host);
x = XOpenDisplay(host);
if (x==NULL) { printf("Cant Connect To Xopen Display On : %s \n",host);exit(10);}
  else { printf("We have access control on : %s \n",host); }
}