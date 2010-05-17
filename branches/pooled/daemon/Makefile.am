bin_PROGRAMS = foobar
foobar_SOURCES= client.c \
client_handler.c \
client_request.c \
conf.c \
daemon.c \
daemon_handler.c \
daemon_request.c \
dl_file.c \
file_cache.c \
main.c \
thread_pool.c \
client_requests/client_request_connect.c\
client_requests/client_request_download.c \
client_requests/client_request_get.c \
client_requests/client_request_help.c \
client_requests/client_request_info.c \
client_requests/client_request_list.c \
client_requests/client_request_raw.c \
client_requests/client_request_set.c \
client_requests/client_request_unknown.c \
client_requests/client_request_upload.c \
daemon_requests/daemon_request_error.c \
daemon_requests/daemon_request_file.c \
daemon_requests/daemon_request_get.c \
daemon_requests/daemon_request_list.c \
daemon_requests/daemon_request_neighbour.c \
daemon_requests/daemon_request_neighbourhood.c \
daemon_requests/daemon_request_ready.c \
daemon_requests/daemon_request_unknown.c \
util/cmd.c \
util/cmd_parser.c \
util/socket.c \
util/logger.c \
util/string.c  \
util/md5/md5.c \
util/md5/md5_algorithm.c
foobar_LDFLAGS = -lpthread
SUBDIRS= client_requests daemon_requests util util/md5
AUTOMAKE_OPTIONS = subdir-objects
