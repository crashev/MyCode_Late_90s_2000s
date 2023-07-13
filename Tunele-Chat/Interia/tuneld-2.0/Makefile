#
# Make file
# Interia Tunnel 
# Coded by Pawel Furtak <crashev@ebox.pl>      All right reserved!!! 
#

TARGET=tuneld
VERSION="1.0 BETA"

COM=gcc
SRC=main.c
TARGET=tuneld
CFLAGS=-lz -ggdb -Wall
FILES=net.c czat.c dec.c debug.c

default:
	@echo "--------------------------------------------------------------"
	@echo "              TUNELD  All rights reserved!!!!!!"
	@echo "              Coded By Pawel Furtak <crashev@ebox.pl>"
	@echo "--------------------------------------------------------------"
	make tuneld


${TARGET}:
	gcc ${CFLAGS} ${SRC} ${FILES} -o $@
	strip ${TARGET}

clean:
	rm -f *.o ${TARGET}