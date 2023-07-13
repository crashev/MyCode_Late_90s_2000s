#
# makes drawtopn binary program from C code.
#
# Needs:
# GD library and related PNG libraries
#       e.g., gd version 1.7.3`
#
# See http://www.boutell.com/gd/ for the GD library
# See http://www.gzip.org for libc.  
#
# note:
# gd include files are assumed to be placed in /usr/local/include
# gd libs in /usr/local/lib
#
CFLAGS=-O 
#CFLAGS=-g
#
# for Linux
#LIBS=/usr/lib/libgd.a /usr/lib/libpng.a /usr/lib/libz.a
# libz.a is in /usr/lib on freebsd, may need tweaking on linux
# other libs in local after port install
LIBS=/usr/local/lib/libgd.a /usr/local/lib/libpng.a  /usr/lib/libz.a
drawtopn: drawtopn.c
	gcc ${CFLAGS} drawtopn.c -o  drawtopn -I/usr/local/include -lm $(LIBS)

install: drawtopn
	cp drawtopn /usr/local/bin
	chmod +x /usr/local/bin/drawtopn

clean:
	rm -fr drawtopn
