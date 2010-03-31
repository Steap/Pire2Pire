CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE=199506
LDFLAGS=-lpthread

HASCOLOR = $(shell if test `which colorgcc`; then echo true; else echo false; fi)
ifeq ($(HASCOLOR),true)
CC := $(addprefix color, $(CC)) 
endif

CLIENT_EXEC=bin/client
DAEMON_EXEC=bin/daemon

CLIENT_SOURCES=\
	client/main.c		\
	client/recv_resp.c	\
	client/send_cmd.c	\
	client/util/prompt.c

DAEMON_SOURCES=\
	daemon/callback_argument.c			                \
	daemon/client.c					                \
	daemon/client_handler.c				                \
	daemon/client_requests/client_request_connect.c	                \
	daemon/client_requests/client_request_help.c	                \
	daemon/client_requests/client_request_info.c	                \
	daemon/client_requests/client_request_list.c	                \
        daemon/client_requests/client_request_raw.c                     \
	daemon/client_requests/client_request_set.c	                \
	daemon/client_requests/client_request_unknown.c    	        \
	daemon/client_request.c				                \
	daemon/conf.c					                \
	daemon/daemon.c					                \
	daemon/daemon_handler.c				                \
	daemon/daemon_requests/daemon_request_error.c	                \
	daemon/daemon_requests/daemon_request_file.c	                \
	daemon/daemon_requests/daemon_request_list.c	                \
        daemon/daemon_requests/daemon_request_neighbour.c               \
        daemon/daemon_requests/daemon_request_neighbourhood.c           \
	daemon/daemon_requests/daemon_request_unknown.c	                \
	daemon/daemon_request.c				                \
	daemon/main.c					                \
	daemon/file_cache.c				                \
	daemon/util/cmd.c				                \
	daemon/util/socket.c

UTIL_SOURCES=\
	util/logger.c           \
	util/md5/md5.c          \
	util/md5/md5_algorithm.c\
	util/string.c

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
