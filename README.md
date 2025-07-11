# Raspberry Pi MIDI Sampler

Real-time, low-latency MIDI sampler for Raspberry Pi with dual-channel operation.

## Dependencies

First, install required packages on your Raspberry Pi:

```bash
./setup.sh
```

## Quick Start

```bash
make
./sampler
```

## MIDI Device Scanner

List connected USB MIDI devices:

```bash
make midi
./list_midi
```

## Requirements

- Raspberry Pi 4 (2GB+ RAM)
- USB Audio Interface
- USB MIDI Interface
- High-speed SD card (Class 10+)

## Installation & Auto-Start

After running `setup.sh`, the `install.sh` script sets up the sampler to run on boot:

```bash
./install.sh
```

**What it does:**
- Builds the sampler executable
- Creates `~/samples/` directory for your audio files
- Installs SystemD service for auto-start on boot
- Configures real-time audio priority settings

**Run as your regular user** (not root) - the script adapts to any username.

## Configuration

Place samples in `~/samples/` directory (WAV format).

Edit `sampler_config_file.txt` for audio/MIDI settings.

## Features

- < 5ms audio latency
- Dual MIDI channels
- Real-time effects (filter, bit crusher, envelope)
- 42 samples via CC1 control
- GPIO status LED

## Service Management

```bash
sudo systemctl start rpi-sampler-$USER     # Start service
sudo systemctl stop rpi-sampler-$USER      # Stop service  
sudo systemctl status rpi-sampler-$USER    # Check status
sudo journalctl -u rpi-sampler-$USER -f    # View logs
```

Service name format: `rpi-sampler-{username}` (e.g., `rpi-sampler-john`)