/*
 *
 *  drawtopn.c
 *   
 *  histogram drawing program for ip flows.
 *
 *  uses libpng for gd library features  
 *  
 *  take command line data sources and generate output PNG image
 *
 *  syntax:
 *
 *	note: -d must be LAST argument.  If not supplied,
 *		we assume no arguments (generates a null graph).
 *		# args following -d must be twice the number of args
 *		given to -n.
 *	      -f must be supplied.
 *	      -n must be supplied.
 *
 *	% drawtopn -t "horizontal title" 
 *		   -v "vertical title"
 *                 -s "time stamp"
 *	           -l  dns name lookup 
 *	               0   no dns name lookup
 *	               1   DNS name lookup 
 *	               2   only show DNS name as legend
 *		    note: DNS lookup timesout in 3 seconds.
 *		    This is a race condition with ourmon in
 *		    that it make take drawtopn to long to timeout
 *		    if IP addresses do not resolve to DNS names.
 *		   -f "output file name"
 *		   -n # of data input pairs
 *		   -e allow errors and/or stdout - turned off by default.
 *		   -h help
 *		   -d last parameter, followed by data input pairs
 *	% drawtopn -f foo.png -n 2 -d 1.1.1.1 100000 2.2.2.2 200000
 *
 * author: Nan Sun,  sunnan@cs.pdx.edu
 * bug fixer: Jim Binkley, jrb@cs.pdx.edu
 *
 * Bugs:
 *	keep a sharp eye out for drawtopn runtime executables piling up
 *	because DNS is not resolving.  If so, use -l 0 (modify
 *	the omupdate script).  This version of drawtopn only
 *	expends DNSTIMEOUT seconds (3 ...) per attempt to resolve
 *	DNS names.  Every unresolved IP address will cost 3 seconds 
 *	or so.
 *
 *	SEE DNSTIMEOUT below if you want to change the timeout value.
 */ 

/*
 * Copyright (c) 2001, 2002, 2004 Portland State University 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Portland State University.
 * 4. The name of Portland State University may not be used to endorse 
 *    or promote products derived from this software without specific 
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR/S ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR/S BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



/* Bring in gd library functions */
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>   
#include <sys/socket.h>   
#include <signal.h>
#include <setjmp.h>
#include "gd.h"
#include "gdfontl.h"
#include "gdfontmb.h"
#include "gdfonts.h"
#include "gdfontt.h"

/* dns timeout time in seconds */
#define DNSTIMEOUT	3

// change following to reflect your local environment
#define Horizon_title "Top N"		/* default H titile */
#define Vertical_title "Bytes"		/* default V titile */
#define Logo "Ourmon Top_N"
#define PNG_OUT_FILE "topn.png"		/* default PNG file name */


// parameters for PNG images, do not change it 
#define WIDTH       545     /* width of png if less than 10 data sources*/
#define LENGTH      355     /* length if png */
#define MAX_N       100     /* total data sources can be showed */ 
#define Top_margin   30     /* top margin for inner data window */
#define Left_margin  90     /* left  margin for inner data window */
#define Right_margin 90     /* right  margin for inner data window */
#define DNS_name      0     /* 0   no DNS lookup	     */
	                    /* 1   DNS lookup                */
	                    /* 2   DNS lookup only as legend */   

char *program_name;  	    /* myself */	

typedef struct n_value {	     /* each data source contains 3 items  */
	char 		  *ipaddr;   /* IP address as string */
	unsigned long  	  counter;   /* packets byte counter as u_long */
	char 		*counter2;   /* packets byte counter as string */
} top_n_value;

// colors for each data bar
int color[MAX_N];

// array with MAX_N size for storing each IP:packets pair
top_n_value  *n_pair[MAX_N];

// DNS name lookup result 
char  result[1024]; 
char * dns_to_name( char *); 

int errorsok = 0;

