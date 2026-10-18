# Project Progress & Handoff Summary — October 13, 2026

> **Workspace:** `d:\House wiring\`  
> **Sub-Project:** `Next Inverter WIFI Card`  
> **Microcontroller Selected:** **NodeMCU ESP8266 ESP-12E Dev Board (CH340, 30-Pin MicroUSB)**  
> **Rationale:** Standard 0.1" (2.54mm) wide DIP form-factor is significantly easier to hand-solder than compact surface-mount or ultra-dense modules.

---

## 1. Summary of Completed Work

### A. Solar & Inverter Earth Leakage Protection Verification
- **Question Addressed:** In `Program 01 = SBU` (off-grid Solar + Battery feeding load, zero grid blending), what happens when Phase or Neutral leaks into the Earth wire?
- **Conclusion:** 
  - **Phase (Live) to Earth Leak:** **Trips Trip Switch 2** (30mA Type A RCCB on Inverter AC Output) immediately ($<40\text{ ms}$) via the TT earth return loop and Inverter Common Neutral architecture.
  - **Neutral to Earth Leak:** **Also trips Trip Switch 2** as soon as any household load is switched on (due to return current splitting between N and PE), or immediately if standing N–PE potential exceeds ~0.3V across $<10\ \Omega$.
  - Preserved in [`instructions.md`](file:///d:/House%20wiring/instructions.md) and [`solar_blending_troubleshooting.md`](file:///d:/House%20wiring/solar_blending_troubleshooting.md).

---

### B. Inverter Manual Full Text Digitization
- **Created Files:**
  - [`next_6_2kw_inverter_manual.md`](file:///d:/House%20wiring/next_6_2kw_inverter_manual.md): Complete 34-page transcribed text reference manual with all program settings (00–39), fault codes (01–58), specifications, and wiring rules.
  - [`search_manual.py`](file:///d:/House%20wiring/search_manual.py): Zero-latency terminal search script (e.g. `python search_manual.py "Program 01"`).
  - [`manual_extracted_text.json`](file:///d:/House%20wiring/manual_extracted_text.json): Raw line-by-line OCR dump of all 34 pages.

---

### C. "Next Inverter WIFI Card" Carrier PCB Design (NodeMCU 30-Pin)
- **Directory:** [`Next Inverter WIFI Card`](file:///d:/House%20wiring/Next%20Inverter%20WIFI%20Card)
- **Host Controller:** **NodeMCU ESP8266 ESP-12E CH340 (30-Pin)**
  - Powered via **`VIN` (Pin 15 Left)** from LM2596 **5.0V** output.
  - **`3V3` Out (Pin 11 Left)** powers the Mini MAX3232 module safely.
  - Hardware UART: **`RX` (GPIO3)** receives 3.3V TTL from MAX3232 `ROUT`; **`TX` (GPIO1)** sends 3.3V TTL commands to MAX3232 `TIN`.
- **PCB Specifications:**
  - **Dimensions:** **85 mm × 65 mm** (spacious, easy to assemble).
  - **100% Single-Sided Copper:** All tracks and pads on Bottom Copper (`Gerber_BottomLayer.GBL`). **Zero crossovers / zero jumper wires**.
  - **Extra-Thick Lines:**
    - 12V & GND Main Incomer Trunks: **2.0 mm (78 mil)**.
    - 5V Regulated Power Rail: **1.6 mm (63 mil)**.
    - 3.3V Power Line: **1.2 mm (47 mil)**.
    - Signal Traces (TX/RX): **1.0 mm (40 mil)**.
  - **Extra-Large Solder Pads:** **3.0 mm – 3.6 mm** outer diameter with 1.0mm–1.4mm drill holes.
  - **Corner Mounting Holes:** **4 × strictly 3.5 mm Drill Diameter** (`T03C3.500`) with 7.0mm copper pads.
- **Deliverables & Production Files:**
  - [`Next_Inverter_WIFI_Card_Gerbers.zip`](file:///d:/House%20wiring/Next%20Inverter%20WIFI%20Card/Next_Inverter_WIFI_Card_Gerbers.zip): Ready to drag & drop into [tracespace.io/view](https://tracespace.io/view) or upload to JLCPCB.
  - [`pcb_visualizer.html`](file:///d:/House%20wiring/Next%20Inverter%20WIFI%20Card/pcb_visualizer.html): Interactive browser visualizer with clickable pads and trace highlights for the NodeMCU layout.
  - [`generate_gerber.py`](file:///d:/House%20wiring/Next%20Inverter%20WIFI%20Card/generate_gerber.py): Automated generator for the NodeMCU Gerber files.

---

## 2. Plan for Tomorrow's Session
1. **PCB Inspection:** Check the board in [tracespace.io/view](https://tracespace.io/view).
2. **ESP8266 Firmware:** Write the ESPHome YAML / Arduino C++ code for NodeMCU ESP8266 to query the Next Inverter via RS-232 (`QPIGS` command) and send solar telemetry to Home Assistant / Wi-Fi.
