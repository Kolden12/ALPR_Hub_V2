# Sabre ALPR Hub V2

🚀 **Commercial-Grade Tactical ALPR Appliance**

The Sabre ALPR Hub is a high-performance, modular system designed for 24/7 law enforcement and professional patrol operations. This repository contains the full software ecosystem for the 3-board modular appliance.

## System Architecture

- **AI Engine (Jetson Orin Nano Super):** Handles dual-stream IR ALPR, YMMV classification, and GPS-based telemetry.
- **System Guardian (ESP32-P4):** Manages power sequencing, thermal PID loops, and high-G "Black Box" event handling.
- **MDT Application (Windows WPF):** Professional officer terminal for live monitoring, hit alerts, and system configuration.

## Quick Start

### Jetson Deployment
1. Ensure NVIDIA Container Toolkit is installed.
2. Mount your NVMe SSD to `/mnt/sabre_storage`.
3. Run the stack:
   ```bash
   cd jetson
   docker-compose up -d
   ```

### Firmware (ESP32-P4)
1. Install ESP-IDF v5.x.
2. Build and flash:
   ```bash
   cd firmware
   idf.py build flash
   ```

### MDT Application
1. Install GStreamer (MSVC 64-bit).
2. Run `scripts/check_mdt_env.ps1` to verify the environment.
3. Open `mdt/SabreMDT/SabreMDT.sln` in Visual Studio 2022 and run.

## Hardware Specs
- **Jetson:** Orin Nano Super (8GB)
- **MCU:** ESP32-P4 (RISC-V 400MHz)
- **Switch:** KSZ9897 7-port
- **Storage:** PCIe Gen3 x4 NVMe
- **Connectivity:** M.2 LTE/GPS + 4x RTSP Camera Streams
