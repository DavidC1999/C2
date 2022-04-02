CC=gcc
CFLAGS=-W -Wall -Wextra -Werror -g -std=c11
TARGET=compiler

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(dir $(MKFILE_PATH))

# globs
SRCS := $(wildcard source/*.c)
HDRS := $(wildcard source/*.h)
OBJS := $(patsubst source/%.c,bin/%.o,$(SRCS))

# link it all together
$(TARGET): $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) $(OBJS) -o $(MKFILE_DIR)$(TARGET)

# compile an object based on source and headers
bin/%.o: source/%.c $(HDRS) Makefile | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p $@

# tidy up
clean:
	rm -rf bin
	rm -f $(TARGET)
