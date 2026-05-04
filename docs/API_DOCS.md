# Sabre ALPR Hub - API & Protocol Documentation

## 1. UART Protocol (Inter-Processor)
**Baud Rate:** 115200, 8N1
**Format:** `[Header (2B) | Length (1B) | Command (1B) | Payload (NB) | CRC16 (2B)]`
**Header:** `0x5342` ("SB")

### Command IDs
- `0x01` (CMD_HEARTBEAT): Sent by Jetson to reset ESP32 Watchdog.
- `0x02` (CMD_HIT_TRIGGER): Sent by Jetson to trigger ESP32 PSRAM context save.
- `0x03` (CMD_CRITICAL_FLUSH): Sent by ESP32 to signal high-G event.
- `0x04` (CMD_SYS_STATUS): Sent by ESP32 with voltage/temp data.
- `0x05` (CMD_FAN_CONTROL): Sent by Jetson to set fan PWM (0-255).
- `0x06` (CMD_SHUTDOWN_REQ): Sent by ESP32 on battery critical or ignition off.
- `0x07` (CMD_CLEAR_INTERRUPT): Sent by Jetson to reset the hardware interrupt GPIO.

## 2. Hub API (Jetson to MDT)
The Hub exposes a REST API and a WebSocket for the MDT application.

### WebSocket: Real-time Alerts
**Endpoint:** `ws://192.168.1.10:8000/ws`
**Payload Example:**
```json
{
  "type": "HIT",
  "plate": "ABC-1234",
  "ymmv": "2022 FORD EXPLORER BLACK",
  "lat": 29.4241,
  "lon": -98.4936,
  "conf": 0.98
}
```

### REST Endpoints
- `GET /health`: System status and thermal levels.
- `POST /hotlist`: Update local SQLite hotlist table.
- `GET /logs`: Retrieve historical hits for shift reporting.
- `POST /config`: Update Station IP and Geofence coordinates.
