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

LIBS += $(LIBLO_LIBS) $(SIGCPP_LIBS)
CFLAGS += $(LIBLO_CFLAGS) $(SIGCPP_CFLAGS) $(XPM_CFLAGS) $(FLTK_CFLAGS)

src/nsmd: src/nsmd.o nonlib/libnonlib.a
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS) -Lnonlib -lnonlib && echo $(DONE)

src/session-manager: src/session-manager.o nonlib/libnonlib.a $(FLTK_LIBS)
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(FLTK_LDFLAGS) $(XPM_LIBS) $(LIBS) -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

src/nsm-proxy: src/nsm-proxy.o  nonlib/libnonlib.a
	@ echo -n Linking NSM Proxy.
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)  -Lnonlib -lnonlib && echo $(DONE)

# src/NSM_Proxy_UI.o:
# 	@ cd src && $(FLUID) -c NSM_Proxy_UI.fl

src/nsm-proxy-gui: src/NSM_Proxy_UI.o src/nsm-proxy-gui.o nonlib/libnonlib.a $(FLTK_LIBS)
	@ echo -n Linking NSM Proxy.
	@ $(CXX) $(CXXFLAGS) $(FLTK_LDFLAGS) $^ -o $@ $(XPM_LIBS) $(LIBS)  -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

src/send_osc: src/send_osc.o nonlib/libnonlib.a
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS) -Lnonlib -lnonlib && echo $(DONE)

src/jackpatch: src/jackpatch.o
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(JACK_LIBS) $(LIBLO_LIBS) && echo $(DONE)


Session: src/send_osc src/nsmd src/session-manager src/jackpatch src/nsm-proxy src/nsm-proxy-gui

clean:
	rm -f $(OBJS) src/nsmd src/session-manager src/jackpatch src/nsm-proxy src/nsm-proxy-gui .deps

install: all
	@ echo -n "Installing..."
	@ install -Dm755 src/nsmd "$(DESTDIR)$(prefix)"/bin/nsmd
	@ install -Dm755 src/session-manager "$(DESTDIR)$(prefix)"/bin/non-session-manager
	@ install -Dm755 src/nsm-proxy "$(DESTDIR)$(prefix)"/bin/nsm-proxy
	@ install -Dm755 src/nsm-proxy-gui "$(DESTDIR)$(prefix)"/bin/nsm-proxy-gui
	@ install -Dm755 src/jackpatch "$(DESTDIR)$(prefix)"/bin/jackpatch
	@ install -d "$(DESTDIR)$(SYSTEM_PATH)"/non-session-manager
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
