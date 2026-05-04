#!/bin/bash
# Sabre ALPR Hub - Jetson Deployment Script

set -e

echo "🚀 Starting Sabre Hub Deployment..."

# 1. Update and Install Dependencies
sudo apt-get update
sudo apt-get install -y docker-compose sqlite3 rsync openssh-server

# 2. Configure NVMe Storage
STORAGE_PATH="/mnt/sabre_storage"
if [ ! -d "$STORAGE_PATH" ]; then
    echo "Creating storage directory..."
    sudo mkdir -p "$STORAGE_PATH"
    sudo chown $USER:$USER "$STORAGE_PATH"
fi

# 3. Setup SQLite Database
DB_PATH="$STORAGE_PATH/sabre_hub.db"
if [ ! -f "$DB_PATH" ]; then
    echo "Initializing SQLite Database..."
    sqlite3 "$DB_PATH" < ../shared/schema.sql
fi

# 4. Install systemd services
echo "Installing services..."
sudo cp ../scripts/sabre_hub.service /etc/systemd/system/
sudo cp ../scripts/sabre_telemetry.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable sabre_hub
sudo systemctl enable sabre_telemetry

echo "✅ Deployment Complete. Restarting services..."
sudo systemctl restart sabre_telemetry
sudo systemctl restart sabre_hub

echo "Hub is now active. Monitor logs with: journalctl -u sabre_hub -f"
