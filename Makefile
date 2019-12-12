ARCH?=native
CFLAGS:= -g -Wextra -Werror
CFLAGS+= -mtune=$(ARCH) -march=$(ARCH) -O3
#CFLAGS+= -O0

all: wc

wc: wc.c

clean:
	$(RM) wc

.PHONY: all clean
