# -*- mode: makefile; -*-

Timeline_VERSION := 0.5.0

Timeline_SRCS= \
Timeline/Annotation_Region.C \
Timeline/Audio_File.C \
Timeline/Audio_File_SF.C \
Timeline/Audio_Region.C \
Timeline/Audio_Sequence.C \
Timeline/Control_Point.C \
Timeline/Control_Sequence.C \
Timeline/Disk_Stream.C \
Timeline/Engine.C \
Timeline/Fl_Menu_Settings.C \
Timeline/LASH.C \
Timeline/LASH_Client.C \
Timeline/Loggable.C \
Timeline/Peaks.C \
Timeline/Playback_DS.C \
Timeline/Port.C \
Timeline/Project.C \
Timeline/Record_DS.C \
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
Timeline/dsp.C \
Timeline/main.C \
debug.C \

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
