# -*- mode: makefile; -*-

Timeline_VERSION := 0.5.0

Timeline_SRCS= \
Timeline/Annotation_Region.C \
Timeline/Audio_Region.C \
Timeline/Audio_Sequence.C \
Timeline/Control_Point.C \
Timeline/Control_Sequence.C \
Timeline/Fl_Menu_Settings.C \
Timeline/LASH.C \
Timeline/LASH_Client.C \
Timeline/Loggable.C \
Timeline/Project.C \
Timeline/Sequence.C \
Timeline/Sequence_Point.C \
Timeline/Sequence_Region.C \
Timeline/Sequence_Widget.C \
Timeline/TLE.C \
Timeline/Tempo_Point.C \
Timeline/Tempo_Sequence.C \
Timeline/Time_Point.C \
Timeline/Time_Sequence.C \
Timeline/Timeline.C \
Timeline/Track.C \
Timeline/Transport.C \
Timeline/Waveform.C \
Timeline/main.C \
util/debug.C \

Timeline_SRCS+=\
Timeline/Engine/Audio_File.C \
Timeline/Engine/Audio_File_SF.C \
Timeline/Engine/Audio_Region.C \
Timeline/Engine/Audio_Sequence.C \
Timeline/Engine/Disk_Stream.C \
Timeline/Engine/Engine.C \
Timeline/Engine/Peaks.C \
Timeline/Engine/Playback_DS.C \
Timeline/Engine/Port.C \
Timeline/Engine/Record_DS.C \
Timeline/Engine/Timeline.C \
Timeline/Engine/Track.C \
Timeline/Engine/dsp.C \



Timeline_OBJS:=$(Timeline_SRCS:.C=.o)

# $(Timeline_OBJS): Makefile
# $(Timeline_OBJS): make.conf

Timeline_LIBS := $(FLTK_LIBS) $(JACK_LIBS) $(SNDFILE_LIBS) $(LASH_LIBS)

Timeline/timeline: $(Timeline_OBJS) FL
	@ echo -n Linking timeline...
	@ $(CXX) $(CXXFLAGS) $(INCLUDES) $(Timeline_LIBS)  $(Timeline_OBJS) -o $@ -LFL -lfl_widgets && echo $(DONE)

Timeline: Timeline/timeline

Timeline_clean:
	rm -f $(Timeline_OBJS) Timeline/timeline
