#
# Make file for Chat Daemon Program
# Chat project v 1.0
#
# WORKS fine with kafejka.php3 project and kafejka.html(java project)
#
# By Crashkiller '2000
#
#

TARGET=chatd
VERSION="1.0 BETA"

COM=gcc
SRC=chat.c
TARGET=chatd
CFLAGS= -O3 -Wall
FILES=funkcje.o chatd.o

default:
	@echo "--------------------------------------------------------------"
	@echo "               WEB-Kafejka Daemon Version 1.0"
	@echo "Works with kafejka.php3 project and kafejka.html(java project)"
	@echo "Coded By crashev '2000 <crashev@ebox.pl>"
	@echo "Read README before starting make script!!!"
	@echo "--------------------------------------------------------------"
	make chatd


${TARGET}: ${FILES}
	gcc ${FILES} -s -o $@

clean:
	rm -f *.o ${TARGET}