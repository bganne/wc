CFLAGS:= -Wall -Werror -g
CFLAGS+= -O2
LDFLAGS:=-g

all: wc

wc: wc.c

clean:
	$(RM) wc

.PHONY: all clean
