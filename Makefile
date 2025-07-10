# Raspberry Pi MIDI Sampler - Makefile
# Basic Hello World build configuration

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99

# For cross-compilation to Raspberry Pi (uncomment if needed)
# CC = arm-linux-gnueabihf-gcc
# CFLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard

# Target binary
TARGET = sampler

# Source files
SOURCES = main.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install to system (for future use)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall from system
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Force rebuild
rebuild: clean all

.PHONY: all clean install uninstall rebuild