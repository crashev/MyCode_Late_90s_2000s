# Makefile

CFLAGS=-Wall -O2 -ggdb -I../common -I/usr/local/include/ -L/usr/local/lib/ 
LIBS=-lpq -lssl -lcrypto -lrrd -lz -lpthread
SRCS=server.c main.c protocol.c log.c aprotocol.c config.c dbmod.c ../common/common.c
OBJECTS=server.o protocol.o log.o aprotocol.o main.o config.o dbmod.o ../common/common.o
BIN=APcenterd

all: $(BIN)
$(SRCS):
	$(GET) $@
	$(CC) $(CFLAGS) -c $(SRCS)

$(BIN): $(OBJECTS)
	/usr/local/bin/bash mkrelease.sh $(RELEASE) release.h
	gcc $(CFLAGS) $(LIBS) -o $(BIN) $(OBJECTS)
	@rm -f *.o
	@echo "Buiding of $(BIN) done"
#	@echo "Remember to set LD_LIBRARY_PATH"
	@ls -l $(BIN)

clean:
	rm -f *.o $(BIN) *.core core.*
