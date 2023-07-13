# Makefile

CFLAGS=-Wall -O2 -ggdb -I../common -Iinclude/
LIBS=-lssl -lcrypto -lz -lpcap
SRCS=client.c log.c main.c server.c mywatch.c tevents.c cprotocol.c iptables.c ../common/common.c
OBJECTS=client.o log.o main.o server.o tevents.o cprotocol.o iptables.o mywatch.o ../common/common.o
BIN=APclientd

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

