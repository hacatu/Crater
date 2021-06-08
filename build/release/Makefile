CC := gcc
LD := gcc
AR := ar
PROJECT_ROOT := $(shell pwd)

LIB_SOURCES := $(shell find ../../src/lib -name '*.c')
TEST_SOURCES := $(shell find ../../src/test -name '*.c')
BINARY_SOURCES := $(shell find ../../src/bin -name '*.c')

LIB_OBJS := $(patsubst ../../src/lib/%.c,obj/lib/%.o,$(LIB_SOURCES))
TEST_OBJS := $(patsubst ../../src/test/%.c,obj/test/%.o,$(TEST_SOURCES))
BINARY_OBJS := $(patsubst ../../src/bin/%.c,obj/bin/%.o,$(BINARY_SOURCES))

LIB_CFLAGS := @cflags.txt
TEST_CFLAGS := @cflags.txt
BINRARY_CFLAGS := @cflags.txt

TEST_LDFLAGS := @ldflags.txt
BINARY_LDFLAGS := @ldflags.txt

MAKEDEPS_CFLAGS = @makedeps_cflags.txt -MM -MQ $@ -MG -MP $^ -MF

SIMPLE_LIB_ARCS := $(patsubst ../../src/lib/%.c,lib/lib%.a,$(shell find ../../src/lib -maxdepth 1 -name '*.c'))
SIMPLE_TEST_BINS := $(patsubst ../../src/test/%.c,bin/test/%,$(shell find ../../src/test -maxdepth 1 -name '*.c'))
SIMPLE_BINARIES := $(patsubst ../../src/bin/%.c,bin/%,$(shell find ../../src/bin -maxdepth 1 -name '*.c'))

COMPOUND_LIB_ARCS := $(patsubst ../../src/lib/%,lib/lib%.a,$(shell find ../../src/lib -mindepth 1 -maxdepth 1 -type d))
COMPOUND_TEST_BINS := $(patsubst ../../src/test/%,bin/test/%,$(shell find ../../src/test -mindepth 1 -maxdepth 1 -type d))
COMPOUND_BINARIES := $(patsubst ../../src/bin/%,bin/%,$(shell find ../../src/bin -mindepth 1 -maxdepth 1 -type d))

.DEFAULT_GOAL := all

.PHONY: all
all: $(SIMPLE_LIB_ARCS) $(COMPOUND_LIB_ARCS) $(SIMPLE_BINARIES) $(COMPOUND_BINARIES)

.SECONDARY:
bin bin/test lib obj obj/lib obj/bin obj/test log:
	@mkdir -p $@

obj/lib/%.o: ../../src/lib/%.c | obj/lib
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) -c -o $@ $<
	@$(CC) $(MAKEDEPS_CFLAGS) obj/lib/$*.d $^

obj/bin/%.o: ../../src/bin/%.c | obj/bin
	@mkdir -p $(@D)
	$(CC) $(BINARY_CFLAGS) -c -o $@ $<
	@$(CC) $(MAKEDEPS_CFLAGS) obj/bin/$*.d $<

obj/test/%.o: ../../src/test/%.c | obj/test
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -c -o $@ $<
	@$(CC) $(MAKEDEPS_CFLAGS) obj/test/$*.d $<

$(SIMPLE_LIB_ARCS): lib/lib%.a: obj/lib/%.o | lib
	$(AR) rcs $@ $^

$(COMPOUND_LIB_ARCS): lib/lib%.a: obj/lib/%.compound | lib
	$(AR) rcs $@ $(filter obj/lib/$*/%.o,$(LIB_OBJS))

$(SIMPLE_BINARIES): bin/%: obj/bin/%.o | $(SIMPLE_LIB_ARCS) $(COMPOUND_LIB_ARCS) bin
	$(LD) -o $@ $^ $(BINARY_LDFLAGS)

$(COMPOUND_BINARIES): bin/%: obj/bin/%.compound | $(SIMPLE_LIB_ARCS) $(COMPOUND_LIB_ARCS) bin
	$(LD) -o $@ $(filter obj/bin/$*/%.o,$(BINARY_OBJS)) $(BINARY_LDFLAGS)

$(SIMPLE_TEST_BINS): bin/test/%: obj/test/%.o | $(SIMPLE_LIB_ARCS) $(COMPOUND_LIB_ARCS) bin/test
	$(LD) -o $@ $^ $(TEST_LDFLAGS)

$(COMPOUND_TEST_BINS): bin/test/%: obj/test/%.compound | $(SIMPLE_LIB_ARCS) $(COMPOUND_LIB_ARCS) bin/test
	$(LD) -o $@ $(filter obj/test/$*/%.o,$(TEST_OBJS)) $(TEST_LDFLAGS)

#obj/%.compound: $(@D)
#	@touch $@

define make-compound
obj/$(1)/$(2).compound: $(filter obj/$(1)/$(2)/%.o,$(3))
	touch obj/$(1)/$(2).compound
endef

$(foreach c,$(patsubst lib/lib%.a,%,$(COMPOUND_LIB_ARCS)),$(eval $(call make-compound,lib,$c,$(LIB_OBJS))))
$(foreach c,$(patsubst bin/test/%,%,$(COMPOUND_TEST_BINS)),$(eval $(call make-compound,test,$c,$(TEST_OBJS))))
$(foreach c,$(patsubst bin/%,%,$(COMPOUND_BINARIES)),$(eval $(call make-compound,bin,$c,$(BINARY_OBJS))))

.PHONY: test
test: all $(SIMPLE_TEST_BINS) $(COMPOUND_TEST_BINS) log
	../../test/test.py ../../test `pwd`/bin/test 2>&1 | tee log/test`date +%m-%d-%y-%R:%S`.log

.PHONY: install
install: all
	cp $(SIMPLE_LIB_ARCS) $(COMPOUND_LIB_ARCS) /usr/lib/
	cp -r ../../include/* /usr/include

.PHONY: clean
clean:
	-rm -rf bin lib notes cov obj log test

debug_makefile:
	@echo $(COMPOUND_TEST_BINS)

obj/%.o: obj/%.d

DEPFILES := $(patsubst ../../src/%.c,obj/%.d,$(LIB_SOURCES) $(MODULE_SOURCES) $(TEST_SOURCES) $(BINARY_SOURCES))

$(DEPFILES):

-include $(wildcard $(DEPFILES))

