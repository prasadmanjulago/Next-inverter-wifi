# Next Inverter Wi-Fi Card — Project Progress & Handoff Summary
**Session Date:** October 18, 2026  
**Workspace:** `D:\House wiring\Next Inverter WIFI Card`  
**Target Hardware:** ESP32 DevKit V1 Type-C + Mini MAX3232 Transceiver + NEXT NM-ECO-6.2KW PLUS Inverter  
**Live IP Address:** [http://192.168.1.15](http://192.168.1.15) (SSID: `Easytech3D Solutions`)  
**GitHub Repository:** [prasadmanjulago/Next-inverter-wifi](https://github.com/prasadmanjulago/Next-inverter-wifi) (Latest Release: `v1.0.2`)  

---

## 1. Major Breakthroughs & Hardware Findings in This Session

### A. Inverter RS-232 Port 12 Confirmed with Multimeter
* **Multimeter Measurement:** The user measured **`-13.62V DC`** on the **Orange-White wire (RJ45 Pin 1)** relative to **Brown wire (Pin 8 / GND)**.
* **Significance:** In RS-232 standards, an idle negative voltage ($-5\text{V}$ to $-15\text{V}$) is the textbook mark state. This proves **100% that Orange-White is the Inverter's TXD line**.
* **Safety Save:** An earlier specification mistakenly listed Pins 1 & 2 as 12V. The user questioned this, preventing +12V from being injected into the inverter's serial transceivers.

### B. Inverter OEM Identification
* **OEM Manufacturer:** **Guangdong Sunray Power Co., Ltd.** (Sunray / SUMRY).
* **OEM Model:** **SUMRY NM-ECO-6.2KW** / Sunray NM-ECO-6.2KW PLUS.
* **Protocol:** **Voltronic PI30 Protocol** (`QPIGS`, `QMOD`, `QPI`, `POP00/01/02`) at `2400` or `9600` baud.
* **Inverter Menu Setting:** There is **NO setting to turn on** in the inverter menu; the RS-232 port is permanently active hardware-wise.

### C. MAX3232 Dual-Channel Discovery
* The MAX3232 module contains **two completely independent transceiver channels**:
  * **Channel 1:** `T1IN` (TTL) $\rightarrow$ `T1OUT` (RS-232) and `R1IN` (RS-232) $\rightarrow$ `R1OUT` (TTL).
  * **Channel 2:** `T2IN` (TTL) $\rightarrow$ `T2OUT` (RS-232) and `R2IN` (RS-232) $\rightarrow$ `R2OUT` (TTL).
* **Cause of 1.6V Reading:** When signals were split across Channel 1 and Channel 2, the signal path was open-circuited inside the chip, leaving the unused output floating at half-supply (~1.6V).
* **Solution:** Wire strictly to **Channel 1**.

### D. RJ45 Pin 4 (Blue Wire) Power Analysis
* **Multimeter Measurement:** The user measured **`+3.85V DC`** on the **Blue wire (Pin 4)** relative to Brown (GND).
* **Crucial Decision:** Pin 4 is **NOT a 12V aux power rail**; it is an internal unbuffered logic rail. The LM2596 buck converter cannot run on 3.85V (requires $\ge 7\text{V}$ to output 5.0V), and attempting to draw 200mA from it risks pulling down the inverter's internal CPU.
* **Power Architecture:** Power the ESP32 safely via standard **5.0V USB-C** / phone adapter.
* **Cable Requirement:** Only **3 wires** are needed from the inverter:
  1. Pin 1 (Orange-White) = Inverter TXD ($-13.62\text{V}$)
  2. Pin 2 (Orange) = Inverter RXD
  3. Pin 8 (Brown) = Ground (GND)
  *(All other wires: Blue, Green, etc. are taped off).*

---

## 2. Final Hardware Wiring Specification (Channel 1)

```
[ Inverter Port 12 (RJ45) ]                  [ MAX3232 Module ]                   [ ESP32 DevKit V1 ]
Pin 1 (Orange-White, -13.6V) ──► RS232 Side: R1IN  ──► TTL Side: R1OUT ──► RX2 (GPIO 16)
Pin 2 (Orange, Inverter RX)  ◄── RS232 Side: T1OUT ◄── TTL Side: T1IN  ◄── TX2 (GPIO 17)
Pin 8 (Brown, Inverter GND)  ──► RS232 Side: GND   ──► TTL Side: GND   ──► GND (Common Ground)
                                                       TTL Side: VCC   ──► 3V3 Out (or VIN 5V if needed)
```

*(Leave `T2IN`, `T2OUT`, `R2IN`, `R2OUT` completely disconnected).*

---

## 3. Firmware v1.0.2 Architecture & Live Diagnostics

### Features Implemented & Flashed Over-The-Air:
1. **Live Serial Diagnostics Panel on Web Screen:**
   * Shows active baud rate (`2400` / `9600 bps`).
   * Shows pin configuration (`RX:16 TX:17`).
   * Shows real-time TX/RX byte counters.
   * Shows exact **Last RX Raw** ASCII string and **Last RX Hex** dump.
2. **Interactive Controls directly on the Dashboard:**
   * **`2400` / `9600` Baud Switchers:** Changes UART baud rate on the fly.
   * **`🔀 Swap RX/TX` Button:** Dynamically swaps GPIO 16 and 17 in software (resolves reversed wiring without physical rewiring).
   * **`Send QPIGS` / `Send QMOD` / `Send QPI`:** Immediate one-click manual command tests.
3. **Auto-Baud Detection:**
   * When disconnected, automatically alternates testing between 2400 and 9600 baud, locking in automatically once the inverter answers.
4. **Non-Blocking Serial Polling:**
   * `server.handleClient()` called inside serial reading loops so web dashboard remains lightning-fast (<10ms).
5. **Wireless OTA Engine:**
   * Full Over-The-Air firmware updates enabled directly from GitHub releases and browser file upload endpoint (`/update`).

---

## 4. Git Synchronization Status

* **Repository:** `https://github.com/prasadmanjulago/Next-inverter-wifi`
* **Branch:** `main`
* **Latest Commit:** `0ecfe76` — *"Release v1.0.2: Serial Diagnostics, Raw Hex/ASCII Feed, Auto-Baud (2400/9600), and Software RX/TX Pin Swap"*
* **Synced Files:**
  * `firmware/src/main.cpp`
  * `firmware/src/web_dashboard.h`
  * `firmware/src/inverter_protocol.h`
  * `firmware.bin` (Compiled v1.0.2 binary, 1,057,760 bytes)
  * `version.json` (v1.0.2 metadata)

---

## 5. Immediate Next Steps for Next Session

1. **Verify Channel 1 Voltage:**
   * Connect `T1IN` to ESP32 `TX2` (GPIO 17).
   * Measure `T1OUT` with multimeter relative to GND. Confirm it outputs negative resting voltage ($-5\text{V}$ to $-9\text{V}$).
2. **Connect to Inverter:**
   * Plug `T1OUT` into Inverter Orange wire (Pin 2).
   * Plug `R1IN` into Inverter Orange-White wire (Pin 1).
3. **Verify Dashboard:**
   * Open [http://192.168.1.15](http://192.168.1.15).
   * Watch **Last RX Raw** and the status header change from `🔴 Inverter Disconnected` to `🟢 Inverter Connected`.
