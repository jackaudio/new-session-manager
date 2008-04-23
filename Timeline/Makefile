
SRCS= \
      Waveform.C \
      Region.C \
      main.C \
      TLE.C \
      Sequence.C \
      Audio_Sequence.C \
      Control_Sequence.C \
      Timeline.C \
      Track.C \
      Sequence_Widget.C \
      Tempo_Point.C \
      Time_Point.C \
      Peaks.C \
      Audio_File.C \
      Audio_File_SF.C \
      Port.C \
      Disk_Stream.C \
      Playback_DS.C \
      Record_DS.C \
      dsp.C \
      Engine.C \
      Transport.C \
      Loggable.C \

OBJS:=$(SRCS:.C=.o)
# OBJS:=$(OBJS:.c=.o)

INCLUDES=-I../Engine -I../FL

.PHONEY: all clean install dist valgrind

all: timeline

$(OBJS): Makefile

include ../make.inc

#LIBS:=$(LIBS) -ljack -lpthread

timeline: $(OBJS)
	echo $(SRCS) >/dev/stderr
	echo $(OBJS) >/dev/stderr
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -ljack -lpthread $(OBJS) -o $@

clean:
	rm -f $(OBJS) timeline makedepend

valgrind:
	valgrind ./test

include makedepend
