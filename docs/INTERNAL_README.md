# Sabre ALPR Hub - Internal Engineering Specs

## Hardware Logic & Pin-Mapping

### 1. Jetson-to-ESP32 Interface
The primary communication is via UART (115200 Baud) and a dedicated hardware interrupt for "Black Box" events.

- **UART:** `/dev/ttyTHS1` (Jetson) <-> `UART0` (ESP32-P4)
- **Critical Flush Interrupt:**
  - **ESP32-P4:** GPIO 14 (Output, Active High)
  - **Jetson Orin Nano:** GPIO 421 (Input, Edge Triggered)
  - **Logic:** Triggered by ESP32 when IMU detects > 4G (configurable). Jetson must immediately flush SQLite WAL and sync file descriptors.

### 2. Networking (KSZ9897)
- **Subnet:** `192.168.1.0/24`
- **Cameras:** Assigned `.101` through `.104`.
- **Jetson:** Static `.10`.
- **ESP32:** Static `.11`.
- **MDT:** DHCP or Static `.50`.

### 3. Power Sequencing
1. **Stabilization:** ESP32 waits for battery > 12.6V.
2. **AI Boot:** ESP32 pulls `JETSON_POWER_EN` High.
3. **Shutdown:** If ignition is lost > 5 mins, ESP32 sends `CMD_SHUTDOWN_REQ` via UART. Jetson responds with acknowledgement after unmounting NVMe.

## Software Protocols

### Sabre Protocol (UART)
Defined in `shared/sabre_protocol.h`.
Packet: `[0x5342 (2B) | Length (1B) | CMD (1B) | Payload (NB) | CRC16 (2B)]`

### Hashing & Chain of Custody
Every plate hit is signed using SHA-256:
`Signature = SHA256(Timestamp + "|" + Plate + "|" + YMMV + "|" + GPS_Lat + "|" + GPS_Long + "|" + Hub_UUID)`

## Data Storage
- **Mount Point:** `/mnt/sabre_storage`
- **Format:** Ext4 with `noatime`
- **Purge Logic:** Service monitors disk usage; deletes oldest logs/crops when > 90% full.
