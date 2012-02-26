# -*- mode: makefile; -*-

all: Timeline

Timeline_VERSION := 0.5.0

Timeline_SRCS := $(wildcard src/*.C src/*.fl src/Engine/*.C)

Timeline_SRCS:=$(Timeline_SRCS:.fl=.C)
Timeline_SRCS:=$(sort $(Timeline_SRCS))
Timeline_OBJS:=$(Timeline_SRCS:.C=.o)

Timeline_LIBS := $(FLTK_LIBS) $(JACK_LIBS) $(SNDFILE_LIBS) $(LIBLO_LIBS) $(SIGCPP_LIBS)

Timeline_CFLAGS := $(SNDFILE_CFLAGS) $(FLTK_CFLAGS) $(JACK_CFLAGS) $(SIGCPP_CFLAGS)

src/timeline: $(Timeline_OBJS) FL/libfl_widgets.a nonlib/libnonlib.a
	@ echo -n Linking timeline...
	@ $(CXX) $(CXXFLAGS) $(INCLUDES) $(Timeline_LIBS)  $(Timeline_OBJS) -o $@ -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

Timeline: src/timeline

Timeline_clean:
	rm -f $(Timeline_OBJS) Timeline/timeline
