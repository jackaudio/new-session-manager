# -*- mode: makefile; -*-

FL/.sources:
	@ echo "FL_SRCS=\\" > FL/.sources
	@ git ls-files 'FL/*.C' | tr '\n' ' ' >> FL/.sources
	@ git ls-files 'FL/*.fl' | tr '\n' ' ' >> FL/.sources

include FL/.sources

FL_SRCS:=$(FL_SRCS:.fl=.C)
FL_OBJS:=$(FL_SRCS:.C=.o)

all: FL/libfl_widgets.a

FL/libfl_widgets.a: $(FL_OBJS)
	@ ar rcs $@ $(FL_OBJS)

.PHONEY: FL
FL: FL/libfl_widgets.a

FL_clean:
	rm -f $(FL_OBJS) FL/libfl_widgets.a
