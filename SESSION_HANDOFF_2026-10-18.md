# Next Inverter Wi-Fi Card — Project Progress & Handoff Summary
**Session Date:** October 18, 2026  
**Workspace:** `D:\House wiring\Next Inverter WIFI Card`  
**Host Hardware:** ESP32 DevKit V1 Type-C (Silicon Labs CP2102, COM4)  
**Target Inverter:** NEXT NM-ECO-6.2KW PLUS Hybrid Inverter  

---

## 1. What Was Completed in Today's Session

### A. Hardware & Driver Configuration
1. **USB-UART Driver Resolution:** Diagnosed Windows `Code 28` error on the CP2102 chip; installed official Silicon Labs VCP driver, successfully mapping the ESP32 to **`COM4`**.
2. **PlatformIO Toolchain Integration:** Installed `PlatformIO Core 6.2.0`, `espressif32@7.1.3` platform, and `xtensa-esp32` toolchain locally.

### B. Pin Mapping Architecture (ESP32 DevKit V1)
| ESP32 Pin | Connects To | Voltage Level | Function |
| :--- | :--- | :---: | :--- |
| **`GPIO 16 (RX2)`** | MAX3232 `ROUT` | 3.3V TTL | Receives RS-232 telemetry from Inverter Port 12 |
| **`GPIO 17 (TX2)`** | MAX3232 `TIN` | 3.3V TTL | Sends `QPIGS` / `QMOD` commands to Inverter |
| **`VIN (5V)`** | LM2596 Output | +5.0V DC | Main board power rail (stepped down from 12V RJ45 aux) |
| **`3V3 Out`** | MAX3232 `VCC` | +3.3V DC | Powers MAX3232 at 3.3V to protect ESP32 GPIOs |
| **`GND`** | Common Ground | 0V | Unified ground plane |
| **USB Type-C** | PC (`COM4`) | 5V / USB | Serial debug monitor (115200) & firmware upload (921600) |

---

## 2. Firmware Architecture & Source Code

Located in: [`Next Inverter WIFI Card/firmware/`](file:///D:/House%20wiring/Next%20Inverter%20WIFI%20Card/firmware)

### Files Created & Flashed:
1. **`platformio.ini`**: Configured for `esp32doit-devkit-v1`, upload & monitor port on **COM4**, high-speed 921600 baud flashing.
2. **`src/inverter_protocol.h`**:
   * Exact Voltronic/Axpert CRC16 checksum calculation (avoiding control bytes `0x0A`, `0x0D`, `0x28`).
   * Parses `QPIGS` response: Grid V/Hz, Output V/Hz/Watts/VA/Load%, Battery Volts/Charge/Discharge/SoC%, Solar PV Volts/Amps/Watts, Heatsink Temp °C.
   * Parses `QMOD` operation mode (SBU, SUB, Line, Battery, Standby, Fault).
3. **`src/web_dashboard.h`**:
   * **SmartESS 1:1 App Replica:** Matching the exact layout from user reference image with 5 circular nodes (Solar, Grid, Inverter, Battery, Home Load).
   * **Separated Energy Flow Paths:** 30px parallel trunk separation between Solar/Battery (left at $x=165$) and Grid/Load (right at $x=195$) with zero overlaps.
   * **Live Inverter Status Pill:**
     * `🟢 Inverter Connected` with live timestamp when valid serial feed is active.
     * `🔴 Inverter Disconnected` (with 8-second auto-timeout if data stops).
   * **Automatic 2.4GHz Wi-Fi Scanner:** Dropdown menu that automatically scans nearby networks with RSSI dBm and lock indicators.
   * **Fallback AP Hotspot:** Boots into **`NextInverter-WIFI`** (`192.168.4.1`) if home Wi-Fi is unavailable.
4. **`src/main.cpp`**: Main event loop, dual UART management, preferences NVS storage for credentials, and REST API endpoints (`/api/telemetry`, `/api/wifi_scan`, `/wifi_save`, `/api/set_priority`).

---

## 3. UI Previews Available for Inspection

* **SmartESS Replica:** [`gui_smartess_replica.html`](file:///D:/House%20wiring/Next%20Inverter%20WIFI%20Card/gui_smartess_replica.html)
* **Tesla Cyber Glass:** [`gui_preview_tesla.html`](file:///D:/House%20wiring/Next%20Inverter%20WIFI%20Card/gui_preview_tesla.html)
* **SunSynk Blueprint:** [`gui_preview_sunsynk.html`](file:///D:/House%20wiring/Next%20Inverter%20WIFI%20Card/gui_preview_sunsynk.html)

---

## 4. Future Roadmap: The 9-Card Solar-Assistant / Grafana Dashboard

As per user reference image (`media_1792304091915_4eb6cda9.png`), future sessions will expand this into a multi-card dashboard:

1. **Card 1 (Completed):** SmartESS Live Orthogonal Energy Flow (Solar $\rightarrow$ Inverter $\rightarrow$ Home $\rightleftarrows$ Battery).
2. **Card 2:** Solar Input History Chart (Solar Power W & Solar Voltage V over 24h).
3. **Card 3:** Cell Voltages & Cell Delta mV chart (integrating JK BMS data).
4. **Card 4:** Total Power Area Chart (Inverter output, Solar input, Net power, Battery flow).
5. **Card 5:** DC Battery Voltage curves (Inverter reported vs BMS reported).
6. **Card 6:** AC Voltages timeline (Grid CEB voltage vs Inverter AC output voltage).
7. **Card 7:** Live Parameter State Table (Time, Sensor name, Last value).
8. **Card 8:** Output Load semi-circle speedometer gauge (0–100% capacity).
9. **Card 9:** Total Daily Energy Generation counter (`kWh`).