int main(int argc,char **argv ) 
{
	int argcount = argc - 1;
	int zflag = 0;			/* no arguments */
	int nflag = 0;
	int fflag = 0;
	char **nargv = argv;

	/* Declare the image */
	gdImagePtr im;

	/* Declare output files */
	FILE *pngout;

	/* Declare color indexes */
	int black, gray, background;
	int white, red, blue, green;
	
	int x, y, w_data;
	int op, i; 
	char *cp;
	char *time_stamp="\0";
	int dflag = 0;
	int dns; 
	/* horizen title of the png, and vertical title, */
	/* overrided by command line parametes if there is*/
	char *h_title=Horizon_title;  
	char *v_title=Vertical_title;   
	
	/* png output file name */                           
	char *png_file=PNG_OUT_FILE;    	  
	
	/* the number of data sources actually need to be drawn */
	int top_n = 0;    
	
	/* index to IP:packets string*/
	int data_index;  
	
	extern char *optarg;
	extern int optind, opterr;

	x=WIDTH;
	y=LENGTH;
	
	/* width and length of inner data bar frame */
	w_data= 200;  
     
	dns = DNS_name;   
        /* get program name */	      

	if ((cp = (char *)strrchr(argv[0], '/')) != NULL)
		program_name = cp + 1;   
	else    
		program_name = argv[0];
	
        /* parsing command line parameters*/     
        while ((op = getopt(argc, argv, "t:v:s:l:n:d:f:he")) != EOF) {
		switch (op) {
	        case 't': 
			argcount -= 2;
	                h_title = optarg;                         
	                break;
	        case 'v':
			argcount -= 2;
	                v_title = optarg;
	                break;
	        case 's': 
			argcount -= 2;
	                time_stamp = optarg;                         
	                break;
	        case 'l':
			argcount -= 2;
			dns = atoi(optarg) ;
	                break;
	        case 'n':
			nflag = 1;
			argcount -= 2;
	                top_n   = atoi(optarg) ;
	                break;
		/* -d must be last arg
		*/
	        case 'd':                     
			argcount -= 1;		/* discount -d arg */
	                data_index = optind; 
	                dflag = 1;
			goto argcheck;
	                break;
	        case 'f':
			argcount -= 2;
	                png_file = optarg;
			fflag = 1;
			break;
	        case 'e':
			errorsok = 1;
			break;
	        case 'h':
	        default:
			argcount -= 1;
	                usage(argc, nargv);
	        }
	}

argcheck:
	
	if ( dns > 3 )  {
		dns = DNS_name;
	}

	if ( fflag == 0 ) {
		fprintf(stderr, "must have -f flag set \n");
		usage(argc, nargv);
	}

	if ( nflag == 0 ) {
		fprintf(stderr, "must have -n flag set \n");
		usage(argc, nargv);
	}

	/* top_n can be 0, but -n must be set.
	 * this forces out a print of no flows
	*/
	if (( top_n == 0 || dflag == 0)) {
		top_n = 1;
		zflag = 1;
	}
	/* top_n was at least 1
	*/
	else {
		if (top_n * 2 != argcount) {
			fprintf(stderr, "-d args (%d) must be 2 times top_n value (%d)\n", argcount, top_n);
			usage(argc, nargv);
			exit(1);
		}
	}

	if (!errorsok) {
		close(1);
		close(2);
	}

   	/*  malloc for topn data sources */
   	i=0;
  
	for (i = 0; i < top_n; i++) {
		n_pair[i] = (struct n_value *) malloc(sizeof(struct n_value)); 
		if (n_pair[i] == NULL) {
			fprintf(stderr, "malloc failure in drawtopn\n");
			usage(argc, nargv);
			exit(1);
		}
		/* deal with no data inputs
		*/
		if (zflag) {
			n_pair[i]->ipaddr = "none";
			n_pair[i]->counter= 0L;
			n_pair[i]->counter2= "0";
		}
		else {
			if ( dns == 1 )  
				n_pair[i]->ipaddr=dns_to_name(argv[data_index-1+2*(i)]);
			else 
				n_pair[i]->ipaddr=argv[data_index-1+2*(i)];
			n_pair[i]->counter= 
			    strtoul(argv[data_index-1+2*(i)+1], 
				(char **)NULL, 10);
			n_pair[i]->counter2=argv[data_index-1+2*(i)+1];
		}
		/*
   		printf ("%d    %s : %s\n", i, n_pair[i]->ipaddr, n_pair[i]->counter2);
		*/
	} 
   	
   	if ( top_n > 5 ) {
		x = x + ( top_n-6 )* (x - Left_margin - Right_margin)/5	;
	};

	/* increase the heigth for more space showing legend */
	if  ( (dns == 2) && ( top_n >= 3 ) ) {
		y = y + top_n * 14;
	};
	    
	/* Allocate the image: x pixels across by y pixels tall */
	im = gdImageCreate(x, y);

	/* Allocate the color black (red, green and blue all minimum).	
	        Since this is the first color in a new image, it will
	        be the background color. */
	background = gdImageColorAllocate(im, 245, 245, 245);

	/* Allocate the colors  (red, green and blue all maximum). */
	black = gdImageColorAllocate(im, 0  , 0  , 0  );        
	white = gdImageColorAllocate(im, 255, 255, 255);
	red   = gdImageColorAllocate(im, 255, 0  , 0  );
	green = gdImageColorAllocate(im, 0  , 255, 0  );
	blue  = gdImageColorAllocate(im, 0  , 0  , 255);
	gray  = gdImageColorAllocate(im, 180, 180, 180);

	/* draw the png boarder */
	draw_frame(im, x, y);
	                 
	/* draw vertical and horizontal title */
	draw_title(im, x, y, w_data, h_title, v_title, Logo);  
	
	assign_colors(im);  
	     
	/* draw topn data bars, show blank if no data source */
	if ( top_n > 0 )
	  { draw_data_bar(im, x, w_data, top_n);  }

	draw_data_frame(im, x, w_data);
	
	display_time_stamp(im, x, w_data, top_n,  time_stamp); 

	if ( dns==2 ) 
		display_legend(im, x, w_data, top_n);
 
	pngout = fopen(png_file, "wb"); //png_file, "wb");

	/* Output the image to the disk file in PNG format. */
	gdImagePng(im, pngout);

	/* Close the files. */
	fclose(pngout);

	/* Destroy the image in memory. */
	gdImageDestroy(im);
	exit(0);
}


