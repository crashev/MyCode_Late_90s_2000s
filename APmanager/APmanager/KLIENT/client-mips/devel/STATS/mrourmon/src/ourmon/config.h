/* config.h.  Generated automatically by configure.  */
/*
 *	Copyright (c) 1997 JSC Rinet, Novosibirsk, Russia
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 * Redistribution in binary form may occur without any restrictions.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' WITHOUT ANY WARRANTIES OF ANY KIND.
 */

/*
 * config.h.in -- template for config.h
 * Process this file with `./configure' to produce config.h
 * 	remember to edit config.h.in, not config.h
 */

/*
 * The curses library definition.
 */


/* #define SHOW 1 
*/
/* Define if you have the slang curses library (-lslang) */
/* #undef HAVE_SLCURSES */

/* Define if you have the ncurses library (-lncurses) */
#define HAVE_NCURSES 1

/* Define if you have the curses library (-lcurses) */
/* #undef HAVE_CURSES */

/* Define if your curses has a colors functions such as
   has_colors(), start_color(), init_pair(), attron(), attrset() */
#define HAVE_HAS_COLORS 1

/* Define if your curses has the bkgd() function */
#define	HAVE_BKGD 1

/* Define if your curses has the wbkgd() function */
#define	HAVE_WBKGD 1

/* Define if your curses has the resizeterm() function */
#define HAVE_RESIZETERM 1

/*
 * The system functions definition.
 */

/* Define if you have the select() function */
#define HAVE_SELECT 1

/* Define if you have the siginterrupt() function
   if defined than BSD else SYSV signal semantics */
#define HAVE_SIGINTERRUPT 1

/* Define if you have the strcasecmp() function */
#define HAVE_STRCASECMP 1

/*
 * The system header definition.
 */

/* Define if you can safely include both <sys/time.h> and <time.h> */
#define TIME_WITH_SYS_TIME 1

/* Define if you have the <net/slip.h> header file */
#define HAVE_NET_SLIP_H 1

/* Define if you have the <sys/ioctl.h> header file */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/mbuf.h> header file */
#define HAVE_SYS_MBUF_H 1

/*
 * Miscellaneous definition.
 */

/* Define if you have the u_int32_t typedef */
/* #undef u_int32_t */

/* Define if unaligned memory access failed */
/* #undef	LBL_ALIGN */

/* Define if words are stored with the most significant byte first */
/* #undef	WORDS_BIGENDIAN */

/* fake for osf */
/* #undef	__STDC__ */

/*
 * How to get ARP table entries.
 */

/* Define if your system has RTF_LLINFO available through sysctl call */
#define	HAVE_RTF_LLINFO 1

/* Define if your system has /proc/net/arp file */
/* #undef	HAVE_PROC_NET_ARP */
