# -*- mode: makefile; -*-

VERSION := 1.1.0
PACKAGE := FL

all: FL

SRCS := $(wildcard FL/*.C FL/*.fl)

SRCS:=$(SRCS:.fl=.C)
SRCS:=$(sort $(SRCS))
OBJS:=$(SRCS:.C=.o)

all: FL/libfl_widgets.a

FL/libfl_widgets.a: $(OBJS)
	@ ar rcs $@ $(OBJS)

.PHONEY: FL
FL: FL/libfl_widgets.a

clean:
	rm -f $(OBJS) FL/libfl_widgets.a .deps

install:
	@