
SRCS=$(wildcard *.mu)
OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	mup.wrapper html $<

.PHONY: all clean

all: $(OBJS)

clean:
	rm -f $(OBJS)