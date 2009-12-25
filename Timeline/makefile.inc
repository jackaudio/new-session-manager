# -*- mode: makefile; -*-

all: Timeline

Timeline_VERSION := 0.5.0

Timeline_SRCS := $(wildcard Timeline/*.C Timeline/*.fl Timeline/Engine/*.C)

Timeline_SRCS += util/debug.C util/Thread.C util/file.C

Timeline_SRCS:=$(Timeline_SRCS:.fl=.C)
Timeline_SRCS:=$(sort $(Timeline_SRCS))
Timeline_OBJS:=$(Timeline_SRCS:.C=.o)

Timeline_LIBS := $(FLTK_LIBS) $(JACK_LIBS) $(SNDFILE_LIBS) $(LASH_LIBS)

Timeline/timeline: $(Timeline_OBJS) FL
	@ echo -n Linking timeline...
	@ $(CXX) $(CXXFLAGS) $(INCLUDES) $(Timeline_LIBS)  $(Timeline_OBJS) -o $@ -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

Timeline: Timeline/timeline

Timeline_clean:
	rm -f $(Timeline_OBJS) Timeline/timeline
