# Raspberry Pi MIDI Sampler

Real-time, low-latency MIDI sampler for Raspberry Pi with dual-channel operation.

## Quick Start

```bash
make
./sampler
```

## Requirements

- Raspberry Pi 4 (2GB+ RAM)
- USB Audio Interface
- USB MIDI Interface
- High-speed SD card (Class 10+)

## Configuration

Edit `sampler_config_file.txt` for audio/MIDI settings.

Place samples in `/home/pi/samples/` directory (WAV format).

## Features

- < 5ms audio latency
- Dual MIDI channels
- Real-time effects (filter, bit crusher, envelope)
- 42 samples via CC1 control
- GPIO status LED

---

See `raspberry_pi_sampler_spec.md` for detailed specifications.