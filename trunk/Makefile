CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 
LDFLAGS=
EXECUTABLE=p2p

DAEMON_SOURCES= daemon/main.c

UTIL_SOURCES=   util/logger.c


daemon: daemon/main.o util/logger.o
	$(CC) $(LDFLAGS) -o bin/daemon daemon/main.o util/logger.o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f *.o

