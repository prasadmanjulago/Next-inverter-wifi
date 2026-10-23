# Next Inverter Wi-Fi Telemetry & Cloud Gateway

Firmware and OTA distribution for the **Next NM-ECO-6.2KW PLUS Hybrid Solar Inverter** Wi-Fi telemetry card (**ESP8266 ESP-01 / ESP32**).

## Current Production Release: v1.6.7
- **Platform:** ESP8266 (ESP-01 1MB Flash) on MAX3232 carrier board.
- **Inverter Communication:**
  - RS-232 at **2400 bps** (8N1) using `SoftwareSerial` (RX on **GPIO 1**, TX on **GPIO 3**).
  - Physical RJ45 wiring: Pin 1 (Orange-White) = Inverter TX (-13V), Pin 2 (Orange) = Inverter RX, Pin 8 (Brown) = GND.
  - Voltronic `QPIGS` (Status), `QMOD` (Mode), `QPIRI` (Ratings) real-time query engine.
  - Remote power control: `SON` (Turn ON) / `SOFF` (Turn OFF).
- **Embedded Web Dashboard:**
  - High-contrast Dark Mode UI (`http://192.168.1.19`).
  - Glowing SVG Power Flow Animations (PV, Grid, Battery, Consuming load) with subtle dark circuit tracks.
  - Dedicated Live Diagnostics Page (`/diagnostics`): Auto-Baud detection, pin swap toggle, live line voltage monitor, and RS-232 telemetry feeds.
  - Persistent flash memory storage (`Preferences`) for UART and baud configuration.
- **ThingsBoard Cloud Integration:**
  - Direct HTTP REST streaming to Oracle Cloud ThingsBoard (`http://161.118.190.164:8080`).
  - 15-second rolling sample averaging pipeline.
- **Wireless Over-The-Air (OTA) Updates:**
  - Direct wireless browser and curl uploads at `/update`.
  - Immutable GitHub release checker with auto-detection via `version.json`.
  - Binary size: **~401 KB** (well within the 479 KB dual-bank OTA safety limit).

---

## Quick Flashing & OTA Commands

### Wireless Flash (Over-The-Air):
```bash
curl.exe -i -X POST -F "update=@firmware.bin" http://192.168.1.19/update
```

### Remote Inverter Power Control:
```bash
# Turn Off Output (SOFF):
curl.exe -X POST "http://192.168.1.19/api/inverter_cmd?cmd=SOFF"

# Turn On Output (SON):
curl.exe -X POST "http://192.168.1.19/api/inverter_cmd?cmd=SON"
```

---

## Files in this Repository:
- `firmware.bin`: Production compiled binary ready for wireless OTA or USB flashing.
- `version.json`: Immutable OTA release manifest used by the automatic update engine.
- `SESSION_HANDOFF_2026-10-23.md`: Comprehensive engineering documentation, pinouts, multimeter readings, and architecture spec.
