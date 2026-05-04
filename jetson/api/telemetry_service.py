import os
import shutil
import time
import math
import sqlite3
import subprocess
import threading
import json
import piexif
import serial
from datetime import datetime
from collections import deque

class TelemetryService:
    def __init__(self, storage_path="/mnt/sabre_storage", db_path="/mnt/sabre_storage/sabre_hub.db"):
        self.storage_path = storage_path
        self.db_path = db_path
        self.is_offloading = False
        self.thermal_level = 0

    def handle_thermal_load(self, temp_c):
        """Thermal Protection: Warning threshold adjusted to 88°C for San Antonio."""
        if temp_c >= 95:
            self.emergency_shutdown()
        elif temp_c >= 88: # Operational shift to 88°C
            if self.thermal_level < 1:
                print("THERMAL LEVEL 1: SHEDDING LOAD.")
                self.thermal_level = 1
        else:
            self.thermal_level = 0

    def verified_offload_and_reset(self, server_ip):
        if self.is_offloading: return
        self.is_offloading = True

        manifest_id = int(time.time())
        try:
            # 1. Create Manifest
            manifest = []
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT id, sha256_hash FROM hits WHERE is_offloaded = 0")
                manifest = [{"id": r[0], "hash": r[1]} for r in cursor.fetchall()]

            manifest_path = f"{self.storage_path}/manifest_{manifest_id}.json"
            with open(manifest_path, "w") as f:
                json.dump(manifest, f)

            # 2. Rsync Transfer
            subprocess.run(["rsync", "-avz", "--quiet", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            subprocess.run(["rsync", "-avz", "--quiet", manifest_path, f"officer@{server_ip}:/data/offload/"], check=True)

            # 3. Production Handshake: Check for Server Receipt
            receipt_found = False
            for attempt in range(12): # Wait up to 60 seconds (5s * 12)
                check = subprocess.run(["ssh", f"officer@{server_ip}", f"ls /data/offload/receipt_{manifest_id}.json"], capture_output=True)
                if check.returncode == 0:
                    receipt_found = True
                    break
                time.sleep(5)

            if receipt_found:
                print(f"Receipt {manifest_id} verified. Purging local shift data.")
                with sqlite3.connect(self.db_path) as conn:
                    conn.execute("DELETE FROM hits WHERE is_offloaded = 1")
                    conn.execute("UPDATE hits SET is_offloaded = 1")
                    conn.commit()
            else:
                print(f"Handshake Timeout for manifest {manifest_id}. Data preserved.")

        except Exception as e:
            print(f"Offload Error: {e}")
        finally:
            self.is_offloading = False
            if os.path.exists(manifest_path): os.remove(manifest_path)
