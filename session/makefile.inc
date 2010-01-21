# -*- mode: makefile; -*-

all: Session

Session_SRCS := $(wildcard src/*.C src/*.fl)
# Session_SRCS += util/debug.C util/Thread.C util/file.C

Session_SRCS:=$(Session_SRCS:.fl=.C)
Session_SRCS:=$(sort $(Session_SRCS))
Session_OBJS:=$(Session_SRCS:.C=.o)

Session_LIBS := $(LIBLO_LIBS)

src/nsmd: src/nsmd.o nonlib/libnonlib.a
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $(Session_LIBS) src/nsmd.o -o $@ -Lnonlib -lnonlib -ldl && echo $(DONE)

src/session-manager: src/session-manager.o nonlib/libnonlib.a
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $(FLTK_LIBS) $(Session_LIBS) src/session-manager.o -o $@ -Lnonlib -lnonlib -ldl && echo $(DONE)

src/send_osc: src/send_osc.o nonlib/libnonlib.a
	@ $(CXX) $(CXXFLAGS) $(Session_LIBS) src/send_osc.o -o $@ -Lnonlib -lnonlib -ddl && echo $(DONE)

Session: src/send_osc src/nsmd src/session-manager

Session_clean:
	rm -f $(Session_OBJS) src/nsmd
