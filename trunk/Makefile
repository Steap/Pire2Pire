CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_BSD_SOURCE
LDFLAGS=
CLIENT_EXEC=bin/client
DAEMON_EXEC=bin/daemon

CLIENT_SOURCES  =
DAEMON_SOURCES  =   daemon/main.c
UTIL_SOURCES    =   util/logger.c

CLIENT_OBJ  =   $(CLIENT_SOURCES:.c=.o)
DAEMON_OBJ  =   $(DAEMON_SOURCES:.c=.o)
UTIL_OBJ    =   $(UTIL_SOURCES:.c=.o)

.PHONY: all clean clear

# FIXME: add client when there will be sources...
all: clear daemon

client: $(CLIENT_SOURCES) $(CLIENT_EXEC)

daemon: $(DAEMON_SOURCES) $(UTIL_SOURCES) $(DAEMON_EXEC)

$(CLIENT_EXEC):	$(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(DAEMON_EXEC):	$(DAEMON_OBJ) $(UTIL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(DAEMON_OBJ) $(UTIL_OBJ)

clear:
	clear
