# Next Inverter Wi-Fi Telemetry & Cloud Gateway

Firmware and OTA distribution for the **Next 6.2kW Hybrid Solar Inverter** Wi-Fi telemetry card (ESP8266 / ESP32).

## Current Release: v1.6.0
- **Platform:** ESP8266 (with USB-to-UART programmer support)
- **Features:**
  - Real-time RS-232 telemetry polling (Voltronic QPIGS / QMOD / QPIRI protocols)
  - Direct HTTP telemetry streaming to Oracle Cloud ThingsBoard (5s/15s interval)
  - Embedded local dashboard & parameter management
  - Web and GitHub OTA updates
  - Automatic Wi-Fi reconnection supervisor watchdog
  - Pure 2.4GHz radio mode (no BLE contention)

## Files in this repository:
- `firmware.bin`: Compiled binary ready for flashing or OTA update.
- `version.json`: Immutable OTA release manifest used by the device update checker.
