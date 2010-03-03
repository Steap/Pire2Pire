CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_POSIX_SOURCE -D_BSD_SOURCE
LDFLAGS=
EXECUTABLE=p2p

USER_SOURCES=   user/interface.c        \
		user/do_connect.c       \
		user/do_download.c      \
		user/do_get.c           \
		user/do_help.c          \
		user/do_info.c          \
		user/do_list.c          \
		user/do_raw.c           \
		user/do_set.c           \
		user/do_upload.c

CLIENT_SOURCES= client/interface.c      \
                client/handle_client.c  \
                client/handle_get.c

UTIL_SOURCES=   util/logger_util.c      \
                util/socket_util.c      \
                util/string_util.c      \
		util/cmd.c

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
