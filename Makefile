
# Makefile for the Non Sequencer.
# Copyright 2007-2008 Jonathan Moore Liles
# This file is licencesd under version 2 of the GPL.

# config
prefix=/usr/local/

SYSTEM_PATH=$(prefix)/share/non-sequencer/
DOCUMENT_PATH=$(prefix)/share/doc/non-sequencer/
USE_LASH=1

VERSION=1.9.1

# Debugging
CFLAGS:=-O0 -ggdb -fno-omit-frame-pointer -Wall
# Production
# CFLAGS:=-O3 -fomit-frame-pointer -DNDEBUG

CFLAGS+=-DVERSION=\"$(VERSION)\" \
	-DINSTALL_PREFIX=\"$(prefix)\" \
	-DSYSTEM_PATH=\"$(SYSTEM_PATH)\" \
	-DDOCUMENT_PATH=\"$(DOCUMENT_PATH)\"

CXXFLAGS:=$(CFLAGS) -fno-exceptions -fno-rtti `fltk-config --cxxflags` `pkg-config jack --atleast-version 0.105 || echo -DJACK_MIDI_PROTO_API` `pkg-config jack --cflags` `pkg-config --cflags sigc++-2.0`
LIBS=`pkg-config --libs jack` `fltk-config --use-images --ldflags` `pkg-config --libs sigc++-2.0`

ifeq ($(USE_LASH),1)
	LIBS+=-llash
	CXXFLAGS+=-DUSE_LASH `pkg-config --cflags lash-1.0`
endif

# uncomment this line to print each playback event to the console (not RT safe)
# CXXFLAGS+= -DDEBUG_EVENTS

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

.PHONEY: all clean install dist valgrind

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
	@ if test -x $@; then tput bold; tput setaf 2; echo done; tput sgr0; test -x "$(prefix)/bin/$@" || echo "You must now run 'make install' (as the appropriate user) to install the executable, documentation and other support files in order for the program to function properly."; fi

install: all
	@ echo -n "Installing..."
	@ install non $(prefix)/bin
	@ mkdir -p "$(SYSTEM_PATH)"
	@ cp -r instruments "$(SYSTEM_PATH)"
	@ mkdir -p "$(DOCUMENT_PATH)"
	@ cp doc/*.html doc/*.png "$(DOCUMENT_PATH)"
	@ tput bold; tput setaf 2; echo done; tput sgr0
#	make -C doc install

dist:
	git archive --prefix=non-sequencer-$(VERSION)/ v$(VERSION) | bzip2 > non-sequencer-$(VERSION).tar.bz2

TAGS: $(SRCS)
	etags $(SRCS)

makedepend: $(SRCS)
	@ echo -n Checking dependencies...
	@ makedepend -f- -- $(CXXFLAGS) -- $(SRCS) > makedepend 2>/dev/null && echo done.


include makedepend
