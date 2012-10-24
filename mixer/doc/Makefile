
SRCS=$(wildcard *.mu)

OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	@ echo Mupping $<...
	@ mup.wrapper html $<

.PHONY: all clean

all: $(OBJS)

upload: all
	@ rsync -L mup.css MANUAL.html *.png ssh.tuxfamily.org:/home/non/non-mixer.tuxfamily.org-web/htdocs

install:
	@ install -d "$(DESTDIR)$(DOCUMENT_PATH)"/non-mixer
	@ cp $(OBJS) *.png mup.css ../../COPYING "$(DESTDIR)$(DOCUMENT_PATH)"/non-mixer
#	@ ln -sf $(PIXMAP_PATH)/logo.png $(DOCUMENT_PATH)

clean:
	rm -f $(OBJS)
