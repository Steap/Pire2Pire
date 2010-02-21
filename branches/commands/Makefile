CC=gcc
CFLAGS=-Wextra -Wall -g
LDFLAGS=

main: main.o cmd.o
	$(CC) $(LDFLAGS) -o main main.o cmd.o

main.o: main.c cmd.h
	$(CC) $(CFLAGS) -c -o main.o main.c

cmd.o: cmd.c cmd.h
	$(CC) $(CFLAGS) -c -o cmd.o cmd.c

clean: 
	rm -f *.o
