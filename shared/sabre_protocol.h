#ifndef SABRE_PROTOCOL_H
#define SABRE_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

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

// Full Packet Structure (Conceptual for size)
typedef struct {
    sabre_packet_header_t header;
    uint8_t payload[255];
    uint16_t crc;
} sabre_packet_t;

#pragma pack(pop)

/**
 * CRC16-CCITT calculation (Poly: 0x1021, Init: 0xFFFF)
 */
static inline uint16_t sabre_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

#ifdef __cplusplus
}
#endif

#endif // SABRE_PROTOCOL_H
