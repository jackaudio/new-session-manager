
SRCS= \
      DPM.C \
      Mixer_Strip.C \
      Panner.C \
      Mixer.C \

OBJS := $(SRCS:.C=.o)

LIBS := $(LIBS) -L../FL -lfl

INCLUDES=-I..

CXXFLAGS := $(CXXFLAGS) -fPIC

.PHONEY: all clean install dist valgrind

all: mixer

$(OBJS): Makefile

include ../make.inc

mixer: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBS)   $(OBJS) -o $@ -L../FL -lfl_widgets

clean:
	rm -f $(OBJS) mixer makedepend

valgrind:
	valgrind ./test

include makedepend