/****************************
*
*  color scheme
*
****************************/

 
int 
assign_colors(gdImagePtr im) 
{
	color[0] =  gdImageColorAllocate(im,   0, 255,  51 );
	color[1] =  gdImageColorAllocate(im,  51,   0, 255 );
	color[2] =  gdImageColorAllocate(im, 255, 102, 102 );
	color[3] =  gdImageColorAllocate(im, 255,  51,   0 );
	color[4] =  gdImageColorAllocate(im, 255,   0, 255 );
	color[5] =  gdImageColorAllocate(im, 255, 153,  51 );
	color[6] =  gdImageColorAllocate(im, 204, 204,  51 );
	color[7] =  gdImageColorAllocate(im,  51, 255, 204 );
	color[8] =  gdImageColorAllocate(im, 255,   0, 153 );
	color[9] =  gdImageColorAllocate(im, 153,   0, 255 );
}


/*****************************************************
*
* draw the frame boarder of png 
*
******************************************************/

int 
draw_frame(gdImagePtr im, int x, int y)
{
	int gray, black;

	gray  = gdImageColorAllocate(im, 180,180,180);
	black =  gdImageColorAllocate(im,0,0,0);
	gdImageRectangle(im, 0, 0, x-1, y-1,gray);
	 
	gdImageLine(im, 0,y-1, x-1, y-1, gray);
	gdImageLine(im, 1,y-2, x-1, y-2, gray);
	  
	gdImageLine(im, x-1,0, x-1, y-1, gray);
	gdImageLine(im, x-2,1, x-2, y-1, gray);
}

/*******************************************************
*
*  draw 2 titles and logo
*
********************************************************/

