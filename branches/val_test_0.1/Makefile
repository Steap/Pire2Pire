CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_POSIX_SOURCE
LDFLAGS=

all: clear p2p

clear:
	clear

p2p: p2p.o util.o ui.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

p2p.o: p2p.c util.h cmd.h
	$(CC) $(CFLAGS) -c -o $@ $<

.o:.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f server *.o
