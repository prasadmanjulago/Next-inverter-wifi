# Next Inverter Wi-Fi Card — Complete Session Summary & Handoff
**Date:** October 18, 2026  
**Workspace:** `D:\House wiring\Next Inverter WIFI Card`  
**Target Hardware:** ESP32 DevKit V1 Type-C + Mini MAX3232 Transceiver + NEXT NM-ECO-6.2KW PLUS Inverter  
**Live IP Address:** [http://192.168.1.15](http://192.168.1.15) (Home Wi-Fi: `Easytech Solutions`)  
**mDNS Endpoint:** [http://next-inverter.local](http://next-inverter.local)  
**ThingsBoard Cloud:** [http://eu.thingsboard.cloud](http://eu.thingsboard.cloud) (Token: `QJJXWvLJZ4GAECdglKb3`)  
**GitHub Repository:** [prasadmanjulago/Next-inverter-wifi](https://github.com/prasadmanjulago/Next-inverter-wifi) (Latest Release: `v1.0.3`, Commit: `d222242`)  

---

## 1. Executive Summary: What We Accomplished Today

Today was a 100% end-to-end success across all hardware, firmware, and cloud layers:
1. **Physical Verification:** Measured and confirmed inverter RS-232 Port 12 voltage (`-13.62V DC` on Pin 1 = Inverter TXD) and identified the 3-wire connection standard.
2. **MAX3232 Architecture:** Discovered module dual-channel independence and corrected wiring strictly to **Channel 1**.
3. **ESP32 Full Recovery & Flashing Fix:** Solved the `Invalid head of packet (0x80)` flashing roadblock by discovering USB slew rate constraints on COM4, tuning the baud rate to **38400**, increasing esptool block timeouts, and successfully reviving the chip.
4. **Live Inverter Communication Established:** Connected ESP32 to the inverter; bidirectional communication locked in at **2400 baud** with 100% data fidelity (`QPIGS` and `QMOD`).
5. **Clean SmartESS Web Dashboard:** Removed debug serial diagnostics from the view for a pristine, native 1:1 SmartESS interface replica.
6. **ThingsBoard Cloud Integration with Averaging Engine (v1.0.3):**
   * Implemented `InverterAccumulator` mathematical engine that samples the inverter continuously and computes **true arithmetic averages** every 10 seconds.
   * Connected directly to `eu.thingsboard.cloud` with HTTP 200 telemetry delivery.
   * Calculated and streamed **4 dedicated power channels (Watts)** for the multi-series line chart (`load_watts`, `solar_watts`, `grid_watts`, and positive `battery_input_watts`).
   * Deployed wirelessly via Web OTA and synchronized with GitHub.

---

## 2. Hardware Wiring Specification (3-Wire Interface)

```
[ Inverter Port 12 (RJ45) ]                  [ MAX3232 Module ]                   [ ESP32 DevKit V1 ]
Pin 1 (Orange-White, -13.6V) ──► RS232 Side: R1IN  ──► TTL Side: R1OUT ──► RX2 (GPIO 16)
Pin 2 (Orange, Inverter RX)  ◄── RS232 Side: T1OUT ◄── TTL Side: T1IN  ◄── TX2 (GPIO 17)
Pin 8 (Brown, Inverter GND)  ──► RS232 Side: GND   ──► TTL Side: GND   ──► GND (Common Ground)
                                                       TTL Side: VCC   ──► 3V3 Pin
```

### Key Hardware Rules:
* **Strictly Channel 1:** `T1IN`, `T1OUT`, `R1IN`, `R1OUT` only. Do not connect anything to Channel 2 (`T2/R2`).
* **Pin 4 (Blue Wire) Abandoned:** Measured at `+3.85V DC` (internal unbuffered logic line). Never draw current from it.
* **Power Source:** ESP32 is powered safely via standard 5.0V USB-C / phone charger.

---

## 3. The ESP32 Flashing Breakthrough

* **The Problem:** PlatformIO and esptool failed at 115200 baud with `Invalid head of packet (0x80)` / `The serial TX path seems to be down`.
* **Root Cause:** Signal rise-time degradation / high line capacitance on the USB-to-UART bridge at 115.2 kbps caused bit slips.
* **The Solution:**
  1. Tuned PlatformIO upload speed to **`38400` baud** (`upload_speed = 38400` in `platformio.ini`).
  2. Increased esptool block write timeout from 3s to 10s (`DEFAULT_TIMEOUT = 10` in `loader.py`) to prevent timeouts during sector write operations at lower baud rates.
  3. Diagnosed strapping pins: verified BOOT button was healthy (3.3V pull-up confirmed via multimeter) and transitioned chip from `boot:0x0` to `boot:0x13 (SPI_FAST_FLASH_BOOT)`.

---

## 4. Live Inverter Telemetry Stream

The inverter communicates via the **Voltronic PI30 Protocol** (`QPIGS` and `QMOD`) at **2400 baud, 8N1**:

| Parameter | Verified Value | Description |
| :--- | :--- | :--- |
| **Working Mode** | `Line Mode (Grid)` | Utility grid powering household loads & bypass |
| **Grid Voltage** | `~225.5 V` | Incoming AC utility line voltage |
| **Grid Frequency** | `50.0 Hz` | Utility frequency |
| **AC Output Voltage** | `~225.5 V` | Voltage supplied to home loads |
| **AC Output Frequency** | `50.0 Hz` | Output frequency |
| **AC Output Power** | `~200 W – 700 W` | Real active home power consumption |
| **Output Load %** | `~4% – 17%` | Percentage of inverter's 6200W capacity |
| **Battery Voltage** | `~56.0 V` | Actual terminal voltage of 48V battery bank |
| **Battery SOC** | `81%` | State of Charge percentage |
| **Heatsink Temp** | `57.0 °C` | Power stage temperature |
| **PV Solar Power** | `0.0 W` | Solar generation *(active during daylight)* |

---

## 5. ThingsBoard Cloud Integration & Mathematical Averaging

### A. Mathematical Averaging Engine
* Because the inverter is queried every 3 seconds, a 10-second cloud interval collects **3 to 4 raw data points**.
* Rather than sending a single instantaneous snapshot, the firmware computes the **exact arithmetic average** for voltage, current, and wattage across each interval before transmitting.
* Supports configurable intervals: `10s` (~3 samples), `15s` (~5 samples), `30s` (~10 samples), or `60s` (~20 samples).

### B. Dedicated Power Series for Line Charts
To make graphing seamless in ThingsBoard, the firmware pre-calculates and streams 4 positive power values:
1. **`load_watts`**: Real power consumed by the house (W).
2. **`solar_watts`**: Solar PV power generated (W).
3. **`grid_watts`**: Utility grid input power (W).
4. **`battery_input_watts`**: Power contributed by the battery to the load (**positive +W** when discharging to power the load; `0 W` when charging).
5. **`battery_power_watts`**: Inverted polarity so discharging is **positive (+)** and charging is negative (-).

### C. ThingsBoard Chart Setup Summary
* **Widget:** Timeseries $\rightarrow$ Time Series Chart (Line chart).
* **Datasources:** Add your Inverter device with the 4 keys:
  * `load_watts` (Blue)
  * `solar_watts` (Orange)
  * `grid_watts` (Red / Grey)
  * `battery_input_watts` or `battery_power_watts` (Green)
* All 4 power flows plot as positive lines on the same timeline.

---

## 6. Git Synchronization & Release History

* **Repository:** `https://github.com/prasadmanjulago/Next-inverter-wifi`
* **Branch:** `main`
* **Commits Pushed Today:**
  * `0ecfe76`: Release v1.0.2 — Serial Diagnostics, Raw Hex/ASCII display, Auto-Baud, and Pin Swap.
  * `b249414`: Release v1.0.3 — ThingsBoard Cloud Integration with multi-sample interval averaging (10s/15s/30s) and clean UI.
  * `ec033d8`: Add explicit power keys (`load_watts`, `solar_watts`, `grid_watts`, `battery_power_watts`) for ThingsBoard.
  * `d222242`: Update battery power polarity to positive (+) when contributing to load.
* **Firmware Binary:** `firmware.bin` (v1.0.3, updated in root repository for 1-click GitHub OTA updates).

---

## 7. Current System State
* **ESP32:** Running live on home Wi-Fi (`Easytech Solutions`) at IP `192.168.1.15`.
* **Inverter RS-232:** Actively polling at 2400 baud; SmartESS web dashboard updating every 2 seconds.
* **ThingsBoard:** Actively streaming averaged telemetry every 10 seconds to `http://eu.thingsboard.cloud`.
* **Hardware:** Standalone, cool, stable, and completely hands-free.
