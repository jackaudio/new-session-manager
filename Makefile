
# Makefile for the Non Sequencer.
# Copyright 2007-2008 Jonathan Moore Liles
# This file is licencesd under version 2 of the GPL.

# config
PREFIX=/usr/local/
SYSTEM_PATH=$(PREFIX)/share/non-sequencer/
DOCUMENT_PATH=$(PREFIX)/share/doc/non-sequencer/
USE_LASH=1

VERSION=1.9.0

# Debugging
CFLAGS:=-O0 -ggdb -fno-omit-frame-pointer -Wall
# Production
# CFLAGS:=-O3 -fomit-frame-pointer -DNDEBUG

CFLAGS+=-DVERSION=\"$(VERSION)\" \
	-DINSTALL_PREFIX=\"$(PREFIX)\" \
	-DSYSTEM_PATH=\"$(SYSTEM_PATH)\" \
	-DDOCUMENT_PATH=\"$(DOCUMENT_PATH)\"

CXXFLAGS:=$(CFLAGS) -fno-exceptions -fno-rtti `fltk-config --cxxflags` `pkg-config jack --atleast-version 0.105 || echo -DJACK_MIDI_PROTO_API` `pkg-config jack --cflags` `pkg-config --cflags sigc++-2.0`
LIBS=`pkg-config --libs jack` `fltk-config --use-images --ldflags` `pkg-config --libs sigc++-2.0`

ifeq ($(USE_LASH),1)
	LIBS+=-llash
	CXXFLAGS+=-DUSE_LASH `pkg-config --cflags lash-1.0`
endif


SRCS= \
     canvas.C \
     debug.C \
     event.C \
     event_list.C \
     grid.C \
     gui/draw.C \
     gui/event_edit.C \
     gui/input.C \
     gui/ui.C \
     gui/widgets.C \
     instrument.C \
     jack.C \
     lash.C \
     main.C \
     mapping.C \
     midievent.C \
     pattern.C \
     phrase.C \
     scale.C \
     sequence.C \
     smf.C \
     transport.C

OBJS=$(SRCS:.C=.o)

.PHONEY: all clean install

all: non makedepend

clean:
	rm -f non makedepend $(OBJS)
	@ echo Done

valgrind:
	valgrind ./non

.C.o:
	@ echo -n "Compiling: "; tput bold; tput setaf 3; echo $<; tput sgr0; true
	@ $(CXX) $(CXXFLAGS) -c $< -o $@

%.C : %.fl
	@ cd gui && fluid -c ../$<

$(OBJS): Makefile

non: $(OBJS)
	@ echo -n "Linking..."
	@ rm -f $@
	@ $(CXX) $(CXXFLAGS) $(LIBS) $(OBJS) -o $@ || (tput bold; tput setaf 1; echo Error!; tput sgr0)
	@ test -x $@ && echo done.

install:
	@ echo -n "Installing..."
	@ install non $(PREFIX)/bin
	@ mkdir -p "$(SYSTEM_PATH)"
	@ cp -r instruments "$(SYSTEM_PATH)"
	@ mkdir -p "$(DOCUMENT_PATH)"
	@ cp doc/*.{html,png} "$(DOCUMENT_PATH)"
	@ echo done
#	make -C doc install

TAGS: $(SRCS)
	etags $(SRCS)

makedepend: $(SRCS)
	@ echo -n Checking dependencies...
	@ makedepend -f- -- $(CXXFLAGS) -- $(SRCS) > makedepend 2>/dev/null && echo done.


include makedepend
