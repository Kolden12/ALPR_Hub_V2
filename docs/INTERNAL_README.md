# Sabre ALPR Hub - Internal Engineering Specs

## Hardware Logic & Pin-Mapping

### 1. Power Distribution (High-Current)
Power is strictly separated from logic signals to prevent noise and ensure reliability in extreme heat.

- **PMB → NPB (Network Board):** Samtec 4-blade terminal.
  - **Blades 1-2:** 12V Rail (System Logic)
  - **Blades 3-4:** 48V Rail (High-Power PoE Cameras)
- **PMB → CORE (Jetson Board):** Samtec 2-blade terminal.
  - **Blades 1-2:** 12V Rail (AI Engine)
- **Local Step-downs:** 3.3V and 5.0V regulation happens locally on the NPB and CORE boards.

### 2. Samtec 40-Pin Data Bridge (CORE to NPB)
Strictly for high-speed differential pairs and low-voltage logic. No power rails.

| Pin | Signal | Direction | Function |
|---|---|---|---|
| 1-4 | NC | - | RESERVED |
| 5 | UART_TX | CORE -> NPB | Debug Console |
| 7 | UART_RX | NPB -> CORE | Debug Console |
| 12 | GPIO_14 | NPB -> CORE | Critical Flush Interrupt |
| 15 | GPIO_18 | NPB -> CORE | Watchdog Reset (Active Low) |
| 20 | CAN_H | NPB <-> CORE | OBD-II Link |
| 22 | CAN_L | NPB <-> CORE | OBD-II Link |
| 25-32 | MDI_P/N | CORE <-> NPB | PHY-to-PHY Ethernet Link |

### 3. GPIO Mapping (Final Verified)
- **Jetson (Power En):** GPIO 12
- **ESP32 (PWM Fan):** GPIO 15
- **ESP32 (Ignition Sense):** GPIO 16
- **ESP32 (IMU Interrupt):** GPIO 17
- **ESP32 (Watchdog Out):** GPIO 18
- **ESP32 (Reset Out):** GPIO 18 -> Jetson SYS_RESET
