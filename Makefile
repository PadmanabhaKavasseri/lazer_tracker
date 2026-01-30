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

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/$(PROJECT_NAME)

# Default target
.PHONY: all
all: $(TARGET)

# Create directories
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Build target
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Clean build files
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Install (optional)
.PHONY: install
install: $(TARGET)
	install -D $(TARGET) /usr/local/bin/$(PROJECT_NAME)

# Uninstall (optional)
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(PROJECT_NAME)

# Debug build
.PHONY: debug
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Show help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all      - Build the project (default)"
	@echo "  clean    - Remove build files"
	@echo "  debug    - Build with debug symbols"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  help     - Show this help"

# Dependencies
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CPPFLAGS) -MM $< | sed 's|^.*:|$(OBJDIR)/$*.o:|' > $@