int 
draw_title(gdImagePtr im,int x, int tall, int y, char *h_title, 
	char *v_title, char *logo)
{
	int black, gray;
    
	gray  = gdImageColorAllocate(im, 180,180,180);
	black =  gdImageColorAllocate(im,0,0,0); /* font color as black */
     
	gdImageString(im, gdFontMediumBold,
	        x/2 - (strlen(h_title) * gdFontMediumBold->w / 2),
	        10 - gdFontMediumBold->h / 2, 
	        h_title, black );

	gdImageStringUp(im, gdFontMediumBold, gdFontMediumBold->h / 2,
	        30+ y/2 + (strlen(v_title) * gdFontMediumBold->w / 2),
	        v_title,black );
    
	gdImageStringUp(im, gdFontMediumBold, 
		x-gdFontMediumBold->h-5,
	        (strlen(logo) * gdFontMediumBold->w+5),
	        logo,gray );
}

/***************************************************
*
* draw inner data frame     
*
****************************************************/


int draw_data_frame(gdImagePtr im,int x, int y)
{
	int gray, white;
	gray  = gdImageColorAllocate(im,255,255,255);
	white = gdImageColorAllocate(im,130,130,130);
	gdImageRectangle(im, Left_margin+1, Top_margin+1, x-Right_margin-1, 
		y+Top_margin-1, gray );
	gdImageRectangle(im, Left_margin, Top_margin, x-Right_margin, 
		y+Top_margin, white);
}

/***************************************************
*
*  draw bars for topn data sources
*
****************************************************/

