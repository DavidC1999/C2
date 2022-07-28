CFLAGS=-W -Wall -Wextra -Werror -g -std=c11 -I ..

SRCS := $(wildcard *.c)
EXECS := $(patsubst %.c,%,$(SRCS))

.PHONY: all
all: $(EXECS) Makefile

%: %.c
	$(CC) $(CFLAGS) $^ ../vector.a -o ../example_$@
