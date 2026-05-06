# Sabre ALPR Hub - Internal Engineering Specs

## ⚠️ CRITICAL FAILURE RISK: Power Isolation
**DO NOT route 12V or 48V power through the 40-pin Samtec Bridge.** Power must strictly utilize the dedicated 2-blade (Core) and 4-blade (Networking) terminals.

## Physical De-Bricking (Tier 2 Recovery)
If a unit has undergone a Tier 2 Hard Wipe (Nuclear Option), the Jetson power rail will be disabled in hardware.
- **To un-brick:** Bridge **Pin 21** (JUMPER_CLEAR) to Ground during the boot sequence. This clears the persistent "bricked" state in the ESP32 NVS.

## Hardware Logic & Pin-Mapping

| Pin | Signal | Direction | Function |
|---|---|---|---|
| 1-4 | NC | - | RESERVED |
| 12 | GPIO_14 | NPB -> CORE | Critical Flush Interrupt |
| 15 | GPIO_18 | NPB -> CORE | Watchdog Reset (Active Low) |
| 21 | GPIO_21 | - | **JUMPER_CLEAR** (Bridge to GND to recover) |
| 25-32 | MDI_P/N | CORE <-> NPB | Gigabit Ethernet PHY-to-PHY |

### Branding Assets
- **Format:** 512x512px Transparent PNG.
- **Watermark:** The MDT displays this at 30% opacity on all live streams.
