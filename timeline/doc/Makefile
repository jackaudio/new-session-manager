
SRCS=$(wildcard *.mu)

OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	@ echo Mupping $<...
	@ mup.wrapper html $<

.PHONY: all clean

all: $(OBJS)

upload: all
	@ ln -sf OVERVIEW.html index.html
	@ rsync -L mup.css MANUAL.html index.html *.png ssh.tuxfamily.org:/home/non/non-daw.tuxfamily.org-web/htdocs
	@ rm -f index.html

install:
	@ mkdir -p $(DOCUMENT_PATH)/non-daw
	@ cp $(OBJS) *.png mup.css ../../COPYING $(DOCUMENT_PATH)/non-daw
#	@ ln -sf $(PIXMAP_PATH)/logo.png $(DOCUMENT_PATH)

clean:
	rm -f $(OBJS)
