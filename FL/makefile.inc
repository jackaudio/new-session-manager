# -*- mode: makefile; -*-

all: FL

FL_SRCS := $(wildcard FL/*.C FL/*.fl)

FL_SRCS:=$(FL_SRCS:.fl=.C)
FL_SRCS:=$(sort $(FL_SRCS))
FL_OBJS:=$(FL_SRCS:.C=.o)

all: FL/libfl_widgets.a

FL/libfl_widgets.a: $(FL_OBJS)
	@ ar rcs $@ $(FL_OBJS)

.PHONEY: FL
FL: FL/libfl_widgets.a

FL_clean:
	rm -f $(FL_OBJS) FL/libfl_widgets.a
