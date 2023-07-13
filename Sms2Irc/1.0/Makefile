#
# Make file for Sms2Irc - v3.0
#
# By crashev '2001
#
#

VERSION="3.0"

COM=gcc
SRC-SERWER=sms2irc.c
SRC-KLIENT=client.c
TAR-SERWER=sms2irc
TAR-KLIENT=client

CFLAGS= -O3 -fomit-frame-pointer -Wall

default:
	@echo "--------------------------------------------------------------"
	@echo "                     SmS2Irc Version 3.0                      "
	@echo "           Coded By crashev '2001 <crashev@ebox.pl>           "
	@echo "          Read README before starting make script!!!          "
	@echo "--------------------------------------------------------------"
	make all

all:
	make serwer
	make klient

serwer: 
	gcc ${SRC-SERWER} ${CFLAGS} -o ${TAR-SERWER}

klient: 
	gcc ${SRC-KLIENT} ${CFLAGS} -o ${TAR-KLIENT}

clean:
	rm -rf ${TAR-KLIENT} ${TAR-SERWER}
