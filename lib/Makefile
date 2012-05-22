
OBJS:= ntk/lib/libntk.a ntk/lib/libntk_images.a
SRCS:= $(wildcard ntk/src/*.cxx ntk/FL/*.H)

$(SRCS): ntk/config.h
	@ echo Building NTK
	$(MAKE) -C ntk

$(OBJS): $(SRCS)

ntk/config.h: Makefile
	@ echo Configuring NTK
	cd ntk && ./configure --prefix=/usr/local --enable-debug --enable-shared --disable-gl --enable-threads --enable-xft --enable-cairo --enable-cairoext --enable-xinerama
	@ touch ntk/config.h
	
config: ntk/config.h

clean:

all: $(OBJS)
