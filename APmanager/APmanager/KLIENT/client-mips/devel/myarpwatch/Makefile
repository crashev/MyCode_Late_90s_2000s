# Makefile

CFLAGS=-Wall -ggdb
LIBS=-lpcap
SRCS=arpfuncs.c arpwatch.c linux_arp.c 
OBJECTS=arpfuncs.o arpwatch.o linux_arp.o 
BIN=arpwatch

all: $(BIN)
$(SRCS):
	$(GET) $@
	$(CC) $(CFLAGS) -c $(SRCS)

$(BIN): $(OBJECTS)
	/bin/sh mkrelease.sh $(RELEASE) release.h
	gcc $(CFLAGS) $(LIBS) -o $(BIN) $(OBJECTS)
	@rm -f *.o
	@echo "Buiding of $(BIN) done"
#	@echo "Remember to set LD_LIBRARY_PATH"
	@ls -l $(BIN)

clean:
	rm -f *.o $(BIN) *.core
