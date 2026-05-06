# Sabre ALPR Hub V2

🚀 **Commercial-Grade Tactical ALPR Appliance**

The Sabre ALPR Hub is a high-performance, modular system designed for 24/7 law enforcement operations. It consists of an NVIDIA Jetson Orin Nano AI Engine, an ESP32-P4 System Guardian, and a WPF-based MDT application.

## User Guide & Installation

### 1. Jetson Deployment (Hub Board)
1. **Flash JetPack:** Ensure JetPack 6.x is installed on the Orin Nano.
2. **NVMe:** Install a PCIe Gen3 x4 NVMe SSD and mount to `/mnt/sabre_storage`.
3. **Deploy:**
   ```bash
   cd scripts
   chmod +x deploy_jetson.sh
   ./deploy_jetson.sh
   ```

### 2. ESP32-P4 Firmware (Guardian Board)
1. **Setup:** Install ESP-IDF v5.1+.
2. **Flash:**
   ```bash
   cd firmware
   idf.py build flash
   ```

### 3. MDT Application (Officer Terminal)
1. **Requirements:** Windows 10/11, GStreamer (MSVC 64-bit).
2. **First Run:** On initial launch, the "First Run Wizard" will appear.
   - Enter the **Station IP** for data offloading.
   - Set the **Home Geofence** (Latitude/Longitude).
   - Input **Officer and Vehicle IDs**.
3. **Shift Workflow:**
   - The Hub starts automatically with the vehicle.
   - View 4 live streams in a 2x2 grid.
   - "Hotlist" hits trigger high-contrast visual and auditory alerts.
   - At end of shift, click **"End Shift & Generate Report"**. The system will verify the offload before allowing a reset.

## System Resilience
- **Thermal Protection:** Automatically sheds load at 85°C; emergency shutdown at 95°C.
- **Power Management:** Waits for 12.6V battery stabilization before booting the AI domain.
- **Black Box:** IMU-triggered "Critical Flush" ensures data is saved during impact events.
- **Watchdog:** ESP32 hard-reboots Jetson if heartbeat is lost for 60 seconds.

## Legal Defensibility
- All hits are cryptographically signed using **SHA-256**.
- Verified Offload ensures no data is deleted until confirmed by the station server.
- SQLite WAL mode ensures database integrity during sudden power loss.
