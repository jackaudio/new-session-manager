
# Makefile for the Non Sequencer.
# Copyright 2007-2008 Jonathan Moore Liles
# This file is licencesd under version 2 of the GPL.

VERSION := 1.9.1

all: make.conf non-sequencer

make.conf: configure
	@ ./configure

config:
	@ ./configure

-include make.conf

SYSTEM_PATH=$(prefix)/share/non-sequencer/
DOCUMENT_PATH=$(prefix)/share/doc/non-sequencer/

ifeq ($(USE_DEBUG),yes)
	CXXFLAGS := -pipe -ggdb -Wall -Wextra -Wnon-virtual-dtor -Wno-missing-field-initializers -O0 -fno-rtti -fno-exceptions
else
	CXXFLAGS := -pipe -O3 -fno-rtti -fno-exceptions -DNDEBUG
endif

CFLAGS+=-DVERSION=\"$(VERSION)\" \
	-DINSTALL_PREFIX=\"$(prefix)\" \
	-DSYSTEM_PATH=\"$(SYSTEM_PATH)\" \
	-DDOCUMENT_PATH=\"$(DOCUMENT_PATH)\"

CXXFLAGS:=$(CFLAGS) $(CXXFLAGS) $(FLTK_CFLAGS) $(sigcpp_CFLAGS) $(LASH_CFLAGS)

LIBS:=$(FLTK_LIBS) $(JACK_LIBS) $(LASH_LIBS) $(sigcpp_LIBS)

ifeq ($(JACK_MIDI_PROTO_API),yes)
	CXXFLAGS+=-DJACK_MIDI_PROTO_API
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

.PHONEY: all clean install dist valgrind config

clean:
	rm -f non-sequencer makedepend $(OBJS)
	@ echo "$(DONE)"

valgrind:
	valgrind ./non-sequencer

include scripts/colors

.C.o:
	@ echo "Compiling: $(BOLD)$(YELLOW)$<$(SGR0)"
	@ $(CXX) $(CXXFLAGS) -c $< -o $@

%.C : %.fl
	@ cd `dirname $<` && fluid -c ../$<

$(OBJS): make.conf

DONE:=$(BOLD)$(GREEN)done$(SGR0)

non-sequencer: $(OBJS)
	@ echo -n "Linking..."
	@ rm -f $@
	@ $(CXX) $(CXXFLAGS) $(LIBS) $(OBJS) -o $@ || echo "$(BOLD)$(RED)Error!$(SGR0)"
	@ if test -x $@; then echo "$(DONE)"; test -x "$(prefix)/bin/$@" || echo "You must now run 'make install' (as the appropriate user) to install the executable, documentation and other support files in order for the program to function properly."; fi

install: all
	@ echo -n "Installing..."
	@ install non-sequencer $(prefix)/bin
	@ mkdir -p "$(SYSTEM_PATH)"
	@ cp -r instruments "$(SYSTEM_PATH)"
	@ mkdir -p "$(DOCUMENT_PATH)"
	@ cp doc/*.html doc/*.png "$(DOCUMENT_PATH)"
	@ echo "$(DONE)"

dist:
	git archive --prefix=non-sequencer-$(VERSION)/ v$(VERSION) | bzip2 > non-sequencer-$(VERSION).tar.bz2

TAGS: $(SRCS)
	etags $(SRCS)

makedepend: make.conf $(SRCS)
	@ echo -n Checking dependencies...
	@ makedepend -f- -- $(CXXFLAGS) -- $(SRCS) > makedepend 2>/dev/null && echo "$(DONE)"

-include makedepend
