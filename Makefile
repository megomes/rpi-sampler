# Raspberry Pi MIDI Sampler - Makefile
# Basic Hello World build configuration

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LIBS = -lasound

# For cross-compilation to Raspberry Pi (uncomment if needed)
# CC = arm-linux-gnueabihf-gcc
# CFLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard

# Target binaries
TARGET = sampler
MIDI_SCANNER = list_midi

# Source files
SOURCES = main.c
MIDI_SOURCES = list_midi.c

# Object files
OBJECTS = $(SOURCES:.c=.o)
MIDI_OBJECTS = $(MIDI_SOURCES:.c=.o)

# Default target
all: $(TARGET) $(MIDI_SCANNER)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Build the MIDI scanner utility
$(MIDI_SCANNER): $(MIDI_OBJECTS)
	$(CC) $(MIDI_OBJECTS) $(LIBS) -o $(MIDI_SCANNER)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(MIDI_OBJECTS) $(TARGET) $(MIDI_SCANNER)

# Install to system (for future use)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall from system
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Force rebuild
rebuild: clean all

# Build only MIDI scanner
midi: $(MIDI_SCANNER)

.PHONY: all clean install uninstall rebuild midi