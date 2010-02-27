CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_POSIX_SOURCE
LDFLAGS=
EXECUTABLE=p2p

USER_SOURCES=   user/interface.c        \
                user/do_get.c           \
                user/do_help.c

CLIENT_SOURCES= client/interface.c      \
                client/handle_client.c  \
                client/handle_get.c

UTIL_SOURCES=   util/logger_util.c      \
                util/socket_util.c      \
                util/string_util.c

SOURCES=        p2p.c                   \
                $(USER_SOURCES)         \
                $(CLIENT_SOURCES)       \
                $(UTIL_SOURCES)

OBJECTS=$(SOURCES:.c=.o)

.PHONY: all clean clear

all: clear $(SOURCES) $(EXECUTABLE)

clean: 
	rm -f $(EXECUTABLE) $(OBJECTS)

clear:
	clear

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
