DIRS=$(dir $(wildcard */))
MAKE_ARGS=

.PHONY: all $(DIRS)
all: $(DIRS)

.PHONY: debug
debug: MAKE_ARGS += debug
debug: $(DIRS)

.PHONY: release
release: MAKE_ARGS += release
release: $(DIRS)

.PHONY: clean
clean: MAKE_ARGS += clean
clean: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ $(MAKE_ARGS)