int 
draw_data_bar(gdImagePtr im,int x, int y, int top_n)
{
	int black, i, w, space, gray, styleDashed[3], s, j;
	unsigned long scale,s5, big;
	char str[20];
	char *scale1;   
	int slen;
	float a;
	int k=0;

 
	w = (x-Left_margin-Right_margin)/top_n; 
	space = w/4; 		/* width for space between bars  */	

	black =  gdImageColorAllocate(im,0,0,0);

	/* draw dotted lines  */

	for (i=1; i<=10; i++) {
		gray =  gdImageColorAllocate(im,170,170,170);
		styleDashed[0] = gray;
		styleDashed[1] = gdTransparent;
		styleDashed[2] = gdTransparent; 
		gdImageSetStyle(im, styleDashed, 3);
		gdImageLine(im, 
		     Left_margin   , y+Top_margin-i*(y/10), 
		     x-Right_margin, y+Top_margin-i*(y/10), gdStyled);
	} 

	j=0;
	a=230;

	big = 0;
	for (i=1; i<=top_n;i++) {
		if ( big <  n_pair[i-1]->counter) {
	       		big = n_pair[i-1]->counter;
	       		j=i-1;
	  	} 
	} 

	slen = strlen(n_pair[j]->counter2);
	switch (slen) {
	       case 1 :
	       case 2 :
	       case 3 :	 
			a = (float) n_pair[j]->counter;
			scale1 = "";
			break;
	       case 4 :  
	       case 5 : 
	       case 6 : 
			a = (float)big/1000;
	        	scale1 = "K";
	        	break;
	       case 7 : 
	       case 8 : 
	       case 9 : 
			a = (float)big/1000000;
			scale1 = "M";
			break;  
	       case 10 : 
	       case 11 :
	       case 12 :
			a = (float)big/100000000;
	        	scale1 = "G";
       		default:
 			scale1 = "x";
       		
	}  

	/* draw scalar at left side */
  	sprintf(str,"%2.1f %s", a/2, scale1);
  	gdImageLine(im, Left_margin-6, y+Top_margin-5*(y/10), Left_margin, 
		y+Top_margin-5*(y)/10, black);
  	gdImageLine(im, x-Right_margin, y+Top_margin-5*(y/10), x-Right_margin+6, 
		y+Top_margin-5*(y)/10, black);
  	gdImageString(im, gdFontTiny,
	        Left_margin - (strlen(str) * gdFontTiny->w ) - 7,
	        y+Top_margin-((float)y/2) - (gdFontTiny->h)/2, 
	        str, black);
  	gdImageString(im, gdFontTiny,
	        x-Right_margin+9,
	        y+Top_margin-((float)y/2) - (gdFontTiny->h)/2, 
	        str, black);
	        
  	sprintf(str,"%2.1f %s", a, scale1);

  	gdImageLine(im, Left_margin-6, y+Top_margin-10*y/10, Left_margin, 
		y+Top_margin-10*y/10, black);

  	gdImageLine(im, x-Right_margin, y+Top_margin-10*y/10, x-Right_margin+6, 
		y+Top_margin-10*y/10 ,black);

  	gdImageString(im, gdFontTiny,
	        Left_margin - (strlen(str) * gdFontTiny->w )- 7 ,
	        Top_margin - (gdFontTiny->h/2), 
	        str, black);

  	gdImageString(im, gdFontTiny,
	        x-Right_margin+9,
	        Top_margin-(gdFontTiny->h)/2, 
	        str, black);

	sprintf(str,"%d %s", 0, scale1);
	gdImageLine(im, Left_margin-6, y+Top_margin, Left_margin, 
		y+Top_margin, black);

	gdImageLine(im, x-Right_margin, y+Top_margin, x-Right_margin+6, 
		y+Top_margin, black); 

  	gdImageString(im, gdFontTiny,
	        Left_margin - (strlen(str) * gdFontTiny->w )-7 ,
	        y+Top_margin-(gdFontTiny->h)/2, 
	        str, black);           

  	gdImageString(im, gdFontTiny,
	        x-Right_margin+9,
	        y+Top_margin-(gdFontTiny->h)/2, 
	        str, black);
  
	gdImageLine(im, Left_margin-3, y+Top_margin-9*y/10, Left_margin, 
		y+Top_margin-9*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-9*y/10, x-Right_margin+3, 
		y+Top_margin-9*y/10 , gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-8*y/10, Left_margin, 
		y+Top_margin-8*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-8*y/10, x-Right_margin+3, 
		y+Top_margin-8*y/10 ,gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-7*y/10, Left_margin, 
		y+Top_margin-7*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-7*y/10, x-Right_margin+3, 
		y+Top_margin-7*y/10 ,gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-6*y/10, Left_margin, 
		y+Top_margin-6*y/10, black);

	gdImageLine(im, x-Right_margin, y+Top_margin-6*y/10, x-Right_margin+3, 
		y+Top_margin-6*y/10 ,gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-4*y/10, Left_margin, 
		y+Top_margin-4*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-4*y/10, x-Right_margin+3, 
		y+Top_margin-4*y/10 ,gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-3*y/10, Left_margin, 
		y+Top_margin-3*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-3*y/10, x-Right_margin+3, 
		y+Top_margin-3*y/10 ,gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-2*y/10, Left_margin, 
		y+Top_margin-2*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-2*y/10, x-Right_margin+3, 
		y+Top_margin-2*y/10 ,gray);

	gdImageLine(im, Left_margin-3, y+Top_margin-1*y/10, Left_margin, 
		y+Top_margin-1*y/10, gray);

	gdImageLine(im, x-Right_margin, y+Top_margin-1*y/10, x-Right_margin+3, 
		y+Top_margin-1*y/10 ,gray);

	/*  save color in gray, and change it for topn */
	for (i=1; i<=top_n; i++) {
  
  		int gray, off;
  		gdImageFilledRectangle( im, 
	                  (Left_margin+space+(i-1)*w), 
	                  (Top_margin + y - (unsigned long )
		(( y*( float)( n_pair[i-1]->counter)/n_pair[j]->counter))) , 
	                  Left_margin+w*i, y+Top_margin,color[(i-1)%10]);

  
  		gdImageFilledRectangle( im,
	                  Left_margin+space+(i-1)*w+(w-space)/2-2,
	                  y+Top_margin+4+30*k,    
	                  Left_margin+space+(i-1)*w+(w-space)/2+2,
	                  y+Top_margin+8+30*k,
	                  color[(i-1)%10]);

  		gdImageString(im, gdFontTiny,
	        	Left_margin+space+(i-1)*w+(w-space)
				/2-((strlen(n_pair[i-1]->ipaddr) 
			* gdFontTiny->w / 2)), 
	        	y+Top_margin+10+30*k, n_pair[i-1]->ipaddr, black );
  
  		gdImageString(im, gdFontTiny,
	       		Left_margin+space+(i-1)*w+(w-space)/
				2-((strlen(n_pair[i-1]->counter2) 
				* gdFontTiny->w / 2)), 
	       		y+Top_margin+20+30*k, n_pair[i-1]->counter2, black );

	  	if ( top_n <=1 ) {  
			k=0; 
		}
	    	else if ( top_n <= 2 ) { 
			k = ( i % 2 ) ; 
		}
	        else if ( top_n <=4 ) { 
			k = ( i % 3 ); 
		}
	     	else { 
			k = ( i % 4 ); 
		}  
	}
}

