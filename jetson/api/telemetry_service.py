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
        self.ser = serial.Serial('/dev/ttyTHS1', 115200, timeout=1)

    def execute_forensic_wipe(self):
        """Tier 2 'Nuclear Option' with Physical Hardware Handover."""
        print("!!! FORENSIC WIPE INITIATED !!!")

        # 1. Signal ESP32 to Brick Hardware (0xDEAD)
        # [Header 0x5342 | Len 2 | CMD 0xFE (WIPE) | Payload 0xDEAD | CRC]
        wipe_packet = bytearray([0x53, 0x42, 0x02, 0xFE, 0xDE, 0xAD, 0x00, 0x00])
        self.ser.write(wipe_packet)
        time.sleep(1) # Ensure UART transmission

        # 2. Secure Data Destruction
        with sqlite3.connect(self.db_path) as conn:
            conn.execute("PRAGMA journal_mode=DELETE;")
        if os.path.exists(self.db_path): os.remove(self.db_path)
        shutil.rmtree(self.storage_path, ignore_errors=True)

        # 3. Final Flare and Terminate
        os.system("shutdown -h now")
