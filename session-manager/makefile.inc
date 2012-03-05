# -*- mode: makefile; -*-

VERSION := 1.0.0
PACKAGE := SESSION_MANAGER
BIN_NAME := non-session-manager

all: Session

SRCS := $(wildcard src/*.C src/*.fl)
# SRCS += util/debug.C util/Thread.C util/file.C

SRCS:=$(SRCS:.fl=.C)
SRCS:=$(sort $(SRCS))
OBJS:=$(SRCS:.C=.o)

LIBS := $(LIBLO_LIBS) $(SIGCPP_LIBS)
CFLAGS := $(LIBLO_CFLAGS) $(SIGCPP_CFLAGS) $(XPM_CFLAGS) $(FLTK_CFLAGS)

src/nsmd: src/nsmd.o nonlib/libnonlib.a
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $(LIBS) src/nsmd.o -o $@ -Lnonlib -lnonlib && echo $(DONE)

src/session-manager: src/session-manager.o nonlib/libnonlib.a
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $(FLTK_LIBS) $(XPM_LIBS) $(LIBS) src/session-manager.o -o $@ -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

src/send_osc: src/send_osc.o nonlib/libnonlib.a
	@ $(CXX) $(CXXFLAGS) $(LIBS) src/send_osc.o -o $@ -Lnonlib -lnonlib && echo $(DONE)

src/jackpatch: src/jackpatch.o
	@ $(CXX) $(CXXFLAGS) $(JACK_LIBS) $(LIBLO_LIBS) src/jackpatch.o -o $@ && echo $(DONE)

Session: src/send_osc src/nsmd src/session-manager src/jackpatch

clean:
	rm -f $(OBJS) src/nsmd src/session-manager src/jackpatch



install: all
	@ echo -n "Installing..."
	@ install src/nsmd "$(DESTDIR)$(prefix)"/bin/nsmd
	@ install src/session-manager "$(DESTDIR)$(prefix)"/bin/non-session-manager
	@ install src/jackpatch "$(DESTDIR)$(prefix)"/bin/jackpatch
	@ mkdir -p "$(DESTDIR)$(SYSTEM_PATH)"/non-session-manager
	@ $(MAKE) -s -C doc install
	@ install -d "$(DESTDIR)$(PIXMAP_PATH)/non-session-manager"
	@ install -m 644 icons/hicolor/256x256/apps/non-session-manager.png "$(DESTDIR)$(PIXMAP_PATH)"/non-session-manager/icon-256x256.png
	@ install -d "$(DESTDIR)$(ICON_PATH)/hicolor"
	@ cp -au icons/hicolor/ "$(DESTDIR)$(ICON_PATH)"
	@ install -d "$(DESTDIR)$(DESKTOP_PATH)"
	@ sed 's:@BIN_PATH@:$(prefix)/bin:' non-session-manager.desktop.in > "$(DESTDIR)$(DESKTOP_PATH)/non-session-manager.desktop"
	@ echo "$(DONE)"
ifneq ($(USE_DEBUG),yes)
	@ echo -n "Stripping..."
	@ strip "$(DESTDIR)$(prefix)"/bin/jackpatch
	@ strip "$(DESTDIR)$(prefix)"/bin/nsmd
	@ strip "$(DESTDIR)$(prefix)"/bin/non-session-manager
	@ echo "$(DONE)"
endif
