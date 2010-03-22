CC=gcc
CFLAGS=-Wextra -Wall -g -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE=199506

HASCOLOR = $(shell if test `which colorgcc`; then echo true; else echo false; fi)
ifeq ($(HASCOLOR),true)
CC := $(addprefix color, $(CC)) 
endif

EXEC=parsebin

SOURCES=\
	main.c		    \
	cmd_parser.c

OBJ  =   $(SOURCES:.c=.o)

.PHONY: all clean clear

all: clear parsebin

$(EXEC):	$(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ)

clear:
	clear
