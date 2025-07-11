#!/bin/bash

# Auto-start JACK script for Raspberry Pi Sampler
# Tries multiple configurations to find one that works

# Check if jackd is available
if ! command -v jackd >/dev/null 2>&1; then
    echo "Error: jackd not found. Install with: sudo apt install jackd2"
    exit 1
fi

# Kill any existing JACK processes
echo "Stopping any existing JACK processes..."
killall jackd 2>/dev/null || true
sleep 2

# Function to test a JACK configuration
test_jack_config() {
    local device=$1
    local rate=$2
    local buffer=$3
    local periods=$4
    
    echo "Testing: jackd -dalsa -d$device -r$rate -p$buffer -n$periods -s"
    
    # Try to start JACK with this config
    jackd -dalsa -d"$device" -r"$rate" -p"$buffer" -n"$periods" -s &
    local jack_pid=$!
    
    # Wait a moment to see if it works
    sleep 4
    
    if ps -p $jack_pid > /dev/null 2>&1; then
        echo "SUCCESS: JACK running with $device at $rate Hz"
        return 0
    else
        echo "FAILED: $device at $rate Hz not supported"
        return 1
    fi
}

echo "Auto-detecting working JACK configuration..."
echo ""

# Try different configurations in order of preference
# Format: device sample_rate buffer_size periods

CONFIGS=(
    "hw:0 48000 1024 2"    # Pi onboard 48kHz
    "hw:1 48000 1024 2"    # USB audio 48kHz  
    "hw:0 44100 1024 2"    # Pi onboard 44.1kHz
    "hw:1 44100 1024 2"    # USB audio 44.1kHz
    "hw:0 48000 512 2"     # Pi onboard smaller buffer
    "hw:1 48000 512 2"     # USB audio smaller buffer
    "hw:0 22050 1024 2"    # Pi onboard low quality
    "plughw:0 48000 1024 2" # Pi with plugin layer
    "plughw:1 48000 1024 2" # USB with plugin layer
)

for config in "${CONFIGS[@]}"; do
    read -r device rate buffer periods <<< "$config"
    
    echo "Trying: $device at $rate Hz, buffer $buffer, $periods periods"
    
    if test_jack_config "$device" "$rate" "$buffer" "$periods"; then
        echo ""
        echo "✓ JACK started successfully!"
        echo "  Device: $device"
        echo "  Sample rate: $rate Hz"
        echo "  Buffer: $buffer frames"
        echo "  Periods: $periods"
        exit 0
    fi
    
    # Kill the failed attempt
    killall jackd 2>/dev/null || true
    sleep 1
done

echo ""
echo "✗ All JACK configurations failed!"
echo ""
echo "Manual troubleshooting:"
echo "1. Check audio devices: aplay -l"
echo "2. Check device permissions: ls -l /dev/snd/"
echo "3. Try manual: jackd -dalsa -dplughw:0 -r44100 -p1024 -n2"
echo ""
exit 1