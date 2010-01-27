
NM_SRCS=$(wildcard non-mixer/*.mu)
SRCS=$(wildcard *.mu)
ND_SRCS=$(wildcard non-daw/*.mu)

OBJS=$(SRCS:.mu=.html)
NM_OBJS=$(NM_SRCS:.mu=.html)
ND_OBJS=$(ND_SRCS:.mu=.html)

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
	@ mkdir -p $(DOCUMENT_PATH)/non-mixer
	@ cp $(ND_OBJS) non-daw/*.png non-daw/mup.css ../COPYING $(DOCUMENT_PATH)/non-daw
	@ cp $(NM_OBJS) non-mixer/*.png non-mixer/mup.css ../COPYING $(DOCUMENT_PATH)/non-mixer
#	@ ln -sf $(PIXMAP_PATH)/logo.png $(DOCUMENT_PATH)

clean:
	rm -f $(OBJS) $(NM_OBJS) $(ND_OBJS)
