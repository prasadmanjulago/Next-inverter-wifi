# Next Inverter 6.2kW Wi-Fi Telemetry & ThingsBoard Cloud Documentation
**Date:** October 22, 2026  
**Firmware Version:** v1.5.0  
**Repository:** [prasadmanjulago/Next-inverter-wifi](https://github.com/prasadmanjulago/Next-inverter-wifi)  

---

## 1. System Architecture Overview

The system provides 24/7 solar telemetry, remote diagnostics, and cloud monitoring for the Next 6.2kW Hybrid Solar Inverter and 14S Li-ion Battery Storage:

```
[ Next 6.2kW Inverter ] 
        │ RS-232 (2400 Baud, MAX3232, GPIO 16/17)
        ▼
[ ESP32 Wi-Fi Telemetry Card ] ◄── BLE (NimBLE) ── [ JK-BMS 14S Battery ]
        │
        ├── Embedded Web Dashboard (http://192.168.1.15)
        ├── Direct Wireless OTA Updates (/update)
        │
        ▼ HTTP REST Telemetry (5s Interval)
[ Oracle Cloud Free Tier VM (161.118.190.164:8080) ]
        │
        ├── Nginx Reverse Proxy (GZIP Compression, WebSocket Upgrade)
        ├── ThingsBoard Community Edition (v4.2.1.1 Docker)
        └── PostgreSQL 12 (Tuned Query Planner, VACUUM ANALYZE)
```

---

## 2. Firmware v1.5.0 Key Changes

- **Blynk IoT Removal**:
  - Completely purged all Blynk IoT libraries, background HTTP tasks, stored preferences, web API endpoints (`/api/blynk_config`), and dashboard UI elements.
  - Reclaimed ~13 KB flash and significant dynamic SRAM (eliminated `WiFiClientSecure` SSL memory buffers).
- **ThingsBoard Telemetry Engine**:
  - Direct HTTP POST to ThingsBoard REST API (`http://161.118.190.164:8080/api/v1/{token}/telemetry`).
  - Automatic rolling sample averaging over telemetry send interval.
  - Real-time status reporting on local dashboard (`tb_status: "Sent OK (1 avg)"`).
- **JK-BMS 14S Bluetooth Monitoring**:
  - Passive/Silent BLE monitoring using NimBLE (no buzzer beeps).
  - Telemetry: 14 individual cell voltages, pack voltage, MOS temperature, sensor temperatures, charge/discharge status, balancing state.
- **Wireless Web OTA**:
  - Browser upload and curl-compatible endpoint at `/update`.
  - Dedicates 100% CPU during OTA flash writing to prevent timeouts.

---

## 3. Oracle Cloud ThingsBoard Infrastructure

- **Server IP:** `161.118.190.164` (Port 8080)
- **Host Location:** `D:\House wiring\oracle\`
- **SSH Access:**
  ```bash
  ssh -i "D:\House wiring\oracle\ssh-key-2026-10-22.key" ubuntu@161.118.190.164
  ```
- **Docker Container:** `thingsboard-mytb-1` (`thingsboard/tb-postgres:latest`)
- **Web UI URL:** `http://161.118.190.164:8080`
- **Dashboard:** `Next 6.2 Inverter Dashboard` (`c1066830-cdc5-11f1-b27b-bda6876cd835`)
- **Device Provisioned:** `Next 6.2kW Inverter` (`7f061d60-cdc3-11f1-b27b-bda6876cd835`)
- **Device Access Token:** `hRIwZ2yNvOPsitjsjOtD`

### Server Optimizations Applied:
1. **Nginx Reverse Proxy with GZIP Compression:**
   - Compresses JavaScript bundles and JSON payloads (`Content-Encoding: gzip`).
   - Slashes initial dashboard download from 15MB to ~2.5MB (6x faster load).
   - Configured with `map $http_upgrade $connection_upgrade` for bidirectional WebSockets.
2. **PostgreSQL Database Tuning:**
   - Ran `VACUUM ANALYZE` on `ts_kv` and all system tables.
   - Slashed query ingestion latency from 13.4s to 0.79s (17x speedup).
3. **Linux Kernel Memory Tuning:**
   - `vm.swappiness = 10`
   - `vm.vfs_cache_pressure = 50`
   - Prevents micro-VM memory thrashing onto disk swap.

---

## 4. ThingsBoard Chart Y-Axis Configuration

- **Widget:** Time Series Chart (ID: `2c028616-bf74-4a67-e0b7-30ceabe8a572`)
- **Keys Tracked:** `load_watts` (Load), `solar_watts` (PV Input), `grid_watts` (Grid Input), `battery_power_watts` (Battery Input)
- **Axis Title:** `W` (`label: "W"`)
- **Tick Value Formatter:** `return value + ' W';` (Renders `0 W`, `500 W`, `1000 W`, etc.)
- **Configuration File:** `d:\Downloads\time_series_chart_time_series_chart.json`

---

## 5. Security & Confidentiality Audit

### What is Safe (NOT in GitHub):
- Oracle VM Private SSH Key (`D:\House wiring\oracle\ssh-key-2026-10-22.key`) is in an external directory outside the repository.
- Oracle Server IP (`161.118.190.164`) is not in the repository.
- Live ThingsBoard Device Token (`hRIwZ2yNvOPsitjsjOtD`) is not in the repository.

### What is Exposed on Public GitHub:
1. **Home Wi-Fi Name & Password:**
   - `firmware/src/main.cpp`:
     ```cpp
     String ssid = preferences.getString("ssid", "Easytech Solutions");
     String pass = preferences.getString("password", "ilen6039");
     ```
2. **Old ThingsBoard Cloud Token:**
   - `tbToken = preferences.getString("token", "QJJXWvLJZ4GAECdglKb3");` in `main.cpp`.
3. **Compiled `firmware.bin`:**
   - Contains the compiled strings above.

### Recommended Actions:
1. Sanitize `main.cpp` default fallback credentials to `""` (the ESP32 already has the credentials safely saved in internal flash NVS).
2. Change GitHub repository visibility to **Private** to protect past commit history.
3. If necessary, rotate your home Wi-Fi password.
