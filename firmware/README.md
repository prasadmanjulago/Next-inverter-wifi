# NEXT 6.2kW Inverter Wi-Fi Firmware

ESP32-based standalone Wi-Fi telemetry card for NEXT NM-ECO-6.2KW PLUS hybrid inverter.

## Pinout
- **GPIO 16 (RX2)**: Connects to MAX3232 `ROUT` (Inverter RS-232 RX)
- **GPIO 17 (TX2)**: Connects to MAX3232 `TIN` (Inverter RS-232 TX)
- **VIN**: 5V DC power supply from LM2596 buck converter
- **3V3**: Powers Mini MAX3232 VCC safely at 3.3V logic
- **GND**: Common Ground

## Quick Flashing
```bash
pio run -t upload
```

## Dashboard Features
- 1:1 SmartESS mobile app power flow diagram with animated flowing pulses.
- Inverter Connection Status pill (`🟢 Inverter Connected` / `🔴 Inverter Disconnected`).
- Auto 2.4GHz Wi-Fi scanner dropdown + fallback AP (`NextInverter-WIFI` at `192.168.4.1`).
- REST API at `/api/telemetry` and `/api/wifi_scan`.

## Wireless OTA Updates (No USB Cable Needed!)
1. **GitHub OTA**: In Settings, enter your GitHub `username/repository`. The dashboard automatically checks GitHub for updates. When a newer version is found, an **"Update Now"** banner appears to flash the ESP32 in 1 click over Wi-Fi!
2. **Browser Direct Flash**: Open Settings, choose any compiled `firmware.bin` from your PC, and click **"Flash Selected .bin Wirelessly"**. Flashes over Wi-Fi with live progress bar in 10 seconds.
