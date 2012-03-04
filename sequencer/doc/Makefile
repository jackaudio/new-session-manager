
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

clean:
	rm -f $(OBJS)