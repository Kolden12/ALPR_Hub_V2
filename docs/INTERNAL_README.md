# Sabre ALPR Hub - Internal Engineering Specs

## Hardware Logic & Pin-Mapping

### 1. Jetson-to-ESP32 Interface
The primary communication is via UART (115200 Baud) and a dedicated hardware interrupt for "Black Box" events.

- **UART:** `/dev/ttyTHS1` (Jetson) <-> `UART0` (ESP32-P4)
- **Critical Flush Interrupt:**
  - **ESP32-P4:** GPIO 14 (Output, Active High)
  - **Jetson Orin Nano:** GPIO 421 (Input, Edge Triggered)
- **System Reset (Watchdog):**
  - **ESP32-P4:** GPIO 18 (Output, Active Low)
  - **Jetson Orin Nano:** SYS_RESET (Dedicated Reset Pin)

### 2. Samtec Bridge Pinout (CORE to NPB)
| Pin | Signal | Direction | Function |
|---|---|---|---|
| 1 | 48V_IN | NPB -> CORE | PoE Power In |
| 5 | UART_TX | CORE -> NPB | Debug Console |
| 7 | UART_RX | NPB -> CORE | Debug Console |
| 12 | GPIO_14 | NPB -> CORE | Critical Flush |
| 15 | GPIO_18 | NPB -> CORE | Watchdog Reset |
| 20 | CAN_H | NPB <-> CORE | OBD-II Link |
| 22 | CAN_L | NPB <-> CORE | OBD-II Link |

### 3. GPIO Mapping (Final Verified)
- **Jetson (Power En):** GPIO 12
- **ESP32 (PWM Fan):** GPIO 15
- **ESP32 (Ignition Sense):** GPIO 16
- **ESP32 (IMU Interrupt):** GPIO 17
- **ESP32 (Watchdog Out):** GPIO 18
