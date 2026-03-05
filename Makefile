# Makefile for metadata processor

# Project settings
PROJECT_NAME = metadata_processor
VERSION = 1.0.0

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
CPPFLAGS = -I$(INCDIR)

# Package config for dependencies
PKG_CONFIG = pkg-config
PACKAGES = gstreamer-1.0 gstreamer-app-1.0 glib-2.0

# Get flags from pkg-config
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(PACKAGES))
LDFLAGS += $(shell $(PKG_CONFIG) --libs $(PACKAGES))

# Source files (exclude test files from main build)
SOURCES = $(filter-out $(SRCDIR)/test_ard.c, $(wildcard $(SRCDIR)/*.c))
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/$(PROJECT_NAME)

# Test executable
TEST_SOURCE = $(SRCDIR)/test_ard.c
TEST_TARGET = $(BINDIR)/test_ard

# Default target
.PHONY: all
all: $(TARGET)

# Build both main and test
.PHONY: all-with-test
all-with-test: $(TARGET) $(TEST_TARGET)

# Create directories
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Build main target
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Build test executable (standalone, no GStreamer dependencies)
$(TEST_TARGET): $(TEST_SOURCE) | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Test target
.PHONY: test
test: $(TEST_TARGET)
	@echo "Test executable built: $(TEST_TARGET)"

# Clean build files
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Install (optional)
.PHONY: install
install: $(TARGET)
	install -D $(TARGET) /usr/local/bin/$(PROJECT_NAME)

# Install test executable too
.PHONY: install-test
install-test: $(TEST_TARGET)
	install -D $(TEST_TARGET) /usr/local/bin/test_ard

# Uninstall (optional)
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(PROJECT_NAME)
	rm -f /usr/local/bin/test_ard

# Debug build
.PHONY: debug
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Debug test build
.PHONY: debug-test
debug-test: CFLAGS += -DDEBUG -g3
debug-test: $(TEST_TARGET)

# Show help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all           - Build the main project (default)"
	@echo "  all-with-test - Build both main project and test"
	@echo "  test          - Build only the test executable"
	@echo "  clean         - Remove build files"
	@echo "  debug         - Build main project with debug symbols"
	@echo "  debug-test    - Build test with debug symbols"
	@echo "  install       - Install main project to /usr/local/bin"
	@echo "  install-test  - Install test executable to /usr/local/bin"
	@echo "  uninstall     - Remove from /usr/local/bin"
	@echo "  help          - Show this help"

# Dependencies (only for main project objects)
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CPPFLAGS) -MM $< | sed 's|^.*:|$(OBJDIR)/$*.o:|' > $@
