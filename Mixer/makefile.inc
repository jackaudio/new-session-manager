# -*- mode: makefile; -*-

Mixer/.sources:
	@ echo "Mixer_SRCS=\\" > Mixer/.sources
	@ git ls-files 'Mixer/*.C' | tr '\n' ' ' >> Mixer/.sources
	@ git ls-files 'Mixer/*.fl' | tr '\n' ' ' >> Mixer/.sources

include Mixer/.sources

Mixer_SRCS += util/debug.C

Mixer_SRCS:=$(Mixer_SRCS:.fl=.C)
Mixer_OBJS:=$(Mixer_SRCS:.C=.o)

Mixer_LIBS := $(FLTK_LIBS)

INCLUDES := -I.

Mixer/mixer: $(Mixer_OBJS) FL
	@ echo -n Linking mixer...
	@ $(CXX) $(CXXFLAGS) $(Mixer_LIBS)  $(Mixer_OBJS) -o $@ -LFL -lfl_widgets && echo $(DONE)

.PHONEY: Mixer
Mixer: Mixer/mixer

Mixer_clean:
	rm -f $(Mixer_OBJS) Mixer/mixer
