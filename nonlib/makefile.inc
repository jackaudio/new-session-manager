# -*- mode: makefile; -*-

VERSION := 1.1.0
PACKAGE := NONLIB

SRCS := $(wildcard nonlib/*.C nonlib/JACK/*.C nonlib/LASH/*.C nonlib/OSC/*.C nonlib/NSM/*.C)

SRCS:=$(sort $(SRCS))
OBJS:=$(SRCS:.C=.o)

CFLAGS := $(SIGCPP_CFLAGS) $(LIBLO_CFLAGS) $(JACK_CFLAGS)

all: nonlib/libnonlib.a

nonlib/libnonlib.a: $(OBJS)
	@ ar rcs $@ $(OBJS)

.PHONEY: nonlib
nonlib: nonlib/libnonlib.a

clean:
	rm -f $(OBJS) nonlib/libnonlib.a .deps

install:
	@