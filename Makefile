
CXXFLAGS := -ggdb -Wall -O0 -fno-rtti -fno-exceptions
LIBS := -lsndfile  `fltk-config --ldflags`

all: all

%:
	@ $(MAKE) -s -C Engine CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)" $@
	@ $(MAKE) -s -C FL CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)" $@
	@ $(MAKE) -s -C Timeline CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)" $@
	@ $(MAKE) -s -C Mixer CXXFLAGS="$(CXXFLAGS)" LIBS="$(LIBS)" $@
