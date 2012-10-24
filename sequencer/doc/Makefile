
SRCS=$(wildcard *.mu)
OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	@ echo Mupping $<...
	@ mup.wrapper html $<

.PHONY: all clean

all: $(OBJS)

upload: all
	@ rsync -L mup.css MANUAL.html *.png ssh.tuxfamily.org:/home/non/non-sequencer.tuxfamily.org-web/htdocs

install:
	@ install -d "$(DESTDIR)$(DOCUMENT_PATH)/non-sequencer"
	@ install -m 644 $(OBJS) *.png mup.css ../../COPYING "$(DESTDIR)$(DOCUMENT_PATH)/non-sequencer"

clean:
	rm -f $(OBJS)