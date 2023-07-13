TARGET=wpg
CC=gcc
CFLAGS=-O2

all: $(TARGET)
$(TARGET): main.o conf.o magic.o
	$(CC) main.o conf.o magic.o -o $(TARGET) $(CFLAGS)
	strip $(TARGET)

magic.o:
	$(CC) magic.c -c $(CFLAGS)

conf.o:
	$(CC) conf.c -c $(CFLAGS)

main.o:
	$(CC) main.c -c $(CFLAGS)

clean:
	rm -f *.o $(TARGET)
