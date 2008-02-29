
CXXFLAGS=-ggdb -Wall -O0
#LIBS=-L/usr/lib/sox -I/usr/include/sox -lsox -lsfx
LIBS=-lsndfile  `fltk-config --ldflags`
# CXXFLAGS=`fltk-config -cxxflags`

SRCS= Waveform.C  Region.C  Peaks.C  main.C Track.C Audio_Track.C Timeline.C Audio_File.C Audio_File_SF.C Loggable.C Track_Header.C

OBJS=$(SRCS:.C=.o)

.PHONEY: all clean install dist valgrind

all: test makedepend

.C.o:
	@ echo -n "Compiling: "; tput bold; tput setaf 3; echo $<; tput sgr0; true
	@ $(CXX) $(CXXFLAGS) -c $< -o $@


%.C : %.fl
	@ fluid -c $<

$(OBJS): Makefile


test: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	rm -f $(OBJS) test makedepend

valgrind:
	valgrind ./test

TAGS: $(SRCS)
	etags $(SRCS)

makedepend: $(SRCS)
	@ echo -n Checking dependencies...
	@ makedepend -f- -- $(CXXFLAGS) -- $(SRCS) > makedepend 2>/dev/null && echo done.


include makedepend
