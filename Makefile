

VERSION := 0.5.0

FLTK_LIBS := `fltk-config --ldflags`
JACK_LIBS := `pkg-config --libs jack`
SNDFILE_LIBS := `pkg-config --libs sndfile`

CXXFLAGS := -DVERSION=\"$(VERSION)\" -ggdb -Wall -O0 -fno-rtti -fno-exceptions

all: makedepend FL Timeline Mixer

.C.o:
	@ echo -n "Compiling: "; tput bold; tput setaf 3; echo $<; tput sgr0; true
	@ $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.C : %.fl
	@ fluid -c $<


include FL/makefile.inc
include Timeline/makefile.inc
include Mixer/makefile.inc

SRCS:=$(FL_SRCS) $(Timeline_SRCS) $(Mixer_SRCS)

TAGS: $(SRCS)
	etags $(SRCS)

makedepend: $(SRCS)
	@ echo -n Checking dependencies...
	@ makedepend -f- -- $(CXXFLAGS) -- $(SRCS) > makedepend 2>/dev/null && echo done.

clean: FL_clean Timeline_clean Mixer_clean

include makedepend
