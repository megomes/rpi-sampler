# Raspberry Pi MIDI Sampler

A real-time MIDI sampler for Raspberry Pi that transforms your Circuit Tracks into a powerful dual-channel chromatic sampler.  
Uses both MIDI tracks from the Circuit Tracks, mapping each to a separate chromatic sampler and routing them to the left and right audio outputs.

## Dependencies

First, install required packages on your Raspberry Pi:

```bash
./setup.sh
```

**Important**: Log out and log back in after setup for audio group permissions.

## Quick Start

```bash
make
./sampler
```

**That's it!** The sampler automatically:
- Tests multiple JACK configurations to find one that works
- Tries 48kHz, 44.1kHz, and different buffer sizes
- Works with USB audio, onboard audio, or plugin layer
- Connects to your audio outputs automatically
- No manual configuration needed!

## MIDI Device Scanner

List connected USB MIDI devices:

```bash
make midi
./list_midi
```

## Requirements

- Raspberry Pi 3/4 (1GB+ RAM for Pi 3, 2GB+ for Pi 4)
- JACK Audio Connection Kit
- USB MIDI Interface (or built-in audio for basic use)
- High-speed SD card (Class 10+)
- Audio interface (USB recommended, built-in 3.5mm supported)

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

Edit `audio_config.txt` for engine settings:
- `master_gain`: Overall volume (0.1-1.0)
- `max_voices`: Polyphony limit (2-16)
- `auto_gain_control`: Automatic volume scaling
- `jack_client_name`: JACK client identifier

## Features

- **Professional latency**: <5ms with JACK (vs 20-50ms with ALSA)
- **Zero underruns**: JACK handles all timing automatically
- **Polyphonic**: Up to 16 simultaneous samples
- **Sample rate conversion**: Automatic resampling to JACK rate
- **Auto-connect**: Connects to system outputs automatically
- **Modular architecture**: Separate JACK client and audio engine

## Manual JACK Commands (if needed)

```bash
# Check JACK status
jack_lsp                    # List JACK ports
jack_lsp -c                 # List connections
qjackctl                    # GUI control (if installed)

# Manual JACK start (if auto-start fails)
./start_jack.sh             # Use the auto-detection script
# OR
jackd -dalsa -dhw:0 -r48000 -p1024 -n2 &   # 48kHz standard

# Connect manually (if auto-connect fails)
jack_connect rpi-sampler:out_left system:playback_1
jack_connect rpi-sampler:out_right system:playback_2
```

## Service Management

```bash
sudo systemctl start rpi-sampler-$USER     # Start service
sudo systemctl stop rpi-sampler-$USER      # Stop service  
sudo systemctl status rpi-sampler-$USER    # Check status
sudo journalctl -u rpi-sampler-$USER -f    # View logs
```

Service name format: `rpi-sampler-{username}` (e.g., `rpi-sampler-john`)

## Troubleshooting

**"Failed to start JACK automatically"**:
- Check audio group: `groups` should include `audio`
- List audio devices: `aplay -l`
- Manual start: `./start_jack.sh`

**"Cannot connect JACK client"**:
- Kill existing JACK: `killall jackd`
- Check device permissions: `ls -l /dev/snd/`
- Try manual start: `jackd -dalsa -dhw:0 -r48000 -p1024 -n2 &`

**"No audio output"**:
- Check connections: `jack_lsp -c`
- Manual connect: `jack_connect rpi-sampler:out_left system:playback_1`