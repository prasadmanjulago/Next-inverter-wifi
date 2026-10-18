# Next Inverter Wi-Fi Card — Carrier Board Hardware Design Specification

> **Target Device:** NEXT NM-ECO-6.2KW PLUS Hybrid Inverter  
> **Host Enclosure / Board:** Carrier PCB for Plug-in Modules  
> **Interface Port:** Inverter Port 12 (RJ45 RS-232 / 12V Supply)  
> **Core Controller:** ESP32-C3 (2.4GHz Wi-Fi + BLE)

---

## 1. Voltage Compatibility Analysis (Direct Answer)

### Does ESP32-C3 accept 5V?
- **Bare ESP32-C3 chip/module (ESP32-C3-WROOM / ESP32-C3-MINI):** **NO!** Absolute maximum is **3.6V DC**. 5V will instantly destroy it.
- **ESP32-C3 Development Board (e.g. ESP32-C3 SuperMini / DevKit):** **YES on the `5V` / `VIN` pin.** The development board has an onboard 3.3V Low-Dropout Regulator (LDO) that safely steps 5V down to 3.3V.
- **ESP32-C3 GPIO Pins (TX, RX):** **STRICTLY 3.3V LOGIC.** The GPIO pins are **NOT 5V tolerant**. Exceeding 3.6V on any GPIO pin can damage the silicon.

---

### Does Mini MAX3232 accept 5V?
- **YES.** The MAX3232 transceiver IC operates from **3.0V to 5.5V DC**.
- **⚠️ THE CRITICAL WARNING:** 
  - If you power the MAX3232 module with **5V**, its TTL output pin (`ROUT` $\rightarrow$ ESP32 RX) will output **5V signals**!
  - Sending 5V TTL directly into the ESP32-C3 RX pin will over-voltage and degrade the ESP32-C3.

---

### The Recommended Power & Voltage Architecture

To keep the circuit simple, reliable, and 100% logic-safe without adding extra resistors or level shifters:

```
[Inverter Port] ──► 12V DC ──► [LM2596 Module (Tuned to 5.0V)]
                                         │
                                         ▼ 5.0V DC
                        [ESP32-C3 Dev Board (VIN / 5V Pin)]
                                         │
                                         ▼ (Clean 3.3V from Onboard LDO)
                          [Mini MAX3232 Module (VCC Pin)]
```

#### Why this is the best setup:
1. **LM2596 outputs 5.0V:** The LM2596 steps the inverter's noisy 12V line down to a stable 5.0V.
2. **ESP32-C3 Dev Board takes 5.0V:** The board's onboard LDO filters the power and generates an ultra-clean, ripple-free **3.3V rail**.
3. **MAX3232 runs on 3.3V:** Powered from the ESP32's `3V3` pin, the MAX3232's TTL logic lines operate natively at **3.3V**, perfectly matching the ESP32-C3 RX and TX pins!
4. **RS-232 side stays full voltage:** Even at 3.3V $V_{CC}$, the MAX3232's internal charge-pump capacitors boost the signals to **$\pm 5.5\text{V} - \pm 6\text{V}$**, fully compliant with the inverter's RS-232 port!

---

## 2. Hardware Block Diagram

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        NEXT INVERTER WIFI CARD (CARRIER BOARD)                         │
│                                                                                        │
│  [4-Pin Incomer]             [LM2596 Buck Module]            [ESP32-C3 Board]          │
│  ┌──────────────┐            ┌──────────────────┐            ┌──────────────┐          │
│  │ 1. 12V DC    ├───────────►│ IN+         OUT+ ├───────────►│ 5V / VIN     │          │
│  │ 2. GND       ├─────┬─────►│ IN-         OUT- ├──┬────────►│ GND          │          │
│  │              │     │      └──────────────────┘  │         │              │          │
│  │              │     │                            │    ┌───►│ 3V3 OUT      │          │
│  │              │     │                            │    │    │              │          │
│  │              │     │      [Mini MAX3232]        │    │    │              │          │
│  │              │     │      ┌──────────────────┐  │    │    │              │          │
│  │              │     │      │ VCC ◄────────────┼──┼────┘    │              │          │
│  │              │     └─────►│ GND ◄────────────┼──┴────────►│ GND          │          │
│  │              │            │                  │            │              │          │
│  │ 3. TX (RS232)├───────────►│ RIN1        ROUT1├───────────►│ GPIO20 (RX)  │          │
│  │ 4. RX (RS232)│◄───────────┤ TOUT1        TIN1│◄───────────┤ GPIO21 (TX)  │          │
│  └──────────────┘            └──────────────────┘            └──────────────┘          │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Detailed Pin-to-Pin Interconnection Table

