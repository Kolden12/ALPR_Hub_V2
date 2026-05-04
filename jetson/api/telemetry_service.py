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

    def calculate_gps_delta(self, lat, lon):
        current_time = time.time()
        self.gps_history.append((lat, lon, current_time))

        dist_to_station = self.calculate_distance(lat, lon, self.station_geofence[0], self.station_geofence[1])
        if dist_to_station < self.offload_radius_m and not self.is_offloading:
            threading.Thread(target=self.trigger_station_offload, args=("192.168.1.50",)).start()

        if len(self.gps_history) < 2: return 0.0, 0.0
        p1, p2 = self.gps_history[0], self.gps_history[-1]
        distance = self.calculate_distance(p1[0], p1[1], p2[0], p2[1])
        time_diff = p2[2] - p1[2]
        if time_diff <= 0: return 0.0, 0.0
        return (distance / time_diff) * 3.6, 0.0

    def check_disk_space(self, threshold=0.90):
        """Circular Purge: Deletes oldest records if disk usage exceeds threshold."""
        total, used, free = shutil.disk_usage(self.storage_path)
        if (used / total) > threshold:
            print(f"Disk usage exceeded {threshold*100}%. Initiating purge...")
            self.purge_oldest_records(500)

    def purge_oldest_records(self, count):
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT id, image_path_ir, image_path_color FROM hits ORDER BY timestamp_iso8601 ASC LIMIT ?", (count,))
                rows = cursor.fetchall()

                for row_id, ir, color in rows:
                    if ir and os.path.exists(ir): os.remove(ir)
                    if color and os.path.exists(color): os.remove(color)
                    cursor.execute("DELETE FROM hits WHERE id = ?", (row_id,))
                conn.commit()
            print(f"Purged {len(rows)} records.")
        except Exception as e: print(f"Purge error: {e}")

    def trigger_station_offload(self, server_ip):
        if self.is_offloading: return
        self.is_offloading = True
        try:
            print(f"Station Offload triggered to {server_ip}...")
            subprocess.run(["rsync", "-avz", "--remove-source-files", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            with sqlite3.connect(self.db_path) as conn:
                conn.execute("UPDATE hits SET is_offloaded = 1 WHERE is_offloaded = 0")
        except Exception as e: print(f"Offload error: {e}")
        finally:
            self.is_offloading = False

if __name__ == "__main__":
    service = TelemetryService()
    while True:
        service.check_disk_space()
        time.sleep(60)
