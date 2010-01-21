# -*- mode: makefile; -*-

nonlib_SRCS := $(wildcard nonlib/*.C nonlib/JACK/*.C nonlib/LASH/*.C nonlib/OSC/*.C nonlib/NSM/*.C)

nonlib_SRCS:=$(sort $(nonlib_SRCS))
nonlib_OBJS:=$(nonlib_SRCS:.C=.o)

all: nonlib/libnonlib.a

nonlib/libnonlib.a: $(nonlib_OBJS)
	@ ar rcs $@ $(nonlib_OBJS)

.PHONEY: nonlib
nonlib: nonlib/libnonlib.a

nonlib_clean:
	rm -f $(nonlib_OBJS) nonlib/libnonlib.a
