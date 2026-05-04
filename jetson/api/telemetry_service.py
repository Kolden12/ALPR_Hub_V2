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
        self.gps_history = deque(maxlen=5)
        self.station_geofence = (29.4241, -98.4936)
        self.offload_radius_m = 100
        self.is_offloading = False
        self.thermal_level = 0
        self.current_shift_id = None

    def forensic_staple(self, image_path, metadata):
        """Embed SHA-256 and GPS into Image EXIF/XMP before final commit."""
        try:
            exif_dict = {"Exif": {
                piexif.ExifIFD.UserComment: metadata['hash'].encode('utf-8'),
                piexif.ExifIFD.MakerNote: json.dumps(metadata).encode('utf-8')
            }}
            # GPS Lat/Long conversion to Rational
            lat = metadata['lat']
            lon = metadata['lon']
            exif_dict["GPS"] = {
                piexif.GPSIFD.GPSLatitude: [(int(abs(lat) * 1000000), 1000000)],
                piexif.GPSIFD.GPSLongitude: [(int(abs(lon) * 1000000), 1000000)],
                piexif.GPSIFD.GPSLatitudeRef: 'N' if lat >= 0 else 'S',
                piexif.GPSIFD.GPSLongitudeRef: 'E' if lon >= 0 else 'W'
            }
            exif_bytes = piexif.dump(exif_dict)
            piexif.insert(exif_bytes, image_path)
        except Exception as e:
            print(f"Forensic Staple failed: {e}")

    def verified_offload_and_reset(self, server_ip):
        """Hardened Manifest Handshake: Verify receipt on server before local purge."""
        if self.is_offloading: return
        self.is_offloading = True

        manifest_id = int(time.time())
        try:
            # 1. Generate Manifest
            manifest = []
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT id, sha256_hash FROM hits WHERE is_offloaded = 0")
                manifest = [{"id": r[0], "hash": r[1]} for r in cursor.fetchall()]

            if not manifest:
                self.is_offloading = False
                return

            manifest_path = f"{self.storage_path}/manifest_{manifest_id}.json"
            with open(manifest_path, "w") as f:
                json.dump(manifest, f)

            # 2. Transfer Data & Manifest
            subprocess.run(["rsync", "-avz", "--quiet", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            subprocess.run(["rsync", "-avz", "--quiet", manifest_path, f"officer@{server_ip}:/data/offload/"], check=True)

            # 3. Handshake: Wait for Server Receipt
            verified = False
            for _ in range(12): # 60s timeout
                check = subprocess.run(["ssh", f"officer@{server_ip}", f"ls /data/offload/receipt_{manifest_id}.json"], capture_output=True)
                if check.returncode == 0:
                    verified = True
                    break
                time.sleep(5)

            if verified:
                print(f"Offload Verified for {manifest_id}. Purging records.")
                with sqlite3.connect(self.db_path) as conn:
                    # Cleanup old offloaded files
                    cursor = conn.cursor()
                    cursor.execute("SELECT image_path_ir, image_path_color FROM hits WHERE is_offloaded = 1")
                    for ir, color in cursor.fetchall():
                        if ir and os.path.exists(ir): os.remove(ir)
                        if color and os.path.exists(color): os.remove(color)

                    conn.execute("DELETE FROM hits WHERE is_offloaded = 1")
                    conn.execute("UPDATE hits SET is_offloaded = 1")
                    conn.commit()
            else:
                print(f"Offload verification timeout for {manifest_id}. Data preserved.")
        finally:
            self.is_offloading = False
            if os.path.exists(manifest_path): os.remove(manifest_path)

    def calculate_gps_delta(self, lat, lon):
        """Spherical trigonometry for Bearing (0-359) and Speed."""
        curr_time = time.time()
        self.gps_history.append((lat, lon, curr_time))
        if len(self.gps_history) < 2: return 0.0, 0.0

        p1, p2 = self.gps_history[0], self.gps_history[-1]
        # Haversine Distance
        R = 6371000
        phi1, phi2 = math.radians(p1[0]), math.radians(p2[0])
        dphi = math.radians(p2[0]-p1[0])
        dlam = math.radians(p2[1]-p1[1])
        a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlam/2)**2
        dist = R * (2 * math.atan2(math.sqrt(a), math.sqrt(1-a)))

        # Bearing
        y = math.sin(dlam) * math.cos(phi2)
        x = math.cos(phi1)*math.sin(phi2) - math.sin(phi1)*math.cos(phi2)*math.cos(dlam)
        bearing = (math.degrees(math.atan2(y, x)) + 360) % 360

        speed_kph = (dist / (p2[2]-p1[2])) * 3.6 if (p2[2]-p1[2]) > 0 else 0
        return speed_kph, bearing
