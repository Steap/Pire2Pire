CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_POSIX_SOURCE
LDFLAGS=

all: clear p2p

clear:
	clear

p2p: p2p.o user_interface.o client_interface.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

p2p.o: p2p.c
	$(CC) $(CFLAGS) -c -o $@ $<

.o:.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f p2p *.o
