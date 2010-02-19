CC=gcc
CFLAGS=-Wextra -Wall -g
LDFLAGS=

all: server client

server: server.o util.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o server server.o util.o

client: client.o util.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o client client.o util.o

server.o: server.c util.c util.h
	$(CC) $(CFLAGS) -c -o server.o server.c

client.o: client.c util.c util.h
	$(CC) $(CFLAGS) -c -o client.o client.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c -o util.o util.c

clean: 
	rm -f server client *.o
