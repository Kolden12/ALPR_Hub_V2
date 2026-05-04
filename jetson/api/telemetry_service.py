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
        self.thermal_level = 0 # 0: Normal, 1: Shedding, 2: Emergency

    def handle_thermal_load(self, temp_c):
        """San Antonio Heat Protection Logic."""
        if temp_c >= 95:
            if self.thermal_level < 2:
                print("THERMAL LEVEL 2: EMERGENCY SHUTDOWN.")
                self.thermal_level = 2
                self.emergency_shutdown()
        elif temp_c >= 85:
            if self.thermal_level < 1:
                print("THERMAL LEVEL 1: SHEDDING LOAD (Drop FPS).")
                self.thermal_level = 1
                self.reduce_camera_fps(10)
        else:
            if self.thermal_level > 0:
                print("THERMAL NORMALIZED: Restoring performance.")
                self.thermal_level = 0
                self.reduce_camera_fps(30)

    def reduce_camera_fps(self, fps):
        """Signals cameras/DeepStream to adjust frame rate."""
        # Logic to send API call to IP cameras or update DeepStream config
        pass

    def emergency_shutdown(self):
        """Graceful data preservation before power cut."""
        with sqlite3.connect(self.db_path) as conn:
            conn.execute("PRAGMA wal_checkpoint(FULL);")
        # Signal ESP32 via UART to cut power after 10s
        os.system("shutdown -h now")

    def verified_offload_and_reset(self, server_ip):
        if self.is_offloading: return
        self.is_offloading = True
        try:
            # RC=0 Strict verification
            result = subprocess.run(["rsync", "-avz", "--quiet", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            if result.returncode == 0:
                with sqlite3.connect(self.db_path) as conn:
                    cursor = conn.cursor()
                    cursor.execute("SELECT image_path_ir, image_path_color FROM hits WHERE is_offloaded = 1")
                    for ir, color in cursor.fetchall():
                        if ir and os.path.exists(ir): os.remove(ir)
                        if color and os.path.exists(color): os.remove(color)
                    conn.execute("DELETE FROM hits WHERE is_offloaded = 1")
                    conn.execute("UPDATE hits SET is_offloaded = 1")
                    conn.commit()
        finally:
            self.is_offloading = False

    def check_disk_space(self, threshold=0.90):
        total, used, free = shutil.disk_usage(self.storage_path)
        if (used / total) > threshold:
            self.purge_oldest_records(500)

    def purge_oldest_records(self, count):
        # Implementation from Phase 2/3
        pass

if __name__ == "__main__":
    service = TelemetryService()
    while True:
        # Get actual Jetson temp: /sys/class/thermal/thermal_zone0/temp
        with open("/sys/class/thermal/thermal_zone0/temp", "r") as f:
            temp = int(f.read()) / 1000.0
            service.handle_thermal_load(temp)

        service.check_disk_space()
        time.sleep(10)
