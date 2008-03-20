
SRCS= \
      Fl_Scalepack.C \
      Fl_Arc_Dial.C \
      Boxtypes.C \

OBJS := $(SRCS:.C=.o)

INCLUDES=-I..

all: libfl_widgets.a

$(OBJS): Makefile

include ../make.inc

libfl_widgets.a: $(OBJS)
	ar rcs $@ $(OBJS)
#	$(CXX) -static $(CXXFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	rm -f $(OBJS) mixer makedepend

valgrind:
	valgrind ./test

include makedepend
