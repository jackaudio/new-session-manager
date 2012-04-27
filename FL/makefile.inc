# -*- mode: makefile; -*-

VERSION := 1.1.0
PACKAGE := FL

all: FL

SRCS := $(wildcard FL/*.C FL/*.fl)

SRCS:=$(SRCS:.fl=.C)
SRCS:=$(sort $(SRCS))
OBJS:=$(SRCS:.C=.o)

all: FL/libfl_widgets.a util/ntk-perf

FL/libfl_widgets.a: $(OBJS)
	@ ar rcs $@ $(OBJS)

.PHONEY: FL
FL: FL/libfl_widgets.a

# ntk-chtheme: ntk-chtheme.o $(FLTK_LIBS) FL/libfl_widgets.a
# 	@ echo -n Linking session handler.
# 	@ $(CXX) $(CXXFLAGS) $(FLTK_LDFLAGS) $(LIBS) $^ -o $@ -LFL -lfl_widgets  && echo $(DONE)


util/ntk-perf.o: util/ntk-perf.C

util/ntk-perf: util/ntk-perf.o $(FLTK_LIBS)  FL/libfl_widgets.a 
	@ echo -n Linking session handler.
	@ $(CXX) $(CXXFLAGS) $^ -o $@ $(FLTK_LDFLAGS) $(LIBS) -LFL -lfl_widgets && echo $(DONE)

clean:
	rm -f $(OBJS) FL/libfl_widgets.a .deps

install:
	@
