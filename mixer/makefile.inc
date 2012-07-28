# -*- mode: makefile; -*-

VERSION := 1.1.0
PACKAGE := MIXER
BIN_NAME := non-mixer

all: Mixer

SRCS := $(wildcard src/*.C src/*.fl src/Engine/*.C)

SRCS:=$(SRCS:.fl=.C)
SRCS:=$(sort $(SRCS))
OBJS:=$(SRCS:.C=.o)

LIBS += $(FLTK_LDFLAGS) $(JACK_LIBS) $(LRDF_LIBS) $(LIBLO_LIBS) $(XPM_LIBS)

CFLAGS += $(FLTK_CFLAGS) $(JACK_CFLAGS) $(LRDF_CFLAGS) $(LIBLO_CFLAGS) $(XPM_CFLAGS)

src/mixer: $(OBJS) FL/libfl_widgets.a nonlib/libnonlib.a $(FLTK_LIBS)
	@ echo -n Linking mixer...
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS) -LFL -lfl_widgets -Lnonlib -lnonlib -ldl && echo $(DONE)

Mixer: src/mixer

clean:
	rm -f $(OBJS) src/mixer .deps

install: all
	@ echo -n "Installing..."
	@ install -Dm755 src/mixer "$(DESTDIR)$(prefix)"/bin/non-mixer
	@ ln -s "$(DESTDIR)$(prefix)"/bin/non-mixer "$(DESTDIR)$(prefix)"/bin/non-mixer-noui
	@ install -d "$(DESTDIR)$(SYSTEM_PATH)"/non-mixer
	@ $(MAKE) -s -C doc install
	@ install -d "$(DESTDIR)$(PIXMAP_PATH)/non-mixer"
	@ install -Dm644 icons/hicolor/256x256/apps/non-mixer.png "$(DESTDIR)$(PIXMAP_PATH)"/non-mixer/icon-256x256.png
	@ install -d "$(DESTDIR)$(ICON_PATH)/hicolor"
	@ cp -au icons/hicolor/ "$(DESTDIR)$(ICON_PATH)"
	@ install -d "$(DESTDIR)$(DESKTOP_PATH)"
	@ sed 's:@BIN_PATH@:$(prefix)/bin:' non-mixer.desktop.in > "$(DESTDIR)$(DESKTOP_PATH)/non-mixer.desktop"
	@ echo "$(DONE)"
ifneq ($(USE_DEBUG),yes)
	@ echo -n "Stripping..."
	@ strip "$(DESTDIR)$(prefix)"/bin/non-mixer
	@ echo "$(DONE)"
endif
