
SRCS= \
      Waveform.C \
      Region.C \
      main.C \
      Track.C \
      Audio_Track.C \
      Timeline.C \
      Track_Header.C \
      Track_Widget.C \
      Peaks.C \
      Audio_File.C \
      Audio_File_SF.C \
      Port.C \
      Disk_Stream.C \
      dsp.c \
      Engine.C \
      Transport.C \
      Loggable.C \

OBJS=$(SRCS:.C=.o)

INCLUDES=-I../Engine -I../FL

.PHONEY: all clean install dist valgrind

all: timeline

$(OBJS): Makefile

include ../make.inc

#LIBS:=$(LIBS) -ljack -lpthread

timeline: $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -ljack -lpthread $(OBJS) -o $@

clean:
	rm -f $(OBJS) timeline makedepend

valgrind:
	valgrind ./test

include makedepend
