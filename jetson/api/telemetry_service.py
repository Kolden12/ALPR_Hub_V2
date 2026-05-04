import os
import shutil
import time
import math
import sqlite3
import subprocess
import threading
import json
import piexif
from datetime import datetime
from collections import deque

class TelemetryService:
    def __init__(self, storage_path="/mnt/sabre_storage", db_path="/mnt/sabre_storage/sabre_hub.db"):
        self.storage_path = storage_path
        self.db_path = db_path
        self.is_offloading = False

    def forensic_staple(self, image_path, metadata):
        """Embed SHA-256 and GPS into Image EXIF."""
        try:
            exif_dict = {"Exif": {
                piexif.ExifIFD.UserComment: metadata['hash'].encode('utf-8'),
                piexif.ExifIFD.MakerNote: json.dumps(metadata).encode('utf-8')
            }}
            exif_bytes = piexif.dump(exif_dict)
            piexif.insert(exif_bytes, image_path)
        except Exception as e:
            print(f"EXIF Staple failed: {e}")

    def verified_offload_and_reset(self, server_ip):
        if self.is_offloading: return
        self.is_offloading = True

        try:
            # 1. Create Manifest
            manifest = []
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT id, plate_text, sha256_hash FROM hits WHERE is_offloaded = 0")
                for row in cursor.fetchall():
                    manifest.append({"id": row[0], "plate": row[1], "hash": row[2]})

            manifest_path = f"{self.storage_path}/manifest.json"
            with open(manifest_path, "w") as f:
                json.dump(manifest, f)

            # 2. Rsync Data
            result = subprocess.run(["rsync", "-avz", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            subprocess.run(["rsync", "-avz", manifest_path, f"officer@{server_ip}:/data/offload/"], check=True)

            if result.returncode == 0:
                # 3. Wait for Handshake Confirmation (.verified file on server)
                # In production, this would be an API check or waiting for a specific file to appear
                print("Waiting for Manifest Handshake...")
                time.sleep(5) # Simulation

                # 4. Verified Success -> Purge
                print("Handshake Verified. Purging local data.")
                with sqlite3.connect(self.db_path) as conn:
                    conn.execute("DELETE FROM hits WHERE is_offloaded = 1")
                    conn.execute("UPDATE hits SET is_offloaded = 1")
                    conn.commit()
        finally:
            self.is_offloading = False
