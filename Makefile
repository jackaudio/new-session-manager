
CXXFLAGS := -ggdb -Wall -O0 -fno-rtti -fno-exceptions
LIBS := -lsndfile  `fltk-config --ldflags`

all: all

%:
	@ make -s -C FL CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)" $@
	@ make -s -C Mixer CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)" $@