### Net 1: Power Routing
| Source Terminal | Destination Terminal | Signal Name | Voltage Level | Notes |
| :--- | :--- | :--- | :---: | :--- |
| **J1 Pin 1 (12V)** | **LM2596 `IN+`** | `V_IN_12V` | +12V DC | Inverter Aux Power from RJ45 |
| **J1 Pin 2 (GND)** | **LM2596 `IN-`** | `GND` | 0V | Inverter Signal & Power Ground |
| **LM2596 `OUT+`** | **ESP32-C3 `5V / VIN`** | `VCC_5V` | +5.0V DC | Tuned via LM2596 trimpot |
| **LM2596 `OUT-`** | **ESP32-C3 `GND`** | `GND` | 0V | Common Ground Plane |
| **ESP32-C3 `3V3`** | **MAX3232 `VCC`** | `VCC_3V3` | +3.3V DC | Powers MAX3232 at safe 3.3V TTL |
| **ESP32-C3 `GND`** | **MAX3232 `GND`** | `GND` | 0V | Common Ground |

---

### Net 2: Inverter RS-232 Signals (Incomer J1 $\leftrightarrow$ MAX3232 Module)
| Incomer (J1) Pin | MAX3232 RS-232 Side Pin | Signal Name | Voltage Level | Description |
| :--- | :--- | :--- | :---: | :--- |
| **Pin 3 (TX from Inverter)**| **`RIN` (or `RXD_RS232`)** | `INV_TX_RS232` | $\pm 5\text{V} – \pm 12\text{V}$ | Transmit data from inverter |
| **Pin 4 (RX to Inverter)** | **`TOUT` (or `TXD_RS232`)**| `INV_RX_RS232` | $\pm 5\text{V} – \pm 12\text{V}$ | Receive data to inverter |

---

### Net 3: TTL Serial Signals (MAX3232 Module $\leftrightarrow$ ESP32-C3)
| MAX3232 TTL Side Pin | ESP32-C3 Pin | Signal Name | Voltage Level | Description |
| :--- | :--- | :--- | :---: | :--- |
| **`ROUT` (TTL Output)** | **GPIO 20 (UART RX)** | `ESP_RX_TTL` | **3.3V TTL** | Inverter data into ESP32 UART |
| **`TIN` (TTL Input)** | **GPIO 21 (UART TX)** | `ESP_TX_TTL` | **3.3V TTL** | ESP32 commands into Inverter |

*(Note: On ESP32-C3, hardware UART1 or UART0 can be assigned to GPIO 20 / 21 or GPIO 6 / 7).*

---

## 4. Inverter RJ45 Cable Wiring Map

To make your RJ45 cable to the 4-pin screw terminal connector (J1):

```
Standard RJ45 Plug (Looking at gold pins, tab facing down):
Pin 1 ── [Orange-White] ──► 12V DC  ────────► Connect to J1 Pin 1 (12V)
Pin 2 ── [Orange]       ──► 12V DC  ────────► (Connect together with Pin 1)
Pin 3 ── [Green-White]  ──► TXD (RS232) ────► Connect to J1 Pin 3 (TX)
Pin 4 ── [Blue]         ──► NC (Not used)
Pin 5 ── [Blue-White]   ──► RXD (RS232) ────► Connect to J1 Pin 4 (RX)
Pin 6 ── [Green]        ──► NC (Not used)
Pin 7 ── [Brown-White]  ──► NC (Not used)
Pin 8 ── [Brown]        ──► GND (Ground) ───► Connect to J1 Pin 2 (GND)
```

---

## 5. Carrier PCB Layout Concept

- **Board Dimensions:** Compact rectangular card (~**60 mm × 45 mm**).
- **Mounting Style:** Female pin headers or direct solder pads for:
  - 1 × 4-Pin 2.54mm or 3.81mm Screw Terminal Block (Left Edge).
  - 1 × LM2596 module footprint (4 corner solder pads: `IN+`, `IN-`, `OUT+`, `OUT-`).
  - 1 × Mini MAX3232 module footprint (2 × 4-pin headers).
  - 1 × ESP32-C3 SuperMini footprint (2 × 8-pin headers, facing outward with USB-C port accessible at board edge).
- **Ground Plane:** Full bottom copper ground pour (`GND`) to suppress high-frequency Wi-Fi and inverter switching noise.
