CC=gcc
CFLAGS=-c -W -Wall -Wextra -Werror -std=c11
CFLAGS_DEBUG= -g -DDEBUG
ARFLAGS=rcs

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(dir $(MKFILE_PATH))

TARGET=$(MKFILE_DIR)hashtable
TARGET_C=$(TARGET).c
TARGET_O=$(TARGET).o
TARGET_A=$(TARGET).a

.PHONY: all
all: release

.PHONY: release
release: $(TARGET_A)

.PHONY: debug
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(TARGET_A)

.PHONY: examples
examples: debug
	$(MAKE) -C examples

$(TARGET_A): $(TARGET_O) Makefile
	ar $(ARFLAGS) $(TARGET_A) $(TARGET_O)

$(TARGET_O): $(TARGET_C)
	$(CC) $(CFLAGS) $(TARGET_C) -o $(TARGET_O)

clean:
	rm -f $(TARGET_O) $(TARGET_A)
	rm -f example_*
