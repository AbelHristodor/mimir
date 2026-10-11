# Build configurations, each into its own output directory under build/.
#   make release   -> build/release/hello
#   make debug     -> build/debug/hello
#   make asan      -> build/asan/hello   + prove AddressSanitizer catches its bug
#   make ubsan     -> build/ubsan/hello  + prove UBSan catches its bug
#   make tsan      -> build/tsan/hello   + prove ThreadSanitizer catches its bug
#   make all       -> every configuration
#   make check     -> build all, run the tests
#   make clean     -> remove build/

CC     := cc
TARGET := main
SRCS   := $(wildcard src/*.c src/builtins/*.c)
RUN    := tests/run.sh

# Warnings are on and fatal. -MMD -MP emit .d files so headers trigger rebuilds.
CFLAGS := -Wall -Wextra -Wpedantic -Werror -MMD -MP -Isrc
LDLIBS := -lreadline

# Per-configuration extra flags.
RELEASE_FLAGS := -O2 -DNDEBUG
DEBUG_FLAGS   := -O0 -g3
ASAN_FLAGS    := -O1 -g -fno-omit-frame-pointer -fsanitize=address
UBSAN_FLAGS   := -O1 -g -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all
TSAN_FLAGS    := -O1 -g -fno-omit-frame-pointer -fsanitize=thread -pthread

.PHONY: help all release debug asan ubsan tsan check clean

# Default target: show the command list.
help:
	@echo "Targets:"
	@echo "  make release   Build build/release/$(TARGET)   (-O2 -DNDEBUG)"
	@echo "  make debug     Build build/debug/$(TARGET)     (-O0 -g3)"
	@echo "  make asan      Build + prove AddressSanitizer catches its bug"
	@echo "  make ubsan     Build + prove UndefinedBehaviorSanitizer catches its bug"
	@echo "  make tsan      Build + prove ThreadSanitizer catches its bug"
	@echo "  make all       Build every configuration (runs the sanitizer tests)"
	@echo "  make check     Build all, run every test"
	@echo "  make clean     Remove build/"
	@echo "  make help      Show this list"

all: release debug asan ubsan tsan

# ---- release ----------------------------------------------------------------
release: build/release/$(TARGET)

build/release/$(TARGET): $(SRCS) | build/release
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $(SRCS) -o $@ $(LDLIBS)

build/release:
	mkdir -p $@

# ---- debug ------------------------------------------------------------------
debug: build/debug/$(TARGET)

build/debug/$(TARGET): $(SRCS) | build/debug
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SRCS) -o $@ $(LDLIBS)

build/debug:
	mkdir -p $@

# ---- asan (AddressSanitizer) ------------------------------------------------
# Build the app, then run a heap-buffer-overflow test that asan must catch.
asan: build/asan/$(TARGET) build/asan/asan_bug
	@$(RUN) catch build/asan/asan_bug AddressSanitizer

build/asan/$(TARGET): $(SRCS) | build/asan
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(SRCS) -o $@ $(LDLIBS)

build/asan/asan_bug: tests/asan_bug.c | build/asan
	$(CC) $(CFLAGS) $(ASAN_FLAGS) tests/asan_bug.c -o $@

build/asan:
	mkdir -p $@

# ---- ubsan (UndefinedBehaviorSanitizer) -------------------------------------
# Build the app, then run a signed-overflow test that ubsan must catch.
ubsan: build/ubsan/$(TARGET) build/ubsan/ubsan_bug
	@$(RUN) catch build/ubsan/ubsan_bug "runtime error"

build/ubsan/$(TARGET): $(SRCS) | build/ubsan
	$(CC) $(CFLAGS) $(UBSAN_FLAGS) $(SRCS) -o $@ $(LDLIBS)

build/ubsan/ubsan_bug: tests/ubsan_bug.c | build/ubsan
	$(CC) $(CFLAGS) $(UBSAN_FLAGS) tests/ubsan_bug.c -o $@

build/ubsan:
	mkdir -p $@

# ---- tsan (ThreadSanitizer) -------------------------------------------------
# Build the app, then run a data-race test that tsan must catch.
tsan: build/tsan/$(TARGET) build/tsan/tsan_bug
	@TSAN_OPTIONS=halt_on_error=1 $(RUN) catch build/tsan/tsan_bug ThreadSanitizer

build/tsan/$(TARGET): $(SRCS) | build/tsan
	$(CC) $(CFLAGS) $(TSAN_FLAGS) $(SRCS) -o $@ $(LDLIBS)

build/tsan/tsan_bug: tests/tsan_bug.c | build/tsan
	$(CC) $(CFLAGS) $(TSAN_FLAGS) tests/tsan_bug.c -o $@

build/tsan:
	mkdir -p $@

# ---- tests ------------------------------------------------------------------
# `all` already runs the three sanitizer catch-tests; add smoke tests for the
# plain builds so `make check` exercises every configuration.
check: all
	@$(RUN) ok build/release/$(TARGET)
	@$(RUN) ok build/debug/$(TARGET)
	@echo "All checks passed."

# ---- housekeeping -----------------------------------------------------------
clean:
	rm -rf build

# Pull in auto-generated header dependencies (created by -MMD -MP).
-include $(wildcard build/*/*.d)
