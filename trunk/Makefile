CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE=199506
LDFLAGS=-lpthread
CLIENT_EXEC=bin/client
DAEMON_EXEC=bin/daemon

CLIENT_SOURCES  =   client/main.c           \
                    client/send_cmd.c       \
                    client/recv_resp.c      \
                    client/util/prompt.c

DAEMON_SOURCES  =   daemon/main.c                                       \
		    daemon/conf.c                                       \
		    daemon/client_handler.c                             \
                    daemon/daemon_handler.c                             \
		    daemon/callback_argument.c                          \
		    daemon/client.c                                     \
                    daemon/daemon.c                                     \
		    daemon/client_request.c                             \
                    daemon/daemon_request.c                             \
                    daemon/util/cmd.c                                   \
                    daemon/util/socket.c                                \
		    daemon/client_requests/client_request_connect.c     \
		    daemon/client_requests/client_request_get.c         \
                    daemon/client_requests/client_request_help.c        \
		    daemon/client_requests/client_request_info.c        \
                    daemon/client_requests/client_request_list.c        \
		    daemon/client_requests/client_request_set.c         \
		    daemon/client_requests/client_request_unknown.c     \
                    daemon/daemon_requests/daemon_request_unknown.c

UTIL_SOURCES    =   util/logger.c   \
                    util/string.c   \
                    util/md5/md5.c  \
                    util/md5/md5_algorithm.c

CLIENT_OBJ  =   $(CLIENT_SOURCES:.c=.o)
DAEMON_OBJ  =   $(DAEMON_SOURCES:.c=.o)
UTIL_OBJ    =   $(UTIL_SOURCES:.c=.o)

.PHONY: all clean clear

all: clear client daemon

client: $(CLIENT_SOURCES) $(UTIL_SOURCES) $(CLIENT_EXEC)

daemon: $(DAEMON_SOURCES) $(UTIL_SOURCES) $(DAEMON_EXEC)

$(CLIENT_EXEC):	$(CLIENT_OBJ) $(UTIL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(DAEMON_EXEC):	$(DAEMON_OBJ) $(UTIL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(DAEMON_OBJ) $(UTIL_OBJ)

clear:
	clear