int 
display_time_stamp(gdImagePtr im,int x, int y, int top_n, char *time_stamp)
{
	int x_locate, y_locate;
	int i, black, gray;

	black =  gdImageColorAllocate(im,0,0,0);
	gray  = gdImageColorAllocate(im,170,170,170);     

	switch ( top_n ) {
	case  1:
	case  2:
	         y_locate =  y+Top_margin+20+30*top_n;
	         break;
	case  3:
	case  4:
	         y_locate =  y+Top_margin+20+30*3;
	         break;
	default: y_locate =  y+Top_margin+20+30*4;
        } 	                             
  	x_locate = 15 ; //x * 1 / 3;

  	gdImageString(im, gdFontTiny,
	        5,
	        y_locate-12 - gdFontTiny->h - 5,
	        time_stamp, black );      
}

int 
display_legend(gdImagePtr im,int x, int y, int top_n)
{
	int x_locate; 
	int y_locate;
	int i; 
	int puce;
	int black; 
	int gray;
	char  s[1024];
  
	bzero(s, 1024);
	black =  gdImageColorAllocate(im,0,0,0);    
	gray  = gdImageColorAllocate(im,170,170,170); 
  
	switch ( top_n ) {
	case  1:
	case  2:   
	         y_locate =  y+Top_margin+20+30*top_n;
	         break;
	case  3:
	case  4:
	         y_locate =  y+Top_margin+20+30*3;
	         break;
	default: 
		y_locate =  y+Top_margin+20+30*4;
        } 

	x_locate = 15 ; 
	                           
	gdImageLine(im, 10, y_locate-10, x-20, y_locate-10, gray); 
 
  	for ( i = 0; i < top_n; i++ ) {

     		gdImageFilledRectangle( 
			im,
	               	x_locate,
	               	y_locate + ( i ) * 10 + 1,    
	               	x_locate + 4,
	               	y_locate + ( i ) * 10 + 5,
	               	color[(i)%10]);
     	
		sprintf(s, "%2d.  %s  %s\0", i+1,  
			dns_to_name(n_pair[i]->ipaddr),  n_pair[i]->counter2 );  

		gdImageString(
			im, 
			gdFontTiny,
	              	x_locate + 5,
	              	y_locate + ( i ) * 10,
	              	s,
	              	black ); 
	}
}

int usage(int argc, char **argv)
{
	int i;

	fprintf(stderr, "argcount %d: args: ", argc);
	for (i = 0; i < argc; i++, argv++) {
		fprintf(stderr, "%s ", *argv);
	} 
	fprintf(stderr, "\n");
	
	fprintf(stderr, "-f and -n must be set\n");
	fprintf(stderr, "-d should have an equal number of args\n");
	fprintf(stderr, "-d should be the last arg\n");
	fprintf(stderr,
	        "Usage: %s [-t string -v string -s string -l number -f string -n number] -d ip1 number1 ip2 number2 ... \n\
	        \t-t string\t\"horizontal Title\"\n\
	        \t-v string\t\"Vertical title\"\n\
	        \t-s string\t\"time Stamp\"\n\
	        \t-l number\t0\tNO dns name lookup, default\n\
	        \t         \t1\tdns name lookup on everywhere\n\
	        \t         \t2\tdns name lookup only shown as legend\n\
	        \t-n number\ttop N IPs\n\
	        \t-f string\tpng File output name, full path\n\
	        \t-d ip number\tData set : ipaddr and packets pair\n\
	        \t-h this Help\n",  
	        program_name);
	exit(1);
}

#ifdef NOTUSED
int 
is_number(char *pkt_count)
{
	int i=0;
	while (pkt_count[i]!=NULL) { 
	       if (!isdigit(pkt_count[i])) 
		  { return 0;};
	       i++;
	}
	return 1;
}
#endif

jmp_buf env;
int alarmset = 0;

