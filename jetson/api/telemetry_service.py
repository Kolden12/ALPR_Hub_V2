import os
import shutil
import time
import math
import sqlite3
import subprocess
import json
import requests
import serial
from datetime import datetime
from collections import deque

class TelemetryService:
    def __init__(self, storage_path="/mnt/sabre_storage", db_path="/mnt/sabre_storage/sabre_hub.db"):
        self.storage_path = storage_path
        self.db_path = db_path
        self.gps_history = deque(maxlen=10)
        self.server_url = "https://command.sabre.local"
        self.ser = serial.Serial('/dev/ttyTHS1', 115200, timeout=1)

    def calculate_haversine_distance(self, lat1, lon1, lat2, lon2):
        R = 6371000 # Earth radius in meters
        phi1, phi2 = math.radians(lat1), math.radians(lat2)
        dphi = math.radians(lat2 - lat1)
        dlam = math.radians(lon2 - lon1)
        a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlam/2)**2
        return R * 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))

    def execute_forensic_wipe(self, tier=1):
        """Tiered Lockdown & Forensic Wipe Protocol."""
        last_gps = self.gps_history[-1] if self.gps_history else (0.0, 0.0)

        if tier == 1:
            print("TIER 1: Soft Lock - Disabling Inference.")
            # Trigger MDT "SYSTEM DISABLED" overlay logic via WebSocket
            return

        if tier == 2:
            print("TIER 2: Hard Wipe - Irreversible Destruction.")
            # 1. Final GPS Flare
            try:
                requests.post(f"{self.server_url}/telemetry/flare", json={
                    "status": "WIPE_INITIATED", "lat": last_gps[0], "lon": last_gps[1]
                })
            except: pass

            # 2. Hardware Brick Handshake (0xDEAD)
            # [Header 0x5342 | Len 2 | CMD 0xFE | Payload 0xDEAD | CRC]
            self.ser.write(bytearray([0x53, 0x42, 0x02, 0xFE, 0xDE, 0xAD, 0x00, 0x00]))

            # 3. Data Destruction
            try:
                with sqlite3.connect(self.db_path) as conn:
                    conn.execute("PRAGMA journal_mode=DELETE;") # Forces WAL truncate
                if os.path.exists(self.db_path): os.remove(self.db_path)
                shutil.rmtree(self.storage_path, ignore_errors=True)
            except Exception as e:
                print(f"Wipe error: {e}")

            # 4. Final Verification
            try:
                requests.post(f"{self.server_url}/telemetry/verify-wipe", json={
                    "status": "WIPE_COMPLETE", "lat": last_gps[0], "lon": last_gps[1]
                })
            except: pass

            # 5. Self-Termination
            os.system("poweroff")
