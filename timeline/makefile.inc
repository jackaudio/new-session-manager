# -*- mode: makefile; -*-

VERSION := 1.1.0
PACKAGE := TIMELINE
BIN_NAME := non-daw

all: Timeline

SRCS := $(wildcard src/*.C src/*.fl src/Engine/*.C)

SRCS:=$(SRCS:.fl=.C)
SRCS:=$(sort $(SRCS))
OBJS:=$(SRCS:.C=.o)

LIBS += $(FLTK_LDFLAGS) $(JACK_LIBS) $(SNDFILE_LIBS) $(LIBLO_LIBS) $(SIGCPP_LIBS) $(XPM_LIBS)

CFLAGS += $(SNDFILE_CFLAGS) $(FLTK_CFLAGS) $(JACK_CFLAGS) $(SIGCPP_CFLAGS) $(XPM_CFLAGS) -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

ifeq ($(USE_UNOPTIMIZED_DRAWING),yes)
	CFLAGS+=-DUSE_UNOPTIMIZED_DRAWING
endif

ifeq ($(USE_SINGLEBUFFERED_TIMELINE),yes)
	CFLAGS+=-DUSE_SINGLEBUFFERED_TIMELINE
endif

ifeq ($(USE_WIDGET_FOR_TIMELINE),yes)
	CFLAGS+=-DUSE_WIDGET_FOR_TIMELINE
endif


src/timeline: $(OBJS) FL/libfl_widgets.a nonlib/libnonlib.a $(FLTK_LIBS)
	@ echo -n Linking timeline...
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)  -LFL -lfl_widgets -Lnonlib -lnonlib && echo $(DONE)

Timeline: src/timeline

clean:
	rm -f $(OBJS) src/timeline .deps

install: all
	@ echo -n "Installing..."
	@ install -Dm755 src/timeline "$(DESTDIR)$(prefix)"/bin/non-daw
	@ install -d "$(DESTDIR)$(SYSTEM_PATH)"/non-daw
	@ $(MAKE) -s -C doc install
	@ install -d "$(DESTDIR)$(PIXMAP_PATH)/non-daw"
	@ install -Dm644 icons/hicolor/256x256/apps/non-daw.png "$(DESTDIR)$(PIXMAP_PATH)"/non-daw/icon-256x256.png
	@ install -d "$(DESTDIR)$(ICON_PATH)/hicolor"
	@ cp -au icons/hicolor/ "$(DESTDIR)$(ICON_PATH)"
	@ install -d "$(DESTDIR)$(DESKTOP_PATH)"
	@ sed 's:@BIN_PATH@:$(prefix)/bin:' non-daw.desktop.in > "$(DESTDIR)$(DESKTOP_PATH)/non-daw.desktop"
	@ echo "$(DONE)"
ifneq ($(USE_DEBUG),yes)
	@ echo -n "Stripping..."
	@ strip "$(DESTDIR)$(prefix)"/bin/non-daw
	@ echo "$(DONE)"
endif
