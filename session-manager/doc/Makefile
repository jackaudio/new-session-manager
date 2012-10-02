
SRCS=$(wildcard *.mu)

OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	@ echo Mupping $<...
	@ mup.wrapper html $<

.PHONY: all clean

all: $(OBJS)

upload: all
	@ ln -sf MANUAL.html index.html
	@ rsync -L mup.css MANUAL.html API.html index.html *.png ssh.tuxfamily.org:/home/non/non.tuxfamily.org-web/htdocs/nsm
	@ rm -f index.html

install:
	@ install -d "$(DESTDIR)$(DOCUMENT_PATH)"/non-session-manager
	@ cp $(OBJS) *.png mup.css ../../COPYING "$(DESTDIR)$(DOCUMENT_PATH)"/non-session-manager
#	@ ln -sf $(PIXMAP_PATH)/logo.png $(DOCUMENT_PATH)

clean:
	rm -f $(OBJS)
