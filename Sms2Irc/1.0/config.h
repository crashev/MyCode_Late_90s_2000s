#include <stdio.h>
#include <unistd.h> // read/write
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>     // sockaddr_un unix(7)
#include <stdlib.h>     // atoi


#define SOCK_PATH "/home/pawq/sms2irc/.temp"
#define LOGS_PATH "/home/pawq/sms2irc/logs/"
#define MAXLINES 5

struct 
{
    char *name;
    int port;
} ircserver[] ={
                { "warszawa.irc.pl",6667 },
                { "krakow.irc.pl",6667 },
                { "wroclaw.irc.pl",6667 },
                { "lublin.irc.pl",6667 },
                { 0 }
};
// Ksiazka adresowa :>
struct 
{
  int numer;
  char *ksywa;
} adres[] ={
        { 603364824, "owczi" },
        { 603514608, "pol" },
        { 609744235, "Ravi-Disasta" },
        { 608109019, "crashev"},
	{ 0 }
};
