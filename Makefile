
CXXFLAGS=-ggdb

LIBS=`fltk-config --ldflags`
# CXXFLAGS=`fltk-config -cxxflags`

OBJS=Waveform.o Region.o main.o

.C.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	rm -f $(OBJS) test
