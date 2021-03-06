CC := gcc
LD := gcc
AR := ar
BUILD_ROOT := build/debug

SOURCES := $(shell find src -name '*.c')
HEADERS := $(shell find include -name '*.h')

.DEFAULT_GOAL := all

.PHONY: all
all:
	@if [ $EUID -e 0 ]; then \
		@read -p "WARNING: building as root, continue anyway [y/n]?" -n 1 r
		@echo
		@if [[ $REPLY =~ ^[Yy]$ ]]; then \
			$(MAKE) -C $(BUILD_ROOT); \
		fi; \
	fi;

.PHONY: test
test:
	$(MAKE) -C $(BUILD_ROOT) test

.PHONY: coverage
coverage:
	$(MAKE) -C $(BUILD_ROOT) coverage

.PHONY: install
install:
	$(MAKE) -C $(BUILD_ROOT) install

.PHONY: clean
clean:
	@for a in $$(ls build); do\
		if [ -d build/$$a ]; then \
			$(MAKE) -C build/$$a $(MAKECMDGOALS); \
		fi; \
	done;
	-rm -rf docs

docs: doxygen.conf $(SOURCES) $(HEADERS)
	doxygen $<

.PHONY: debug_makefile
debug_makefile:
	$(MAKE) -C $(BUILD_ROOT) $(MAKECMDGOALS)

