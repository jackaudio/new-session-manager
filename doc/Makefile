

SRCS=OVERVIEW.mu KEYS.mu MANUAL.mu
OBJS=$(SRCS:.mu=.html)

%.html: %.mu
	mup html < $< > $@

.PHONY: all clean

all: $(OBJS)

clean:
	rm -f $(OBJS)