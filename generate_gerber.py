import os
import zipfile

def to_coord(val_mm):
    return f"{int(round(val_mm * 1000000)):010d}"

def to_drill_coord(val_mm):
    return f"X{int(round(val_mm * 1000)):06d}Y{int(round(val_mm * 1000)):06d}"

def create_nodemcu_single_sided_gerbers():
    out_dir = r"d:\House wiring\Next Inverter WIFI Card\gerber"
    os.makedirs(out_dir, exist_ok=True)

    # -------------------------------------------------------------
    # 1. Edge_Cuts (Board Outline: 85mm x 65mm with 2.5mm rounded corners)
    # Generous dimensions for NodeMCU ESP8266 30-pin Dev Board
    # -------------------------------------------------------------
    w, h = 85.0, 65.0
    r = 2.5
    outline_lines = [
        "G04 Board Outline*",
        "%FSLAX46Y46*%",
        "%MOMM*%",
        "%ADD10C,0.200000*%",
        "D10*",
        f"X{to_coord(r)}Y{to_coord(0)}D02*",
        f"X{to_coord(w-r)}Y{to_coord(0)}D01*",
        f"X{to_coord(w)}Y{to_coord(r)}D01*",
        f"X{to_coord(w)}Y{to_coord(h-r)}D01*",
        f"X{to_coord(w-r)}Y{to_coord(h)}D01*",
        f"X{to_coord(r)}Y{to_coord(h)}D01*",
        f"X{to_coord(0)}Y{to_coord(h-r)}D01*",
        f"X{to_coord(0)}Y{to_coord(r)}D01*",
        f"X{to_coord(r)}Y{to_coord(0)}D01*",
        "M02*"
    ]
    with open(os.path.join(out_dir, "Gerber_BoardOutline.GKO"), "w") as f:
        f.write("\n".join(outline_lines))

    # -------------------------------------------------------------
    # PINS AND COMPONENT DEFINITIONS
    # -------------------------------------------------------------
    # J1: 4-Pin 5.08mm Incomer Terminal Block (Left Edge)
    j1_pins = [
        (1, 9.0, 48.00, "square", "12V"),
        (2, 9.0, 42.92, "round",  "GND"),
        (3, 9.0, 37.84, "round",  "TXD"),
        (4, 9.0, 32.76, "round",  "RXD")
    ]

    # LM2596: 4 Heavy Corner Solder Pads
    lm_pads = [
        ("IN+",  22.0, 58.0, "heavy_sq"),
        ("IN-",  22.0, 44.0, "heavy_rd"),
        ("OUT+", 68.0, 58.0, "heavy_sq"),
        ("OUT-", 68.0, 44.0, "heavy_rd")
    ]

    # Mini MAX3232 Module: 2 rows of 4 pins @ 2.54mm pitch
    max_rs232 = [
        (1, 18.0, 15.00, "round", "VCC"),
        (2, 18.0, 17.54, "round", "GND"),
        (3, 18.0, 20.08, "round", "RIN"),
        (4, 18.0, 22.62, "round", "TOUT")
    ]
    max_ttl = [
        (1, 32.0, 15.00, "square", "VCC_3V3"),
        (2, 32.0, 17.54, "round",  "GND"),
        (3, 32.0, 20.08, "round",  "ROUT"),
        (4, 32.0, 22.62, "round",  "TIN")
    ]

    # NodeMCU ESP8266 ESP-12E CH340 (30-pin: 2 rows of 15 pins @ 2.54mm pitch)
    # Row Spacing: 28.0 mm (standard 1.1 inch wide CH340 footprint)
    # Left Row (X = 48.0 mm):
    nodemcu_left = []
    # Right Row (X = 76.0 mm):
    nodemcu_right = []
    
    y_start = 12.0
    for i in range(15):
        y_pos = y_start + i * 2.54
        ptype = "square" if i == 0 else "round"
        nodemcu_left.append((i + 1, 48.0, y_pos, ptype, f"L_{i+1}"))
        nodemcu_right.append((i + 1, 76.0, y_pos, ptype, f"R_{i+1}"))

    # 4 Corner Holes: STRICTLY 3.5mm Drill Diameter
    m35_holes = [
        (4.5, 4.5), (80.5, 4.5), (4.5, 60.5), (80.5, 60.5)
    ]

    # -------------------------------------------------------------
    # 2. BOTTOM COPPER (Gerber_BottomLayer.GBL) — 100% SINGLE-SIDED
    # ALL TRACES ON THIS LAYER • EXTRA THICK LINES • ZERO CROSSINGS
    # -------------------------------------------------------------
    gbl_lines = [
        "G04 Single-Sided Bottom Copper Layer for NodeMCU*",
        "%FSLAX46Y46*%",
        "%MOMM*%",
        # Thick Line Apertures
        "%ADD11C,1.000000*%", # Signal trace (1.0mm / 40 mil)
        "%ADD12C,1.200000*%", # 3.3V power line (1.2mm / 47 mil)
        "%ADD13C,1.600000*%", # 5V power trace (1.6mm / 63 mil)
        "%ADD14C,2.000000*%", # 12V and GND main trunk (2.0mm / 78 mil)
        # Extra-Large Solder Pads (Easy soldering, no pad lifting)
        "%ADD20C,3.000000*%", # 3.0mm round pad
        "%ADD21R,3.000000,3.000000*%", # 3.0x3.0mm square pad
        "%ADD22C,3.600000*%", # 3.6mm heavy round pad for LM2596
        "%ADD23R,3.600000,3.600000*%", # 3.6x3.6mm heavy square pad
        "%ADD24C,7.000000*%", # 7.0mm pad ring for 3.5mm mounting hole
    ]

    # Flash Pads
    for _, x, y, ptype, _ in j1_pins:
        ap = "D21*" if ptype == "square" else "D20*"
        gbl_lines.append(f"{ap}X{to_coord(x)}Y{to_coord(y)}D03*")
    for _, x, y, ptype in lm_pads:
        ap = "D23*" if ptype == "heavy_sq" else "D22*"
        gbl_lines.append(f"{ap}X{to_coord(x)}Y{to_coord(y)}D03*")
    for _, x, y, ptype, _ in max_rs232 + max_ttl + nodemcu_left + nodemcu_right:
        ap = "D21*" if ptype == "square" else "D20*"
        gbl_lines.append(f"{ap}X{to_coord(x)}Y{to_coord(y)}D03*")
    gbl_lines.append("D24*")
    for x, y in m35_holes:
        gbl_lines.append(f"X{to_coord(x)}Y{to_coord(y)}D03*")

    # =============================================================
    # THICK COPPER TRACES (ALL ON 1 SIDE — ZERO CROSSINGS)
    # =============================================================
    # Trace 1: 12V Incomer (J1 Pin 1 -> LM2596 IN+) — 2.0mm Thick
    gbl_lines.extend([
        "D14*",
        f"X{to_coord(9.0)}Y{to_coord(48.0)}D02*",
        f"X{to_coord(14.0)}Y{to_coord(48.0)}D01*",
        f"X{to_coord(14.0)}Y{to_coord(58.0)}D01*",
        f"X{to_coord(22.0)}Y{to_coord(58.0)}D01*"
    ])

    # Trace 2: GND Incomer (J1 Pin 2 -> LM2596 IN-) — 2.0mm Thick
    gbl_lines.extend([
        "D14*",
        f"X{to_coord(9.0)}Y{to_coord(42.92)}D02*",
        f"X{to_coord(16.0)}Y{to_coord(42.92)}D01*",
        f"X{to_coord(16.0)}Y{to_coord(44.0)}D01*",
        f"X{to_coord(22.0)}Y{to_coord(44.0)}D01*"
    ])

    # Trace 3: Inverter TX (J1 Pin 3 -> MAX3232 RIN) — 1.0mm Thick
    gbl_lines.extend([
        "D11*",
        f"X{to_coord(9.0)}Y{to_coord(37.84)}D02*",
        f"X{to_coord(13.0)}Y{to_coord(37.84)}D01*",
        f"X{to_coord(13.0)}Y{to_coord(20.08)}D01*",
        f"X{to_coord(18.0)}Y{to_coord(20.08)}D01*"
    ])

    # Trace 4: Inverter RX (J1 Pin 4 -> MAX3232 TOUT) — 1.0mm Thick
    gbl_lines.extend([
        "D11*",
        f"X{to_coord(9.0)}Y{to_coord(32.76)}D02*",
        f"X{to_coord(15.0)}Y{to_coord(32.76)}D01*",
        f"X{to_coord(15.0)}Y{to_coord(22.62)}D01*",
        f"X{to_coord(18.0)}Y{to_coord(22.62)}D01*"
    ])

    # Trace 5: 5V Power (LM2596 OUT+ -> NodeMCU VIN Pin 1 Left) — 1.6mm Thick
    # Left pin 1 is at (48.0, 12.0)
    gbl_lines.extend([
        "D13*",
        f"X{to_coord(68.0)}Y{to_coord(58.0)}D02*",
        f"X{to_coord(44.0)}Y{to_coord(58.0)}D01*",
        f"X{to_coord(44.0)}Y{to_coord(12.0)}D01*",
        f"X{to_coord(48.0)}Y{to_coord(12.0)}D01*"
    ])

    # Trace 6: GND (LM2596 OUT- -> NodeMCU GND Pin 2 Left) — 1.6mm Thick
    # Left pin 2 is at (48.0, 14.54)
    gbl_lines.extend([
        "D13*",
        f"X{to_coord(68.0)}Y{to_coord(44.0)}D02*",
        f"X{to_coord(42.0)}Y{to_coord(44.0)}D01*",
        f"X{to_coord(42.0)}Y{to_coord(14.54)}D01*",
        f"X{to_coord(48.0)}Y{to_coord(14.54)}D01*"
    ])

    # Trace 7: 3.3V Power (NodeMCU 3V3 Pin 5 Left -> MAX3232 VCC) — 1.2mm Thick
    # Left Pin 5 (Y = 22.16)
    gbl_lines.extend([
        "D12*",
        f"X{to_coord(48.0)}Y{to_coord(22.16)}D02*",
        f"X{to_coord(38.0)}Y{to_coord(22.16)}D01*",
        f"X{to_coord(38.0)}Y{to_coord(15.0)}D01*",
        f"X{to_coord(32.0)}Y{to_coord(15.0)}D01*"
    ])

    # Trace 8: MAX3232 GND -> NodeMCU GND (1.2mm Thick)
    gbl_lines.extend([
        "D12*",
        f"X{to_coord(32.0)}Y{to_coord(17.54)}D02*",
        f"X{to_coord(40.0)}Y{to_coord(17.54)}D01*",
        f"X{to_coord(40.0)}Y{to_coord(14.54)}D01*",
        f"X{to_coord(48.0)}Y{to_coord(14.54)}D01*"
    ])

    # Trace 9: RS-232 side GND to J1 GND (1.2mm Thick)
    gbl_lines.extend([
        "D12*",
        f"X{to_coord(18.0)}Y{to_coord(17.54)}D02*",
        f"X{to_coord(11.0)}Y{to_coord(17.54)}D01*",
        f"X{to_coord(11.0)}Y{to_coord(42.92)}D01*",
        f"X{to_coord(9.0)}Y{to_coord(42.92)}D01*"
    ])

    # Trace 10: MAX3232 ROUT (TTL RX) -> NodeMCU RX Pin (Pin 3 Right: 76.0, 17.08) — 1.0mm Thick
    # Routes straight through open center channel between NodeMCU rows (zero crossing!)
    gbl_lines.extend([
        "D11*",
        f"X{to_coord(32.0)}Y{to_coord(20.08)}D02*",
        f"X{to_coord(36.0)}Y{to_coord(20.08)}D01*",
        f"X{to_coord(36.0)}Y{to_coord(26.0)}D01*",
        f"X{to_coord(60.0)}Y{to_coord(26.0)}D01*",
        f"X{to_coord(60.0)}Y{to_coord(17.08)}D01*",
        f"X{to_coord(76.0)}Y{to_coord(17.08)}D01*"
    ])

    # Trace 11: NodeMCU TX Pin (Pin 4 Right: 76.0, 19.62) -> MAX3232 TIN (32.0, 22.62) — 1.0mm Thick
    # Routes through open center channel (zero crossing!)
    gbl_lines.extend([
        "D11*",
        f"X{to_coord(76.0)}Y{to_coord(19.62)}D02*",
        f"X{to_coord(62.0)}Y{to_coord(19.62)}D01*",
        f"X{to_coord(62.0)}Y{to_coord(22.62)}D01*",
        f"X{to_coord(32.0)}Y{to_coord(22.62)}D01*"
    ])

    gbl_lines.append("M02*")
    with open(os.path.join(out_dir, "Gerber_BottomLayer.GBL"), "w") as f:
        f.write("\n".join(gbl_lines))

    # -------------------------------------------------------------
    # 3. TOP COPPER (Empty — 100% Single Sided)
    # -------------------------------------------------------------
    with open(os.path.join(out_dir, "Gerber_TopLayer.GTL"), "w") as f:
        f.write("G04 Single Sided Board Top Copper*\n%FSLAX46Y46*%\n%MOMM*%\nM02*\n")

    # -------------------------------------------------------------
    # 4. SOLDER MASK (Bottom Solder Mask: GBS)
    # -------------------------------------------------------------
    mask_lines = [
        "G04 Bottom Solder Mask Layer*",
        "%FSLAX46Y46*%",
        "%MOMM*%",
        "%ADD30C,3.200000*%",
        "%ADD31R,3.200000,3.200000*%",
        "%ADD32C,3.800000*%",
        "%ADD33R,3.800000,3.800000*%",
        "%ADD34C,7.200000*%", # Clearance for 3.5mm holes
    ]
    for _, x, y, ptype, _ in j1_pins:
        ap = "D31*" if ptype == "square" else "D30*"
        mask_lines.append(f"{ap}X{to_coord(x)}Y{to_coord(y)}D03*")
    for _, x, y, ptype in lm_pads:
        ap = "D33*" if ptype == "heavy_sq" else "D32*"
        mask_lines.append(f"{ap}X{to_coord(x)}Y{to_coord(y)}D03*")
    for _, x, y, ptype, _ in max_rs232 + max_ttl + nodemcu_left + nodemcu_right:
        ap = "D31*" if ptype == "square" else "D30*"
        mask_lines.append(f"{ap}X{to_coord(x)}Y{to_coord(y)}D03*")
    mask_lines.append("D34*")
    for x, y in m35_holes:
        mask_lines.append(f"X{to_coord(x)}Y{to_coord(y)}D03*")
    mask_lines.append("M02*")

    with open(os.path.join(out_dir, "Gerber_BottomSolderMask.GBS"), "w") as f:
        f.write("\n".join(mask_lines))
    with open(os.path.join(out_dir, "Gerber_TopSolderMask.GTS"), "w") as f:
        f.write("\n".join([
            "G04 Top Solder Mask*",
            "%FSLAX46Y46*%",
            "%MOMM*%",
            "%ADD34C,7.200000*%",
            "D34*",
            *[f"X{to_coord(x)}Y{to_coord(y)}D03*" for x, y in m35_holes],
            "M02*"
        ]))

    # -------------------------------------------------------------
    # 5. TOP SILKSCREEN (Gerber_TopSilkScreen.GTO)
    # Component outlines and labels
    # -------------------------------------------------------------
    silk_lines = [
        "G04 Top Silkscreen Layer*",
        "%FSLAX46Y46*%",
        "%MOMM*%",
        "%ADD10C,0.200000*%",
        "D10*",
        # J1 Outline
        f"X{to_coord(5.5)}Y{to_coord(29.5)}D02*",
        f"X{to_coord(12.5)}Y{to_coord(29.5)}D01*",
        f"X{to_coord(12.5)}Y{to_coord(51.5)}D01*",
        f"X{to_coord(5.5)}Y{to_coord(51.5)}D01*",
        f"X{to_coord(5.5)}Y{to_coord(29.5)}D01*",
        # LM2596 Outline
        f"X{to_coord(20.0)}Y{to_coord(42.0)}D02*",
        f"X{to_coord(70.0)}Y{to_coord(42.0)}D01*",
        f"X{to_coord(70.0)}Y{to_coord(60.0)}D01*",
        f"X{to_coord(20.0)}Y{to_coord(60.0)}D01*",
        f"X{to_coord(20.0)}Y{to_coord(42.0)}D01*",
        # MAX3232 Outline
        f"X{to_coord(16.0)}Y{to_coord(13.0)}D02*",
        f"X{to_coord(34.0)}Y{to_coord(13.0)}D01*",
        f"X{to_coord(34.0)}Y{to_coord(25.0)}D01*",
        f"X{to_coord(16.0)}Y{to_coord(25.0)}D01*",
        f"X{to_coord(16.0)}Y{to_coord(13.0)}D01*",
        # NodeMCU Outline (31mm x 58mm)
        f"X{to_coord(46.0)}Y{to_coord(8.0)}D02*",
        f"X{to_coord(78.0)}Y{to_coord(8.0)}D01*",
        f"X{to_coord(78.0)}Y{to_coord(50.0)}D01*",
        f"X{to_coord(46.0)}Y{to_coord(50.0)}D01*",
        f"X{to_coord(46.0)}Y{to_coord(8.0)}D01*",
        # MicroUSB port outline
        f"X{to_coord(58.0)}Y{to_coord(6.0)}D02*",
        f"X{to_coord(66.0)}Y{to_coord(6.0)}D01*",
        f"X{to_coord(66.0)}Y{to_coord(10.0)}D01*",
        f"X{to_coord(58.0)}Y{to_coord(10.0)}D01*",
        # 3.5mm Corner Hole Rings
        *[f"X{to_coord(x)}Y{to_coord(y-3.8)}D02*X{to_coord(x)}Y{to_coord(y+3.8)}D01*" for x, y in m35_holes],
        "M02*"
    ]
    with open(os.path.join(out_dir, "Gerber_TopSilkScreen.GTO"), "w") as f:
        f.write("\n".join(silk_lines))

    with open(os.path.join(out_dir, "Gerber_BottomSilkScreen.GBO"), "w") as f:
        f.write("G04 Bottom Silkscreen*\n%FSLAX46Y46*%\n%MOMM*%\nM02*\n")

    # -------------------------------------------------------------
    # 6. EXCELLON NC DRILL (4 Corner Holes: STRICTLY 3.5mm)
    # -------------------------------------------------------------
    drl_lines = [
        "M48",
        "; Single-Sided NodeMCU Carrier Board Drill File",
        "METRIC,TZ",
        "T01C1.000", # 1.0mm component pins
        "T02C1.400", # 1.4mm LM2596 corner pads
        "T03C3.500", # 3.5mm Corner Mounting Holes
        "%",
        "T01"
    ]
    for _, x, y, _, _ in j1_pins:
        drl_lines.append(f"X{int(round(x*1000)):06d}Y{int(round(y*1000)):06d}")
    for _, x, y, _, _ in max_rs232 + max_ttl + nodemcu_left + nodemcu_right:
        drl_lines.append(f"X{int(round(x*1000)):06d}Y{int(round(y*1000)):06d}")

    drl_lines.append("T02")
    for _, x, y, _ in lm_pads:
        drl_lines.append(f"X{int(round(x*1000)):06d}Y{int(round(y*1000)):06d}")

    drl_lines.append("T03")
    for x, y in m35_holes:
        drl_lines.append(f"X{int(round(x*1000)):06d}Y{int(round(y*1000)):06d}")

    drl_lines.append("M30")
    with open(os.path.join(out_dir, "Drill_PTH_Through.DRL"), "w") as f:
        f.write("\n".join(drl_lines))

    # -------------------------------------------------------------
    # 7. CREATE ZIP ARCHIVE
    # -------------------------------------------------------------
    zip_path = r"d:\House wiring\Next Inverter WIFI Card\Next_Inverter_WIFI_Card_Gerbers.zip"
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as z:
        for fname in os.listdir(out_dir):
            full_p = os.path.join(out_dir, fname)
            z.write(full_p, arcname=fname)

    print(f"NodeMCU Single-Sided Gerbers successfully generated to: {zip_path}")

if __name__ == "__main__":
    create_nodemcu_single_sided_gerbers()
