CFLAGS:= -g -Wextra -Werror
CFLAGS+= -mtune=native -march=native -O3
#CLFAGS+=-O0

all: wc

wc: wc.c

clean:
	$(RM) wc

.PHONY: all clean
