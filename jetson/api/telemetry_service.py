import os
import shutil
import time
import math
import sqlite3
import subprocess
import threading
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

    def calculate_distance(self, lat1, lon1, lat2, lon2):
        R = 6371000
        phi1, phi2 = math.radians(lat1), math.radians(lat2)
        dphi = math.radians(lat2 - lat1)
        dlambda = math.radians(lon2 - lon1)
        a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlambda/2)**2
        return R * (2 * math.atan2(math.sqrt(a), math.sqrt(1-a)))

    def calculate_bearing(self, lat1, lon1, lat2, lon2):
        phi1, phi2 = math.radians(lat1), math.radians(lat2)
        dlambda = math.radians(lon2 - lon1)
        y = math.sin(dlambda) * math.cos(phi2)
        x = math.cos(phi1) * math.sin(phi2) - math.sin(phi1) * math.cos(phi2) * math.cos(dlambda)
        return (math.degrees(math.atan2(y, x)) + 360) % 360

    def update_gps(self, lat, lon):
        current_time = time.time()
        self.gps_history.append((lat, lon, current_time))

        dist_to_station = self.calculate_distance(lat, lon, self.station_geofence[0], self.station_geofence[1])
        if dist_to_station < self.offload_radius_m and not self.is_offloading:
            threading.Thread(target=self.verified_offload_and_reset, args=("192.168.1.50",)).start()

        if len(self.gps_history) < 2: return 0.0, 0.0
        p1, p2 = self.gps_history[0], self.gps_history[-1]

        distance = self.calculate_distance(p1[0], p1[1], p2[0], p2[1])
        time_diff = p2[2] - p1[2]
        if time_diff <= 0: return 0.0, 0.0

        speed_kph = (distance / time_diff) * 3.6
        bearing = self.calculate_bearing(p1[0], p1[1], p2[0], p2[1])

        return speed_kph, bearing

    def verified_offload_and_reset(self, server_ip):
        if self.is_offloading: return
        self.is_offloading = True
        print(f"Verified Offload Initiated to {server_ip}...")

        try:
            # 1. SFTP/Rsync Transfer
            result = subprocess.run(["rsync", "-avz", "--quiet", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], capture_output=True)

            if result.returncode == 0:
                print("Offload Verification: SUCCESS. Initiating Purge.")
                with sqlite3.connect(self.db_path) as conn:
                    cursor = conn.cursor()
                    # 2. Get IDs of offloaded records
                    cursor.execute("SELECT image_path_ir, image_path_color FROM hits WHERE is_offloaded = 1")
                    to_delete = cursor.fetchall()

                    # 3. Delete Physical Files
                    for ir, color in to_delete:
                        if ir and os.path.exists(ir): os.remove(ir)
                        if color and os.path.exists(color): os.remove(color)

                    # 4. Cleanup DB
                    conn.execute("DELETE FROM hits WHERE is_offloaded = 1")
                    # Mark current as offloaded
                    conn.execute("UPDATE hits SET is_offloaded = 1")
                    conn.commit()
            else:
                print(f"Offload Verification: FAILED (RC={result.returncode}). Data preserved.")

        except Exception as e:
            print(f"Offload Error: {e}")
        finally:
            self.is_offloading = False

    def check_disk_space(self, threshold=0.90):
        total, used, free = shutil.disk_usage(self.storage_path)
        if (used / total) > threshold:
            self.purge_oldest_records(500)

    def purge_oldest_records(self, count):
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT id, image_path_ir, image_path_color FROM hits ORDER BY timestamp_iso8601 ASC LIMIT ?", (count,))
                for row_id, ir, color in cursor.fetchall():
                    if ir and os.path.exists(ir): os.remove(ir)
                    if color and os.path.exists(color): os.remove(color)
                    cursor.execute("DELETE FROM hits WHERE id = ?", (row_id,))
        except Exception as e: print(f"Purge error: {e}")

if __name__ == "__main__":
    service = TelemetryService()
    while True:
        service.check_disk_space()
        time.sleep(60)
