
CXXFLAGS=-ggdb

LIBS=`fltk-config --ldflags`
# CXXFLAGS=`fltk-config -cxxflags`

OBJS=Waveform.o main.o

.C.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: Waveform.o main.o
	$(CXX) $(CXXFLAGS) $(LIBS) $(OBJS) -o $@
