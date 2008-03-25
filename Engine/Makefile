
SRCS= \
      Server.C \
      Timeline_Server.C \
      main.C \
      Audio_File.C \
      Audio_File_SF.C \
      Peaks.C \
      Loggable.C \

OBJS=$(SRCS:.C=.o)

.PHONEY: all clean install dist valgrind

all: engine

$(OBJS): Makefile

include ../make.inc

engine: $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) $(OBJS) -o $@

clean:
	rm -f $(OBJS) engine makedepend

valgrind:
	valgrind ./test

include makedepend
