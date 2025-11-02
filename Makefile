# Makefile for TP_CRIPTO project

# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -O2
LDFLAGS = -l crypto

# Project name
TARGET = stegobmp

# Source files
SOURCES = $(wildcard *.c) \
       $(wildcard cryptography/*.c) \
       $(wildcard steganography/*.c)

# Object files (generated from source files)
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Clean everything including backup files
distclean: clean
	rm -f *~ *.bak

# Install (optional - copies executable to /usr/local/bin)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Run the program (you may need to specify input file)
run: $(TARGET)
	./$(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  debug     - Build with debug flags"
	@echo "  release   - Build optimized version"
	@echo "  run       - Build and run the program"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this help message"

# Declare phony targets
.PHONY: all clean distclean install uninstall debug release run help

# Dependencies (optional - helps with incremental builds)
main.o: main.c
bmp_lib.o: bmp_lib.c
parser.o: parser.c
handlers.o: handlers.c
steganography/steganography.o: steganography/steganography.c
steganography/embed_utils.o: steganography/embed_utils.c
