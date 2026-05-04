# Sabre ALPR Hub - Internal Engineering Specs

## Hardware Logic & Pin-Mapping

### 1. Power Distribution (High-Current Terminal Blades)
Power is strictly separated from the 40-pin data bridge to prevent noise and ensure reliability in extreme heat.

- **PMB → NPB (Network Board):** Samtec 4-blade power terminal.
  - **Blades 1-2:** 12V Rail (Logic Power)
  - **Blades 3-4:** 48V Rail (PoE Camera Power)
- **PMB → CORE (Jetson Board):** Samtec 2-blade power terminal.
  - **Blades 1-2:** 12V Rail (Jetson SoC Power)

### 2. Samtec 40-Pin Data Bridge (CORE to NPB)
Strictly for high-speed signal pairs and low-voltage logic.

| Pin | Signal | Direction | Function |
|---|---|---|---|
| 1-4 | NC / RESERVED | - | **Do NOT connect power to these pins.** |
| 5 | UART_TX | CORE -> NPB | Debug/Protocol TX |
| 7 | UART_RX | NPB -> CORE | Debug/Protocol RX |
| 12 | GPIO_14 | NPB -> CORE | Critical Flush Interrupt (Active High) |
| 15 | GPIO_18 | NPB -> CORE | Watchdog Reset (Active Low) |
| 20 | CAN_H | NPB <-> CORE | OBD-II Differential High |
| 22 | CAN_L | NPB <-> CORE | OBD-II Differential Low |
| 25-32 | MDI_P/N | CORE <-> NPB | Gigabit Ethernet PHY-to-PHY Link |

### 3. GPIO Mapping (Final Verified)
- **Jetson (Power En):** GPIO 12
- **ESP32 (PWM Fan):** GPIO 15
- **ESP32 (Ignition Sense):** GPIO 16
- **ESP32 (IMU Interrupt):** GPIO 17
- **ESP32 (Watchdog Out):** GPIO 18 -> Jetson SYS_RESET
- **Jetson (Interrupt In):** GPIO 421 <- ESP32 GPIO 14
