
CXXFLAGS := -ggdb -Wall -O0 -fno-rtti -fno-exceptions
LIBS := -lsndfile  `fltk-config --ldflags`

all:
	@ make -C FL CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)"
	@ make -C Mixer CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)"
