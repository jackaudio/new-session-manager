
OBJS:= ntk/lib/libntk.a ntk/lib/libntk_images.a
SRCS:= ntk/src/*.cxx ntk/FL/*.H

$(SRCS): ntk/config.h
	@ echo Building NTK
	cd ntk && make

$(OBJS): ntk/config.h $(SRCS)
	@ echo Building NTK
	cd ntk && make

ntk/config.h: Makefile
	@ echo Configuring NTK
	cd ntk && ./configure --prefix=/usr/local --enable-shared --disable-gl --enable-threads --enable-xft --enable-cairo --enable-cairoext --enable-xinerama

config: ntk/config.h

clean:

all: $(OBJS)
	@ echo Building NTK
	cd ntk && make