#define MAX_SIZE 1024

char *dns_to_name(char *ipstring)
{
	extern void onalarm();
	extern int errno;
	char t[MAX_SIZE];
	char *t2= "->";
	char left[MAX_SIZE],  ip1[MAX_SIZE], port1[MAX_SIZE];
	char right[MAX_SIZE], ip2[MAX_SIZE], port2[MAX_SIZE];
	char name1[MAX_SIZE], name2[MAX_SIZE];
	struct hostent hent;
	uint32_t ipaddr2;
	char *n1;
	int l, total_length, i;
	struct hostent  * p1 = (struct hostent *) &hent;
	struct in_addr  * p2 = (struct in_addr *) &ipaddr2;
	int rc;

	errno = 0;

	/* copy ipstring to t, limit size to MAX_SIZE - 1
	*/
	total_length = strlen(ipstring);
	if (total_length >= MAX_SIZE)
		total_length = MAX_SIZE - 1;
	memcpy(t, ipstring, total_length);
	t[total_length] = '\0';

	/* look for -> or - in order to divide the string into 2 
	*/
	l = strcspn ( t, t2 );

	/* bail if we do not find two targets ->
	*/
	if ( l == strlen(t)) 
		return (ipstring);

	/* copy left hand portion out of t, the target
	*/
	memcpy( left, t, l);
	left[l]='\0';

	/* look for possible port number in lhs
	 * n1 is probably .port number.
	 * left is ip.port
	 * result here is either last . or null
 	 * at end of string.
	 */
	n1 = strrchr( left, '.' );

	if ( n1==NULL || (strcmp(n1, left) == 0)) {
		/* copy left into name1 (ip)
		*/
		strcpy(name1, left);
		/* no port found
		*/
	     	port1[0]='\0';
	}
  	else {
		memcpy ( ip1, left, n1-left );
		ip1[n1 - left]='\0';
		memcpy ( port1, n1+1, strlen(left)-((n1+1)-left) );
		port1[strlen(left)-((n1+1)-left)]='\0';      
		i = inet_aton(ip1, p2);
		/* alarm mechanism here allows us to
	         * escape from gethostbyaddr ... within a controlled
		 * time from here.  drawtopn cannot afford to
		 * wait around long.  Note that gethostbyaddr is 	
		 * likely not interruptible in terms of signal setup.
		 */
		strcpy(name1, ip1);
		rc = setjmp(env);
		p1 = NULL;
		if (rc == 1) {
			goto moveon;
		} 
		signal(SIGALRM, onalarm);
		alarm(DNSTIMEOUT);
		alarmset = 0;
		p1 = gethostbyaddr(( char * ) p2, 4, AF_INET);
moveon:
		alarm(0);
		if (alarmset==0)  {
			if (p1) {
				strcpy(name1, p1->h_name);
			}
		} 
     	} 

	memcpy ( right, t+l+2, total_length-(l+2) );
	right[total_length-(l+2)]='\0';

	n1 = strrchr ( right, '.');
	if ( n1==NULL || (n1-right+1) == strlen(right)) {
       		strcpy(name2, right);
       		port2[0]='\0';
     	}
  	else {
		memcpy( ip2, right, n1 - right);
		ip2[n1 - right] = '\0';
       		memcpy ( port2, n1+1, strlen(right)-((n1+1)-right));
       		port2[strlen(right)-((n1+1)-right)]= '\0';
       		i = inet_aton(ip2, p2);
		strcpy(name2, ip2);
		signal(SIGALRM, onalarm);
		alarm(DNSTIMEOUT);
		alarmset = 0;
		p1 = NULL;
		rc = setjmp(env);
		if (rc == 1) {
			goto moveon2;
		} 
       		p1 = gethostbyaddr(( char * ) p2, 4, AF_INET);
moveon2:
		alarm(0);
		if (alarmset == 0)  {
			if (p1) {
				strcpy(name2, p1->h_name);
			}
		} 
	}

	bzero(result, 1024);
	sprintf(result, "%s.%s->%s.%s\0", name1, port1, name2, port2);
	return result;
}	                

void
onalarm()
{
	alarmset = 1;
	longjmp(env, 1);
}
