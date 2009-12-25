# -*- mode: makefile; -*-

all: Mixer

Mixer_SRCS := $(wildcard Mixer/*.C Mixer/*.fl Mixer/Engine/*.C)
Mixer_SRCS += util/debug.C util/Thread.C util/file.C

Mixer_SRCS:=$(Mixer_SRCS:.fl=.C)
Mixer_SRCS:=$(sort $(Mixer_SRCS))
Mixer_OBJS:=$(Mixer_SRCS:.C=.o)

Mixer_LIBS := $(FLTK_LIBS) $(JACK_LIBS) $(LASH_LIBS) -llrdf

Mixer/mixer: $(Mixer_OBJS) FL
	@ echo -n Linking mixer...
	@ $(CXX) $(CXXFLAGS) $(Mixer_LIBS) $(Mixer_OBJS) -o $@ -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

Mixer: Mixer/mixer

Mixer_clean:
	rm -f $(Mixer_OBJS) Mixer/mixer
