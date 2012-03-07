

OBJS:= fltk/lib/libfltk.a fltk/lib/libfltk_images.a

$(OBJS): fltk/config.h
	cd fltk && make

fltk/config.h:
	cd fltk && ./configure --prefix=/dev/null --disable-shared --disable-gl --enable-threads --enable-xft --enable-xdbe --enable-xinerama

config: $(OBJS)

clean:

all: $(OBJS)
