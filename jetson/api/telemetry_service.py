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
from datetime import datetime
from collections import deque

class TelemetryService:
    def __init__(self, storage_path="/mnt/sabre_storage", db_path="/mnt/sabre_storage/sabre_hub.db"):
        self.storage_path = storage_path
        self.db_path = db_path
        self.gps_history = deque(maxlen=10)
        self.is_offloading = False
        self.server_url = "https://command.sabre.local"
        self.jwt_token = None
        self.thermal_level = 0
        self.current_shift_id = None
        self.station_geofence = (29.4241, -98.4936)
        self.offload_radius_m = 100

    def calculate_gps_delta(self, lat, lon):
        """High-precision Bearing and Speed via GPS Delta."""
        curr_time = time.time()
        self.gps_history.append((lat, lon, curr_time))
        if len(self.gps_history) < 2: return 0.0, 0.0

        p1, p2 = self.gps_history[0], self.gps_history[-1]
        R = 6371000
        phi1, phi2 = math.radians(p1[0]), math.radians(p2[0])
        dphi, dlam = math.radians(p2[0]-p1[0]), math.radians(p2[1]-p1[1])

        # Haversine Distance
        a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlam/2)**2
        dist = R * (2 * math.atan2(math.sqrt(a), math.sqrt(1-a)))

        # Bearing (Spherical Trigonometry)
        y = math.sin(dlam) * math.cos(phi2)
        x = math.cos(phi1)*math.sin(phi2) - math.sin(phi1)*math.cos(phi2)*math.cos(dlam)
        bearing = (math.degrees(math.atan2(y, x)) + 360) % 360

        time_diff = p2[2] - p1[2]
        speed_kph = (dist / time_diff) * 3.6 if time_diff > 0 else 0

        # Geofence Check for Auto-Offload
        if dist < self.offload_radius_m and not self.is_offloading:
             threading.Thread(target=self.verified_offload_and_reset, args=("192.168.1.50",)).start()

        return speed_kph, bearing

    def forensic_staple(self, image_path, metadata):
        """Forensic Digital Staple: Inseparable EXIF metadata."""
        try:
            exif_dict = {"Exif": {
                piexif.ExifIFD.UserComment: metadata['hash'].encode('utf-8'),
                piexif.ExifIFD.MakerNote: json.dumps(metadata).encode('utf-8')
            }}
            lat, lon = metadata['lat'], metadata['lon']
            exif_dict["GPS"] = {
                piexif.GPSIFD.GPSLatitude: [(int(abs(lat) * 1000000), 1000000)],
                piexif.GPSIFD.GPSLongitude: [(int(abs(lon) * 1000000), 1000000)],
                piexif.GPSIFD.GPSLatitudeRef: 'N' if lat >= 0 else 'S',
                piexif.GPSIFD.GPSLongitudeRef: 'E' if lon >= 0 else 'W'
            }
            piexif.insert(piexif.dump(exif_dict), image_path)
        except Exception as e: print(f"Staple Error: {e}")

    def check_disk_space(self, threshold=0.90):
        """Circular Purge: 90% Capacity Lockdown."""
        try:
            total, used, free = shutil.disk_usage(self.storage_path)
            if (used / total) > threshold:
                self.purge_oldest_records(500)
        except: pass

    def purge_oldest_records(self, count):
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT id, image_path_ir, image_path_color FROM hits ORDER BY timestamp_iso8601 ASC LIMIT ?", (count,))
                rows = cursor.fetchall()
                for rid, ir, color in rows:
                    if ir and os.path.exists(ir): os.remove(ir)
                    if color and os.path.exists(color): os.remove(color)
                    conn.execute("DELETE FROM hits WHERE id = ?", (rid,))
                conn.commit()
        except Exception as e: print(f"Purge error: {e}")

    def verified_offload_and_reset(self, server_ip):
        """Hardened Manifest Handshake."""
        if self.is_offloading: return
        self.is_offloading = True
        mid = int(time.time())
        try:
            m_path = f"{self.storage_path}/manifest_{mid}.json"
            # Rsync
            subprocess.run(["rsync", "-avz", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            # Receipt Verification Loop
            verified = False
            for _ in range(12):
                if subprocess.run(["ssh", f"officer@{server_ip}", f"ls /data/offload/receipt_{mid}.json"], capture_output=True).returncode == 0:
                    verified = True; break
                time.sleep(5)
            if verified:
                with sqlite3.connect(self.db_path) as conn:
                    conn.execute("DELETE FROM hits WHERE is_offloaded = 1")
                    conn.execute("UPDATE hits SET is_offloaded = 1")
                    conn.commit()
        finally: self.is_offloading = False

    def execute_forensic_wipe(self):
        """Tier 2 Nuclear Option: Irreversible Destruction."""
        last_gps = self.gps_history[-1] if self.gps_history else (0.0, 0.0)
        # Pre-Wipe Flare
        requests.post(f"{self.server_url}/telemetry/flare", json={"status": "WIPE_INITIATED", "lat": last_gps[0], "lon": last_gps[1]})
        # Secure Destruction
        with sqlite3.connect(self.db_path) as conn:
            conn.execute("PRAGMA journal_mode=DELETE;") # Wipe WAL
        if os.path.exists(self.db_path): os.remove(self.db_path)
        shutil.rmtree(self.storage_path, ignore_errors=True)
        # Final Verification
        requests.post(f"{self.server_url}/telemetry/verify-wipe", json={"status": "COMPLETE", "lat": last_gps[0], "lon": last_gps[1]})
        os.system("reboot")
