#!/bin/bash

# Raspberry Pi MIDI Sampler - Dependency Setup Script
# Installs required packages for building the sampler

set -e

echo "=========================================="
echo "Raspberry Pi MIDI Sampler - Setup Script"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo "Error: Please run this script as a regular user (not root)"
    echo "The script will ask for sudo when needed."
    exit 1
fi

echo "Installing required dependencies..."
echo ""

# Update package lists
echo "Updating package lists..."
sudo apt update

# Install required packages
echo "Installing development packages..."
sudo apt install -y \
    build-essential \
    libjack-jackd2-dev \
    jackd2 \
    libsndfile1-dev \
    pkg-config

echo ""
echo "Verifying installations..."

# Check if packages were installed correctly
if pkg-config --exists jack; then
    echo "✓ JACK development libraries installed successfully"
else
    echo "✗ JACK installation failed"
    exit 1
fi

if pkg-config --exists sndfile; then
    echo "✓ libsndfile development libraries installed successfully"
else
    echo "✗ libsndfile installation failed"
    exit 1
fi

if command -v gcc >/dev/null 2>&1; then
    echo "✓ GCC compiler available"
else
    echo "✗ GCC compiler not found"
    exit 1
fi

if command -v make >/dev/null 2>&1; then
    echo "✓ Make build tool available"
else
    echo "✗ Make build tool not found"
    exit 1
fi

echo ""
echo "=========================================="
echo "Setup completed successfully!"
echo "=========================================="
echo ""
echo "Setting up JACK for audio..."
echo "Adding user to audio group (required for JACK)..."
sudo usermod -a -G audio $USER

echo ""
echo "==========================================="
echo "Setup completed successfully!"
echo "==========================================="
echo ""
echo "IMPORTANT: You need to log out and log back in for audio group changes to take effect."
echo ""
echo "Next steps:"
echo "1. Log out and log back in (for audio group)"
echo "2. Run './install.sh' to build and install the sampler"
echo "3. Use 'make midi && ./list_midi' to scan for MIDI devices"
echo ""