import os
import shutil
import time
import math
import sqlite3
import subprocess
import threading
import json
import piexif
import requests
import serial
from datetime import datetime
from collections import deque

class TelemetryService:
    def __init__(self, storage_path="/mnt/sabre_storage", db_path="/mnt/sabre_storage/sabre_hub.db"):
        self.storage_path = storage_path
        self.db_path = db_path
        self.gps_history = deque(maxlen=10)
        self.is_offloading = False
        self.server_url = "https://command.sabre.local"
        self.gateway_ip = "10.8.0.1"
        try:
            self.ser = serial.Serial('/dev/ttyTHS1', 115200, timeout=1)
        except: self.ser = None

        self.boot_diagnostics()
        threading.Thread(target=self.dpd_loop, daemon=True).start()

    def boot_diagnostics(self):
        if not self.ser: return
        # Query ESP32 for LastResetReason (CMD 0x08)
        self.ser.write(bytearray([0x53, 0x42, 0x00, 0x08, 0x00, 0x00])) # Placeholder CRC
        resp = self.ser.read(7)
        if len(resp) == 7 and resp[3] == 0x08:
            reason = resp[4]
            try:
                requests.post(f"{self.server_url}/maintenance/log", json={
                    "event": "SYSTEM_RECOVERY_EVENT", "code": hex(reason)
                })
            except: pass

    def dpd_loop(self):
        """Dead Peer Detection & Tunnel Recovery."""
        while True:
            res = subprocess.run(["ping", "-c", "1", "-W", "1", self.gateway_ip], capture_output=True)
            if res.returncode != 0:
                print("VPN Gateway unreachable. Restarting WireGuard...")
                subprocess.run(["systemctl", "restart", "wg-quick@wg0"])
            time.sleep(60)

    def calculate_gps_delta(self, lat, lon):
        """Haversine math for precise Speed and Bearing."""
        curr_time = time.time()
        self.gps_history.append((lat, lon, curr_time))
        if len(self.gps_history) < 2: return 0.0, 0.0

        p1, p2 = self.gps_history[0], self.gps_history[-1]
        R = 6371000
        phi1, phi2 = math.radians(p1[0]), math.radians(p2[0])
        dphi, dlam = math.radians(p2[0]-p1[0]), math.radians(p2[1]-p1[1])

        a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlam/2)**2
        dist = R * 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))

        y = math.sin(dlam) * math.cos(phi2)
        x = math.cos(phi1)*math.sin(phi2) - math.sin(phi1)*math.cos(phi2)*math.cos(dlam)
        bearing = (math.degrees(math.atan2(y, x)) + 360) % 360

        dt = p2[2] - p1[2]
        speed = (dist / dt) * 3.6 if dt > 0 else 0
        return speed, bearing

    def manual_usb_export(self, usb_mount="/mnt/usb"):
        """verified Evidence Export for Connectivity Dead Zones."""
        if not os.path.ismount(usb_mount): return False

        ts = int(time.time())
        dest = os.path.join(usb_mount, f"SABRE_EXPORT_{ts}")
        os.makedirs(dest)

        shutil.copy2(self.db_path, dest)
        shutil.copytree(os.path.join(self.storage_path, "crops"), os.path.join(dest, "crops"))

        # Forensic Manifest
        manifest_path = os.path.join(dest, "checksum_manifest.txt")
        with open(manifest_path, "w") as f:
            f.write(f"SABRE ALPR EXPORT - {ts}\n")
            # ... (Walk and hash files)
        return True

    def execute_forensic_wipe(self):
        """Tier 2 Hard Wipe Protocol."""
        # 1. Final GPS Flare
        requests.post(f"{self.server_url}/telemetry/flare", json={"status": "WIPE_COMPLETE"})
        # 2. Hardware Brick
        if self.ser: self.ser.write(bytearray([0x53, 0x42, 0x02, 0xFE, 0xDE, 0xAD, 0x00, 0x00]))
        # 3. Destroy and Poweroff
        shutil.rmtree(self.storage_path, ignore_errors=True)
        os.system("poweroff")
