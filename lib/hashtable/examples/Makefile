CFLAGS=-W -Wall -Wextra -Werror -g -std=c11 -I ..

SRCS := $(wildcard *.c)
EXECS := $(patsubst %.c,%,$(SRCS))

.PHONY: all
all: $(EXECS) Makefile

%: %.c
	$(CC) $(CFLAGS) $^ ../hashtable.a -o ../example_$@
