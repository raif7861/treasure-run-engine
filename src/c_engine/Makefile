# Makefile for Treasure Runner – Student Code

# Disable built-in rules to avoid conflicts
.SUFFIXES:

# Compiler and tools
CC      := gcc
AR      := ar
ARFLAGS := rcs

# Flags
CFLAGS  := -Wall -Wextra -O2 -fPIC -fvisibility=default -Iinclude
LDFLAGS :=

# Project layout
SRC_DIR        := src
INCLUDE_DIR    := include
BUILD_DIR      := build
LIB_DIR        := lib
BIN_DIR        := bin
TESTS_DIR      := tests
TEST_BIN_DIR   := build/tests
DIST_DIR       := ../dist
TOOLS_DIR := tools
TOOLS_BIN_DIR := build/tools

# External library
PUZZLEGEN_SO := $(DIST_DIR)/libpuzzlegen.so

# Student outputs
STUDENT_SO  := $(LIB_DIR)/libbackend.so

# Source / object mapping
STUDENT_SRCS := $(wildcard $(SRC_DIR)/*.c)
STUDENT_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(STUDENT_SRCS))

# Check (unit testing)
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS   := $(shell pkg-config --libs check)

# Option 1: Single test binary with main.c + all suite files
TEST_MAIN := $(TESTS_DIR)/main.c
TEST_SUITES := $(filter-out $(TEST_MAIN),$(wildcard $(TESTS_DIR)/*.c))
TEST_RUNNER := $(TEST_BIN_DIR)/test_runner

# Option 2: Each test_*.c is standalone (comment out if using Option 1)
# TEST_SRCS := $(wildcard $(TESTS_DIR)/test_*.c)
# TEST_NAMES := $(notdir $(basename $(TEST_SRCS)))
# TEST_BINS := $(addprefix $(TEST_BIN_DIR)/,$(TEST_NAMES))

# Linker configuration
RPATH_DIST := -Wl,-rpath,'$$ORIGIN/../../dist'
RPATH_LIB := -Wl,-rpath,'$$ORIGIN'
LINK_PUZZLEGEN := -L$(DIST_DIR) -lpuzzlegen -lm

# Phony targets
.PHONY: all clean help student_so dist test tests c-render puzzlegen_demo

# Default target
all: student_so
	@echo "✓ Student code compiled successfully"

# Directory creation
$(BUILD_DIR) $(LIB_DIR) $(BIN_DIR) $(TEST_BIN_DIR) $(TOOLS_BIN_DIR):
	@mkdir -p $@

# Object compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) \
	    -I$(INCLUDE_DIR) \
	    -I../data_gen/include \
	    -c $< -o $@

# Shared library
student_so: $(LIB_DIR) $(STUDENT_OBJS) $(PUZZLEGEN_SO)
	$(CC) -shared -o $(STUDENT_SO) \
	    $(STUDENT_OBJS) \
	    $(LINK_PUZZLEGEN) \
	    $(RPATH_LIB)
	@echo "✓ Built $(STUDENT_SO)"

# Tools
$(TOOLS_BIN_DIR)/c-render: $(TOOLS_DIR)/test_render.c $(STUDENT_OBJS) $(PUZZLEGEN_SO) | $(TOOLS_BIN_DIR)
	$(CC) $(CFLAGS) \
	    -I$(INCLUDE_DIR) \
	    -I../data_gen/include \
	    $< $(STUDENT_OBJS) \
	    -o $@ \
	    $(LINK_PUZZLEGEN) \
	    $(RPATH_DIST)

c-render: $(TOOLS_BIN_DIR)/c-render
	@echo "✓ Built c-render tool"

$(TOOLS_BIN_DIR)/puzzlegen_demo: $(TOOLS_DIR)/puzzlegen_demo.c $(PUZZLEGEN_SO) | $(TOOLS_BIN_DIR)
	$(CC) $(CFLAGS) \
	    -I../data_gen/include \
	    $< \
	    -o $@ \
	    $(LINK_PUZZLEGEN) \
	    $(RPATH_DIST)

puzzlegen_demo: $(TOOLS_BIN_DIR)/puzzlegen_demo
	@echo "✓ Built puzzlegen_demo tool"

# Tests
# Option 1: Single test runner with all suites
$(TEST_RUNNER): $(TEST_MAIN) $(TEST_SUITES) $(STUDENT_OBJS) | $(TEST_BIN_DIR)
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) \
	    -I$(INCLUDE_DIR) \
	    -I../data_gen/include \
	    $(TEST_MAIN) $(TEST_SUITES) $(STUDENT_OBJS) \
	    -o $@ \
	    $(LINK_PUZZLEGEN) \
	    $(RPATH_DIST) \
	    $(CHECK_LIBS)

# Option 2: Each test_*.c is standalone (uncomment if using Option 2)
# $(TEST_BIN_DIR)/test_%: $(TESTS_DIR)/test_%.c $(STUDENT_OBJS) | $(TEST_BIN_DIR)
# 	$(CC) $(CFLAGS) $(CHECK_CFLAGS) \
# 	    -I$(INCLUDE_DIR) \
# 	    -I../data_gen/include \
# 	    $^ -o $@ \
# 	    $(LINK_PUZZLEGEN) \
# 	    $(RPATH_DIST) \
# 	    $(CHECK_LIBS)

test: $(TEST_RUNNER)
	@echo "Running C unit tests..."
	@if [ ! -f "$(TEST_RUNNER)" ]; then \
	    echo "No test runner found"; \
	    exit 1; \
	fi
	@export TREASURE_RUNNER_ASSETS=$(CURDIR)/../assets; \
	export LD_LIBRARY_PATH=$(CURDIR)/$(DIST_DIR):$$LD_LIBRARY_PATH; \
	export CK_VERBOSITY=normal; \
	echo ""; \
	echo "=== Running test_runner ==="; \
	$(TEST_RUNNER); \
	echo ""; \
	echo "✓ All tests passed"
tests: test
# Clean
clean:
	@rm -rf $(BUILD_DIR) $(LIB_DIR) $(BIN_DIR)


