

OBJS:= fltk/lib/libfltk.a fltk/lib/libfltk_images.a
SRCS:= fltk/src/*.cpp fltk/FL/*.H

$(SRCS): fltk/config.h
	@ echo Building FLTK
	cd fltk && make

$(OBJS): fltk/config.h $(SRCS)
	@ echo Building FLTK
	cd fltk && make

fltk/config.h: Makefile
	@ echo Configuring FLTK
	cd fltk && ./configure --prefix=/dev/null --disable-shared --disable-gl --enable-threads --enable-xft --disable-xdbe --enable-xinerama

config: $(OBJS)

clean:

all: $(OBJS)
	@ echo Building FLTK
	cd fltk && make
