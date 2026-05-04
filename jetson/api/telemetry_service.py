import os
import shutil
import time
import math
import sqlite3
import subprocess
import threading
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

        # UART for Heartbeat to ESP32
        try:
            self.ser = serial.Serial('/dev/ttyTHS1', 115200, timeout=1)
        except Exception:
            self.ser = None
            print("UART Not found. Heartbeat disabled.")

        self.boot_integrity_check()
        threading.Thread(target=self.heartbeat_loop, daemon=True).start()

    def heartbeat_loop(self):
        """Send 1Hz Heartbeat to ESP32."""
        while True:
            if self.ser:
                # [Header 0x5342 | Len 0 | CMD 0x01 | CRC]
                packet = bytearray([0x53, 0x42, 0x00, 0x01])
                # Simplified CRC or pre-calculated for empty payload CMD_HEARTBEAT
                packet.extend([0x00, 0x00])
                self.ser.write(packet)
            time.sleep(1)

    def boot_integrity_check(self):
        try:
            with sqlite3.connect(self.db_path) as conn:
                conn.execute("PRAGMA journal_mode=WAL;")
                conn.execute("PRAGMA synchronous=NORMAL;")
                cursor = conn.cursor()
                cursor.execute("PRAGMA integrity_check;")
                if cursor.fetchone()[0] == "ok":
                    print("DB Integrity: OK.")
        except Exception as e:
            print(f"Boot check failed: {e}")

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

    def handle_thermal_load(self, temp_c):
        if temp_c >= 95:
            self.emergency_shutdown()
        elif temp_c >= 85:
            self.reduce_camera_fps(10)

    def verified_offload_and_reset(self, server_ip):
        if self.is_offloading: return
        self.is_offloading = True
        try:
            result = subprocess.run(["rsync", "-avz", "--quiet", f"{self.storage_path}/crops/", f"officer@{server_ip}:/data/offload/"], check=True)
            if result.returncode == 0:
                with sqlite3.connect(self.db_path) as conn:
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
        except Exception as e: print(f"Purge error: {e}")

if __name__ == "__main__":
    service = TelemetryService()
    while True:
        # Monitoring loop...
        time.sleep(10)
