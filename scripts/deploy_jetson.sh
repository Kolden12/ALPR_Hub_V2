#!/bin/bash
# Sabre ALPR Hub - Jetson Deployment Script (Golden Master)

set -e

echo "🚀 Starting Sabre Hub Deployment..."

# 1. Update and Install Dependencies
sudo apt-get update
sudo apt-get install -y docker-compose sqlite3 rsync openssh-server python3-pip

# 2. Install Forensic Metadata Tools
pip3 install piexif

# 3. Configure NVMe Storage
STORAGE_PATH="/mnt/sabre_storage"
sudo mkdir -p "$STORAGE_PATH/crops"
sudo chown -R $USER:$USER "$STORAGE_PATH"

# 4. Setup SQLite Database
DB_PATH="$STORAGE_PATH/sabre_hub.db"
if [ ! -f "$DB_PATH" ]; then
    sqlite3 "$DB_PATH" < ../shared/schema.sql
fi

# 5. Install systemd services
sudo cp ../scripts/sabre_hub.service /etc/systemd/system/
sudo cp ../scripts/sabre_telemetry.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable sabre_hub
sudo systemctl enable sabre_telemetry

echo "✅ Deployment Complete."
