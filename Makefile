# Raspberry Pi MIDI Sampler - Makefile
# Professional build configuration

# Directories
SRC_DIR = src
BUILD_DIR = build

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -I$(SRC_DIR)
LIBS = -ljack -lasound -lsndfile -lpthread -lm

# For cross-compilation to Raspberry Pi (uncomment if needed)
# CC = arm-linux-gnueabihf-gcc
# CFLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard

# Target binaries
TARGET = sampler
MIDI_SCANNER = list_midi

# Source files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/midi.c $(SRC_DIR)/jack_client.c $(SRC_DIR)/audio_engine.c $(SRC_DIR)/sample_loader.c
MIDI_SOURCES = $(SRC_DIR)/list_midi.c

# Object files
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/midi.o $(BUILD_DIR)/jack_client.o $(BUILD_DIR)/audio_engine.o $(BUILD_DIR)/sample_loader.o
MIDI_OBJECTS = $(BUILD_DIR)/list_midi.o

# Default target
all: $(BUILD_DIR) $(TARGET) $(MIDI_SCANNER)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGET)

# Build the MIDI scanner utility
$(MIDI_SCANNER): $(MIDI_OBJECTS)
	$(CC) $(MIDI_OBJECTS) $(LIBS) -o $(MIDI_SCANNER)

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(MIDI_SCANNER)

# Install to system (for future use)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall from system
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Force rebuild
rebuild: clean all

# Build only MIDI scanner
midi: $(BUILD_DIR) $(MIDI_SCANNER)

.PHONY: all clean install uninstall rebuild midi