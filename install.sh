#!/bin/bash

# Raspberry Pi MIDI Sampler - Installation Script
# Installs the sampler as a SystemD service for auto-start

set -e

echo "Installing Raspberry Pi MIDI Sampler for user: $USER"

# Check if running as root (which should be avoided)
if [ "$USER" = "root" ]; then
    echo "Error: Please run this script as a regular user, not root"
    exit 1
fi

# Build the sampler
echo "Building sampler..."
make clean
make

# Create samples directory if it doesn't exist
SAMPLES_DIR="$HOME/samples"
if [ ! -d "$SAMPLES_DIR" ]; then
    echo "Creating samples directory..."
    mkdir -p "$SAMPLES_DIR"
    echo "Place your .wav sample files in $SAMPLES_DIR/"
fi

# Create dynamic service file
SERVICE_NAME="rpi-sampler-$USER"
SERVICE_FILE="/tmp/${SERVICE_NAME}.service"

echo "Creating SystemD service file..."
cat > "$SERVICE_FILE" << EOF
[Unit]
Description=Raspberry Pi MIDI Sampler ($USER)
After=sound.target network.target
Wants=sound.target

[Service]
Type=simple
User=$USER
Group=audio
WorkingDirectory=$PWD
ExecStart=$PWD/sampler
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Real-time priority settings
LimitRTPRIO=99
LimitMEMLOCK=infinity

# Environment
Environment=HOME=$HOME

[Install]
WantedBy=multi-user.target
EOF

# Copy service file to systemd
echo "Installing SystemD service..."
sudo cp "$SERVICE_FILE" /etc/systemd/system/
rm "$SERVICE_FILE"

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable "${SERVICE_NAME}.service"

echo "Installation complete!"
echo ""
echo "Service name: $SERVICE_NAME"
echo ""
echo "Commands:"
echo "  sudo systemctl start $SERVICE_NAME      # Start the service"
echo "  sudo systemctl stop $SERVICE_NAME       # Stop the service"
echo "  sudo systemctl status $SERVICE_NAME     # Check service status"
echo "  sudo journalctl -u $SERVICE_NAME -f     # View live logs"
echo ""
echo "The sampler will now start automatically on boot."
echo "To disable auto-start: sudo systemctl disable $SERVICE_NAME"