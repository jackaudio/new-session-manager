# -*- mode: makefile; -*-

Timeline_VERSION := 0.5.0

Timeline/.sources:
	@ echo "Timeline_SRCS=\\" > Timeline/.sources
	@ git ls-files 'Timeline/*.C' | tr '\n' ' ' >> Timeline/.sources
	@ git ls-files 'Timeline/*.fl' | tr '\n' ' ' >> Timeline/.sources

include Timeline/.sources

Timeline_SRCS += util/debug.C

Timeline_SRCS:=$(Timeline_SRCS:.fl=.C)
Timeline_OBJS:=$(Timeline_SRCS:.C=.o)

Timeline_LIBS := $(FLTK_LIBS) $(JACK_LIBS) $(SNDFILE_LIBS) $(LASH_LIBS)

Timeline/timeline: $(Timeline_OBJS) FL
	@ echo -n Linking timeline...
	@ $(CXX) $(CXXFLAGS) $(INCLUDES) $(Timeline_LIBS)  $(Timeline_OBJS) -o $@ -LFL -lfl_widgets && echo $(DONE)

Timeline: Timeline/timeline

Timeline_clean:
	rm -f $(Timeline_OBJS) Timeline/timeline
