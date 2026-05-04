#ifndef SABRE_PROTOCOL_H
#define SABRE_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sabre ALPR Hub UART Communication Protocol
 * Structure: [Header | Length | Command_ID | Payload | CRC16]
 */

#define SABRE_HEADER 0x5342 // "SB" (Sabre)

// Command IDs
typedef enum {
    CMD_HEARTBEAT      = 0x01, // 1Hz Pulse
    CMD_HIT_TRIGGER    = 0x02, // Jetson -> ESP32: Save color context
    CMD_CRITICAL_FLUSH = 0x03, // ESP32 -> Jetson: IMU > 4G impact event
    CMD_SYS_STATUS     = 0x04, // ESP32 -> Jetson: Voltage/Temp telemetry
    CMD_FAN_CONTROL    = 0x05, // Jetson -> ESP32: Target fan PWM
    CMD_SHUTDOWN_REQ   = 0x06  // ESP32 -> Jetson: Battery low / Ignition off
} sabre_cmd_t;

#pragma pack(push, 1)

typedef struct {
    uint16_t header;
    uint8_t  length;
    uint8_t  command_id;
} sabre_packet_header_t;

// Payload for CMD_SYS_STATUS
typedef struct {
    float voltage_3v3;
    float voltage_5v0;
    float voltage_48v;
    float battery_voltage;
    float temperature_c;
} sabre_status_payload_t;

// Payload for CMD_FAN_CONTROL
typedef struct {
    uint8_t fan_pwm_percent;
} sabre_fan_payload_t;

#pragma pack(pop)

/**
 * Hashing Protocol for Chain of Custody
 *
 * SHA-256 Signature is calculated on a concatenated string:
 * Format: "TIMESTAMP|PLATE_TEXT|VEHICLE_YMMV|GPS_LAT|GPS_LONG|JETSON_UUID"
 *
 * 1. Timestamp: ISO8601 (UTC)
 * 2. Plate: Raw string (or "UNKNOWN")
 * 3. YMMV: "Year Make Model Color"
 * 4. GPS: Decimal degrees (6 decimal places)
 * 5. UUID: Unique identifier for the Hub hardware
 */

#ifdef __cplusplus
}
#endif

#endif // SABRE_PROTOCOL_H
