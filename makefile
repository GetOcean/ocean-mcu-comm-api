CC=gcc
CFLAGS=-I
MKDIR_P=mkdir -p ./tmp
RESTART=/etc/init.d/mcutool restart

all: serial mcu cleanup

serial: ./src/icracked.mcu.serial.c
	$(MKDIR_P)
	$(CC) -c ./src/icracked.mcu.util.c -o ./tmp/icracked.mcu.util.o
	$(CC) -c ./src/icracked.mcu.serial.c -o ./tmp/icracked.mcu.serial.o -I.
	$(CC) ./tmp/icracked.mcu.serial.o ./tmp/icracked.mcu.util.o -o /usr/local/bin/icracked.mcu.serial -lrt

client: ./src/icracked.mcu.client.c
	$(MKDIR_P)
	$(CC) -c ./src/icracked.mcu.util.c -o ./tmp/icracked.mcu.util.o
	$(CC) -c ./src/icracked.mcu.client.c -o ./tmp/icracked.mcu.client.o -I.
	$(CC) ./tmp/icracked.mcu.client.o ./tmp/icracked.mcu.util.o -o icracked.mcu.client

mcu: ./src/mcu.c
	$(MKDIR_P)
	$(CC) -c ./src/icracked.mcu.util.c -o ./tmp/icracked.mcu.util.o
	$(CC) -c ./src/icracked.mcu.client.c -o ./tmp/icracked.mcu.client.o
	$(CC) -c ./src/mcu.c -o ./tmp/mcu.o -lm -I.
	$(CC) ./tmp/mcu.o ./tmp/icracked.mcu.util.o ./tmp/icracked.mcu.client.o -o /usr/local/bin/mcu -lm

restart:

cleanup:
	$(RESTART)
	rm -rf $(CURDIR)
