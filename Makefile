# Generated automatically from Makefile.in by configure.
CC = gcc
# CFLAGS = -g -O4 -Wall -I.
CFLAGS = -g -O2 -pthread

LIBS = -lnsl -lm -lpthread

ARCH:=$(shell uname -m)

SRC=src/
BIN=bin/$(ARCH)/

EOBJS = $(SRC)abw_rfl.c
FOBJS = $(SRC)abw.c $(SRC)msdelay.c $(SRC)do_client.c $(SRC)abw6.c

OBJS = $(FOBJS) $(EOBJS)

TARGETS = $(BIN)abw_rfl $(BIN)abing

all: $(TARGETS)

bin_arch_dir:
	@if [ ! -d "$(BIN)" ]; then mkdir -p $(BIN); fi;

$(BIN)abw_rfl: bin_arch_dir $(SRC)abw_rfl.c $(SRC)abw.h
	echo $(BIN)
	$(CC) $(CFLAGS) $(SRC)abw_rfl.c -o $(BIN)abw_rfl $(LIBS)

$(BIN)abing: bin_arch_dir $(SRC)abw.c $(SRC)msdelay.c $(SRC)abw.h $(SRC)do_client.c  $(SRC)abw6.c
	echo $(BIN)
	$(CC) $(CFLAGS) $(FOBJS) -o $(BIN)abing $(LIBS)

clean:
	rm -f ${TARGETS}
	rm -f config.cache config.log config.status

distclean: clean
	rm -f Makefile configure config.h

configure: configure.in
	autoconf

Makefile: Makefile.in configure
	./configure
