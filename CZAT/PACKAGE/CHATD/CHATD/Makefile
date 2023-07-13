#
# Make file for Chat Daemon Program
# Chat project v 1.0
#
# 
# Coded by Pawel Furtak <crashev@ebox.pl>      All right reserved!!! 
# 
#
#

TARGET=chatd
VERSION="1.0 BETA"

COM=gcc
SRC=chat.c
TARGET=chatd
CFLAGS= -O3 -Wall -ggdb
FILES=funkcje.o chatd.o

default:
	@echo "--------------------------------------------------------------"
	@echo "               WEB-Kafejka Daemon Version 1.0"
	@echo "                   All rights reserved!!!!!!"
	@echo "Coded By Pawel Furtak <crashev@ebox.pl>"
	@echo "Read README before starting make script!!!"
	@echo "--------------------------------------------------------------"
	make chatd


${TARGET}: ${FILES}
	gcc ${FILES} -s -o $@

clean:
	rm -f *.o ${TARGET}