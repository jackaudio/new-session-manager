
SRCS=$(wildcard *.mu)
OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	@ echo Mupping $<...
	@ mup.wrapper html $<

.PHONY: all clean

all: $(OBJS)

upload: all
	@ ln -sf OVERVIEW.html index.html
	@ rsync -L mup.css MANUAL.html index.html *.png ssh.tuxfamily.org:/home/non/non-sequencer.tuxfamily.org-web/htdocs
	@ rm -f index.html

install:
	@ install -d "$(DESTDIR)$(DOCUMENT_PATH)/non-sequencer"
	@ install -m 644 $(OBJS) *.png mup.css ../../COPYING "$(DESTDIR)$(DOCUMENT_PATH)/non-sequencer"

clean:
	rm -f $(OBJS)