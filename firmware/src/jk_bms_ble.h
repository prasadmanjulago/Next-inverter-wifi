#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// JK-BMS Bluetooth Low Energy (BLE) Client & Decoder
// Multi-Characteristic Probing & Diagnostic Engine
// -----------------------------------------------------------------------------

struct JkBmsTelemetry {
    bool enabled = false;
    bool connected = false;
    bool data_valid = false;
    String device_name = "";
    String device_mac = "";
    String pin = "1234";
    int rssi = 0;

    int cell_count = 14;              // Default 14S Nissan Leaf pack
    float cell_voltages[32] = {0};    // Volts per cell
    float max_cell_v = 0.0f;
    int max_cell_idx = 0;             // 1-indexed (e.g. 1 to 14)
    float min_cell_v = 0.0f;
    int min_cell_idx = 0;             // 1-indexed (e.g. 1 to 14)
    float delta_mv = 0.0f;            // Max - Min in millivolts
    float avg_cell_v = 0.0f;

    float temp_mos = 0.0f;            // Power tube / MOSFET temperature (°C)
    float temp_sensor1 = 0.0f;        // Battery Temp 1 (°C)
    float temp_sensor2 = 0.0f;        // Battery Temp 2 (°C)

    float pack_voltage = 0.0f;        // Total battery pack voltage (V)
    float current = 0.0f;             // Pack current (A)
    int soc = 0;                      // State of charge (%)
    bool balancing = false;
    float balance_current = 0.0f;     // Balance current (A)

    uint32_t rx_packet_count = 0;
    uint32_t rx_byte_count = 0;
    uint32_t tx_cmd_count = 0;
    uint16_t write_handle = 0;
    uint16_t notify_handle = 0;
    String last_rx_hex = "None";
    String debug_info = "";

    std::vector<String> logs;
    void addLog(const String& l) {
        if (logs.size() >= 12) logs.erase(logs.begin());
        logs.push_back(l);
        Serial.println("[JK-BMS] " + l);
    }

    unsigned long last_update = 0;
    String last_status = "Not Configured";
};

struct DiscoveredBleDevice {
    String name;
    String mac;
    int rssi;
    bool is_jk;
};

class JkBmsManager {
public:
    static JkBmsManager& getInstance() {
        static JkBmsManager instance;
        return instance;
    }

    JkBmsTelemetry data;
    std::vector<DiscoveredBleDevice> scanResults;
    bool isScanning = false;

    void init() {
        Preferences prefs;
        prefs.begin("jkbms", true);
        data.enabled = prefs.getBool("enabled", false);
        data.device_mac = prefs.getString("mac", "");
        data.device_name = prefs.getString("name", "");
        data.pin = prefs.getString("pin", "1234");
        prefs.end();

        NimBLEDevice::init("NextInverter-BLE");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
        NimBLEDevice::setMTU(517);
        NimBLEDevice::setSecurityAuth(false, false, false);

        Serial.printf("[JK-BMS] Initialized. Enabled=%s, MAC=%s, Name=%s, PIN=%s\n",
                      data.enabled ? "YES" : "NO",
                      data.device_mac.c_str(),
                      data.device_mac.c_str(),
                      data.pin.c_str());
    }

    void saveConfig(bool enabled, const String& mac, const String& name, const String& pin) {
        data.enabled = enabled;
        data.device_mac = mac;
        data.device_name = name;
        data.pin = pin;

        Preferences prefs;
        prefs.begin("jkbms", false);
        prefs.putBool("enabled", enabled);
        prefs.putString("mac", mac);
        prefs.putString("name", name);
        prefs.putString("pin", pin);
        prefs.end();

        Serial.printf("[JK-BMS] Config saved: Enabled=%s, MAC=%s, Name=%s, PIN=%s\n",
                      enabled ? "YES" : "NO", mac.c_str(), name.c_str(), pin.c_str());

        if (!enabled && pClient != nullptr && pClient->isConnected()) {
            pClient->disconnect();
            data.connected = false;
            data.data_valid = false;
            data.last_status = "Disabled";
        }
    }

    void startScan(int durationSec = 4) {
        if (isScanning) return;
        isScanning = true;
        scanResults.clear();

        Serial.println("[JK-BMS] Starting BLE Scan for nearby BMS devices...");
        NimBLEScan* pScan = NimBLEDevice::getScan();
        pScan->setActiveScan(true);
        pScan->setInterval(100);
        pScan->setWindow(99);

        NimBLEScanResults results = pScan->start(durationSec, false);
        for (int i = 0; i < results.getCount(); i++) {
            NimBLEAdvertisedDevice dev = results.getDevice(i);
            String devName = dev.getName().c_str();
            String devMac = dev.getAddress().toString().c_str();
            devMac.toUpperCase();
            int rssi = dev.getRSSI();

            bool isJk = (devName.startsWith("JK") || devName.startsWith("jk") ||
                         devName.indexOf("BMS") >= 0 || devName.indexOf("B2A") >= 0 ||
                         devName.indexOf("B1A") >= 0);

            DiscoveredBleDevice d;
            d.name = (devName.length() > 0) ? devName : "Unknown BLE Device";
            d.mac = devMac;
            d.rssi = rssi;
            d.is_jk = isJk;

            if (isJk) {
                scanResults.insert(scanResults.begin(), d);
            } else {
                scanResults.push_back(d);
            }
        }
        pScan->clearResults();
        isScanning = false;
        Serial.printf("[JK-BMS] BLE Scan complete. Found %d devices.\n", (int)scanResults.size());
    }

    void loop() {
        if (!data.enabled || data.device_mac.length() < 12 || isScanning) {
            return;
        }

        unsigned long now = millis();

        // 1. Connection Management
        if (!data.connected) {
            if (now - lastConnectAttempt >= 7000) {
                lastConnectAttempt = now;
                connectToBms();
            }
            return;
        }

        // 2. Unresponsive Connection Watchdog
        if (data.connected && !data.data_valid && (now - connectTime > 35000)) {
            data.addLog("Watchdog: No telemetry in 35s. Reconnecting...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.last_status = "Connection Timeout (No Packets)";
            return;
        }

        // 3. Stale Live Telemetry Watchdog
        if (data.connected && data.data_valid && (now - data.last_update > 20000)) {
            data.addLog("Watchdog: Live stream stalled (>20s). Reconnecting...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.data_valid = false;
            data.last_status = "Connection Lost (Timeout)";
            return;
        }

        // 4. Periodic probing sequence while waiting for valid telemetry
        unsigned long queryInterval = data.data_valid ? 6000 : 2500;
        if (data.connected && (now - lastQueryTime >= queryInterval)) {
            lastQueryTime = now;

            if (data.data_valid) {
                // Keep-alive heartbeat: send 0x96 to primary write
                if (!allWriteChars.empty()) sendBmsCommand(allWriteChars[0], 0x96);
            } else {
                // Systematic cycling through probe strategies
                probeStep++;
                int step = probeStep % 6;

                if (step == 0 && pCharWriteE1) {
                    sendBmsCommand(pCharWriteE1, 0x96);
                } else if (step == 1 && pCharWriteE2) {
                    sendBmsCommand(pCharWriteE2, 0x96);
                } else if (step == 2 && data.pin.length() > 0) {
                    if (pCharWriteE1) sendAuthFrame(pCharWriteE1, data.pin);
                    if (pCharWriteE2) sendAuthFrame(pCharWriteE2, data.pin);
                } else if (step == 3 && pCharWriteE1) {
                    sendModbusQuery(pCharWriteE1);
                } else if (step == 4 && pCharWriteE2) {
                    sendModbusQuery(pCharWriteE2);
                } else if (step == 5 && allWriteChars.size() > 2) {
                    // Try other write characteristics (e.g. 0x0017 or 0x001A)
                    for (size_t i = 2; i < allWriteChars.size(); i++) {
                        sendBmsCommand(allWriteChars[i], 0x96);
                        sendModbusQuery(allWriteChars[i]);
                    }
                }
            }
        }
    }

    bool sendBmsCommand(NimBLERemoteCharacteristic* pCh, uint8_t cmdType) {
        if (pCh == nullptr) return false;
        uint8_t frame[20] = {0};
        frame[0] = 0xAA;
        frame[1] = 0x55;
        frame[2] = 0x90;
        frame[3] = 0xEB;
        frame[4] = cmdType; // 0x97: DeviceInfo, 0x96: CellInfo
        frame[5] = 0x00;
        // bytes 6..18: 0x00
        uint8_t sum = 0;
        for (int i = 0; i < 19; i++) sum += frame[i];
        frame[19] = sum;

        data.tx_cmd_count++;
        bool ok = false;
        if (pCh->canWrite()) ok = pCh->writeValue(frame, sizeof(frame), true);
        if (!ok && pCh->canWriteNoResponse()) ok = pCh->writeValue(frame, sizeof(frame), false);

        char buf[64];
        snprintf(buf, sizeof(buf), "TX 0x%02X to 0x%04X: %s", cmdType, pCh->getHandle(), ok ? "OK" : "FAIL");
        data.addLog(String(buf));
        return ok;
    }

    bool sendModbusQuery(NimBLERemoteCharacteristic* pCh) {
        if (pCh == nullptr) return false;
        uint8_t mbFrame[21] = {
            0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00,
            0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x68, 0x00, 0x00, 0x01, 0x29
        };
        data.tx_cmd_count++;
        bool ok = false;
        if (pCh->canWrite()) ok = pCh->writeValue(mbFrame, sizeof(mbFrame), true);
        if (!ok && pCh->canWriteNoResponse()) ok = pCh->writeValue(mbFrame, sizeof(mbFrame), false);
        data.addLog("TX Modbus to 0x" + String(pCh->getHandle(), HEX) + ": " + (ok ? "OK" : "FAIL"));
        return ok;
    }

    bool sendAuthFrame(NimBLERemoteCharacteristic* pCh, const String& pin) {
        if (pCh == nullptr || pin.length() == 0) return false;
        uint8_t frame[20] = {0};
        frame[0] = 0xAA;
        frame[1] = 0x55;
        frame[2] = 0x90;
        frame[3] = 0xEB;
        frame[4] = 0x05; // Auth instruction
        frame[5] = (uint8_t)std::min((size_t)10, (size_t)pin.length());
        for (size_t i = 0; i < frame[5]; i++) {
            frame[6 + i] = (uint8_t)pin[i];
        }
        uint8_t sum = 0;
        for (int i = 0; i < 19; i++) sum += frame[i];
        frame[19] = sum;

        data.tx_cmd_count++;
        bool ok = false;
        if (pCh->canWrite()) ok = pCh->writeValue(frame, sizeof(frame), true);
        if (!ok && pCh->canWriteNoResponse()) ok = pCh->writeValue(frame, sizeof(frame), false);
        data.addLog("TX Auth (" + pin + ") to 0x" + String(pCh->getHandle(), HEX) + ": " + (ok ? "OK" : "FAIL"));
        return ok;
    }

private:
    NimBLEClient* pClient = nullptr;
    NimBLERemoteCharacteristic* pCharWriteE1 = nullptr;
    NimBLERemoteCharacteristic* pCharWriteE2 = nullptr;
    std::vector<NimBLERemoteCharacteristic*> allWriteChars;
    std::vector<NimBLERemoteCharacteristic*> allNotifyChars;
    unsigned long lastConnectAttempt = 0;
    unsigned long lastQueryTime = 0;
    unsigned long connectTime = 0;
    std::vector<uint8_t> frameBuffer;
    int probeStep = 0;

    JkBmsManager() {}

    void connectToBms() {
        data.last_status = "Connecting...";
        data.addLog("Connecting to " + data.device_name + " (" + data.device_mac + ")");

        if (pClient == nullptr) {
            pClient = NimBLEDevice::createClient();
            pClient->setClientCallbacks(new ClientCallbacks(this), false);
            pClient->setConnectionParams(12, 12, 0, 300); // 15ms interval, 3s supervision
            pClient->setConnectTimeout(7);
        }

        NimBLEAddress targetAddr(data.device_mac.c_str());
        if (!pClient->connect(targetAddr)) {
            data.last_status = "Connection Failed";
            data.addLog("BLE Connect failed to " + data.device_mac);
            return;
        }

        data.rssi = pClient->getRssi();
        connectTime = millis();
        probeStep = 0;

        pCharWriteE1 = nullptr;
        pCharWriteE2 = nullptr;
        allWriteChars.clear();
        allNotifyChars.clear();
        data.debug_info = "";

        // Enumerate services & characteristics
        std::vector<NimBLERemoteService*>* pServices = pClient->getServices(true);
        if (pServices != nullptr) {
            for (auto* pSvc : *pServices) {
                String svcStr = pSvc->getUUID().toString().c_str();
                bool isSig = (svcStr.indexOf("1800") >= 0 || svcStr.indexOf("1801") >= 0);
                data.debug_info += "Svc:" + svcStr + "[";

                std::vector<NimBLERemoteCharacteristic*>* pChars = pSvc->getCharacteristics(true);
                if (pChars != nullptr) {
                    for (auto* pCh : *pChars) {
                        uint16_t h = pCh->getHandle();
                        String cStr = pCh->getUUID().toString().c_str();
                        bool w = pCh->canWrite();
                        bool wnr = pCh->canWriteNoResponse();
                        bool n = pCh->canNotify();
                        bool ind = pCh->canIndicate();

                        char buf[48];
                        snprintf(buf, sizeof(buf), "0x%04X(%s)W%dWNR%dN%d ", h, cStr.c_str(), w, wnr, n);
                        data.debug_info += buf;

                        if (!isSig) {
                            if (w || wnr) allWriteChars.push_back(pCh);
                            if (n || ind) allNotifyChars.push_back(pCh);

                            if (cStr.indexOf("ffe1") >= 0 || h == 0x000E || h == 0x0003) {
                                pCharWriteE1 = pCh;
                            }
                            if (cStr.indexOf("ffe2") >= 0 || h == 0x0011) {
                                pCharWriteE2 = pCh;
                            }
                        }
                    }
                }
                data.debug_info += "]; ";
            }
        }

        if (allNotifyChars.empty()) {
            data.last_status = "No Notify Characteristic Found";
            data.addLog("Error: No notify characteristic found");
            pClient->disconnect();
            return;
        }

        data.write_handle = pCharWriteE1 ? pCharWriteE1->getHandle() : (!allWriteChars.empty() ? allWriteChars[0]->getHandle() : 0);
        data.notify_handle = !allNotifyChars.empty() ? allNotifyChars[0]->getHandle() : 0;

        // Reset RX telemetry counters
        frameBuffer.clear();
        data.rx_packet_count = 0;
        data.rx_byte_count = 0;
        data.tx_cmd_count = 0;
        data.last_rx_hex = "None";

        // Subscribe to ALL notifiable characteristics and log each one explicitly
        for (auto* pCh : allNotifyChars) {
            uint16_t h = pCh->getHandle();
            bool ok = pCh->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->handleNotification(pChar->getHandle(), pData, length);
            });
            if (!ok) {
                ok = pCh->subscribe(false, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                    this->handleNotification(pChar->getHandle(), pData, length);
                });
            }
            data.addLog("Sub 0x" + String(h, HEX) + ": " + (ok ? "OK" : "FAIL"));
        }

        data.connected = true;
        data.last_status = "Connected (Waiting for data)";
        lastConnectAttempt = millis();

        // Send initial handshake queries: 0x97 then 0x96 to primary write
        delay(150);
        if (pCharWriteE1) {
            sendBmsCommand(pCharWriteE1, 0x97);
            delay(200);
            sendBmsCommand(pCharWriteE1, 0x96);
        } else if (!allWriteChars.empty()) {
            sendBmsCommand(allWriteChars[0], 0x97);
            delay(200);
            sendBmsCommand(allWriteChars[0], 0x96);
        }
        lastQueryTime = millis();
    }

    void handleNotification(uint16_t hdl, const uint8_t* pData, size_t length) {
        if (length == 0 || pData == nullptr) return;

        data.rx_packet_count++;
        data.rx_byte_count += length;

        // Format hex dump for diagnostics
        char hexBuf[72] = {0};
        size_t hexLen = std::min((size_t)16, length);
        for (size_t i = 0; i < hexLen; i++) {
            snprintf(hexBuf + i * 3, 4, "%02X ", pData[i]);
        }
        data.last_rx_hex = String(hexBuf);

        // Always log every packet with origin handle and byte length
        data.addLog("RX [0x" + String(hdl, HEX) + "] " + String(length) + "B: " + String(hexBuf));

        // If 3-byte response FC AA 06 arrives:
        if (length == 3 && pData[0] == 0xFC && pData[1] == 0xAA && pData[2] == 0x06) {
            if (data.pin.length() > 0) {
                if (pCharWriteE1) sendAuthFrame(pCharWriteE1, data.pin);
                if (pCharWriteE2) sendAuthFrame(pCharWriteE2, data.pin);
            }
            return;
        }

        // Check for start sequence of JK binary frame (0x55 0xAA 0xEB 0x90) or Modbus (0x4E 0x57)
        bool isNewPreamble = (length >= 4 && pData[0] == 0x55 && pData[1] == 0xAA && pData[2] == 0xEB && pData[3] == 0x90) ||
                             (length >= 2 && pData[0] == 0x4E && pData[1] == 0x57);

        if (isNewPreamble) {
            if (frameBuffer.size() >= 50) {
                decodeFrame(frameBuffer.data(), frameBuffer.size());
            }
            frameBuffer.clear();
        }

        // Append incoming bytes
        frameBuffer.insert(frameBuffer.end(), pData, pData + length);

        if (frameBuffer.size() > 450) {
            frameBuffer.clear();
            return;
        }

        // Check if assembled buffer has reached full JK02 frame (300 to 384 bytes)
        if (frameBuffer.size() >= 300) {
            if (frameBuffer[0] == 0x55 && frameBuffer[1] == 0xAA && frameBuffer[2] == 0xEB && frameBuffer[3] == 0x90) {
                decodeFrame(frameBuffer.data(), frameBuffer.size());
                frameBuffer.clear();
            } else {
                for (size_t i = 1; i + 4 <= frameBuffer.size(); i++) {
                    if (frameBuffer[i] == 0x55 && frameBuffer[i+1] == 0xAA &&
                        frameBuffer[i+2] == 0xEB && frameBuffer[i+3] == 0x90) {
                        frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + i);
                        if (frameBuffer.size() >= 300) {
                            decodeFrame(frameBuffer.data(), frameBuffer.size());
                            frameBuffer.clear();
                        }
                        break;
                    }
                }
            }
        }
        // Modbus frame support (~130 to 300 bytes)
        else if (frameBuffer.size() >= 130 && frameBuffer[0] == 0x4E && frameBuffer[1] == 0x57) {
            decodeFrame(frameBuffer.data(), frameBuffer.size());
            frameBuffer.clear();
        }
    }

    void decodeFrame(const uint8_t* buf, size_t len) {
        if (len < 10) return;

        // Format 1: JK02 Cell Info Frame (Record Type 0x02 or 0x03)
        if (buf[0] == 0x55 && buf[1] == 0xAA && buf[2] == 0xEB && buf[3] == 0x90 && (buf[4] == 0x02 || buf[4] == 0x03)) {
            float minV = 99.0f;
            float maxV = -99.0f;
            int minIdx = 1;
            int maxIdx = 1;
            float sumV = 0.0f;
            int detectedCells = 0;

            // 32 cells * 2 bytes = bytes 6 to 69 (little-endian uint16, mV)
            for (int i = 0; i < 32; i++) {
                size_t offset = 6 + i * 2;
                if (offset + 1 >= len) break;
                uint16_t rawMv = (uint16_t)buf[offset] | ((uint16_t)buf[offset + 1] << 8);
                float volt = rawMv * 0.001f;
                data.cell_voltages[i] = volt;

                // For 14S Nissan Leaf battery, evaluate valid cell voltages (typically 2.5V - 4.3V)
                if (i < 14 || (volt > 1.5f && volt < 4.8f)) {
                    if (volt > 1.5f && volt < 4.8f) {
                        detectedCells++;
                        sumV += volt;
                        if (volt < minV) { minV = volt; minIdx = i + 1; }
                        if (volt > maxV) { maxV = volt; maxIdx = i + 1; }
                    }
                }
            }

            if (detectedCells > 0) {
                data.cell_count = (detectedCells >= 14) ? 14 : detectedCells;
                data.min_cell_v = minV;
                data.min_cell_idx = minIdx;
                data.max_cell_v = maxV;
                data.max_cell_idx = maxIdx;
                data.delta_mv = (maxV - minV) * 1000.0f;
                data.avg_cell_v = (data.cell_count > 0) ? (sumV / data.cell_count) : 0.0f;
                data.pack_voltage = sumV;
            }

            // Power tube (MOSFET) Temperature
            // In JK02_32S: bytes 144-145 (int16_t * 0.1°C), in JK02_24S: bytes 130-131
            if (len >= 146) {
                int16_t rawMos = (int16_t)((uint16_t)buf[144] | ((uint16_t)buf[145] << 8));
                if (rawMos == 0 && len >= 132) {
                    rawMos = (int16_t)((uint16_t)buf[130] | ((uint16_t)buf[131] << 8));
                }
                data.temp_mos = rawMos * 0.1f;
            }

            // Battery Temperature Sensor 1
            // In JK02_32S: bytes 162-163 (int16_t * 0.1°C), in JK02_24S: bytes 132-133
            if (len >= 164) {
                int16_t rawT1 = (int16_t)((uint16_t)buf[162] | ((uint16_t)buf[163] << 8));
                if (rawT1 == 0 && len >= 134) {
                    rawT1 = (int16_t)((uint16_t)buf[132] | ((uint16_t)buf[133] << 8));
                }
                data.temp_sensor1 = rawT1 * 0.1f;
            }

            // Battery Temperature Sensor 2
            // In JK02_32S: bytes 164-165 (int16_t * 0.1°C), in JK02_24S: bytes 134-135
            if (len >= 166) {
                int16_t rawT2 = (int16_t)((uint16_t)buf[164] | ((uint16_t)buf[165] << 8));
                if (rawT2 == 0 && len >= 136) {
                    rawT2 = (int16_t)((uint16_t)buf[134] | ((uint16_t)buf[135] << 8));
                }
                data.temp_sensor2 = rawT2 * 0.1f;
            }

            // Balancing Status: byte 142
            if (len >= 143) {
                data.balancing = (buf[142] > 0);
            }

            data.data_valid = true;
            data.last_update = millis();
            data.last_status = "Connected (Live)";
            data.addLog("Live Stream OK: 14 Cells Decoded");
            return;
        }

        // Format 2: JK04 / Tag-Length-Value (Record Type 0x01)
        if (buf[0] == 0x55 && buf[1] == 0xAA && buf[2] == 0xEB && buf[3] == 0x90 && buf[4] == 0x01) {
            size_t idx = 11;
            float minV = 99.0f;
            float maxV = -99.0f;
            int minIdx = 1;
            int maxIdx = 1;
            float sumV = 0.0f;
            int detectedCells = 0;

            while (idx + 2 < len) {
                uint8_t tag = buf[idx++];
                if (tag == 0x79) { // Cell Voltages
                    uint8_t subLen = buf[idx++];
                    int nCells = subLen / 3;
                    for (int c = 0; c < nCells && c < 32; c++) {
                        if (idx + 3 > len) break;
                        uint8_t cellNum = buf[idx++];
                        uint16_t mV = ((uint16_t)buf[idx] << 8) | buf[idx + 1]; idx += 2;
                        float volt = mV * 0.001f;
                        data.cell_voltages[c] = volt;
                        if (volt > 1.5f && volt < 4.8f) {
                            detectedCells++;
                            sumV += volt;
                            if (volt < minV) { minV = volt; minIdx = c + 1; }
                            if (volt > maxV) { maxV = volt; maxIdx = c + 1; }
                        }
                    }
                } else if (tag == 0x80) { // MOS Temp
                    idx++; // skip len
                    int16_t t = ((int16_t)buf[idx] << 8) | buf[idx + 1]; idx += 2;
                    data.temp_mos = (t > 100) ? (100 - t) : (float)t;
                } else if (tag == 0x81) { // Batt Temp 1
                    idx++;
                    int16_t t = ((int16_t)buf[idx] << 8) | buf[idx + 1]; idx += 2;
                    data.temp_sensor1 = (t > 100) ? (100 - t) : (float)t;
                } else if (tag == 0x82) { // Batt Temp 2
                    idx++;
                    int16_t t = ((int16_t)buf[idx] << 8) | buf[idx + 1]; idx += 2;
                    data.temp_sensor2 = (t > 100) ? (100 - t) : (float)t;
                } else if (tag == 0x83) { // Total Voltage
                    idx++;
                    uint16_t tv = ((uint16_t)buf[idx] << 8) | buf[idx + 1]; idx += 2;
                    data.pack_voltage = tv * 0.01f;
                } else if (tag == 0x85) { // SOC
                    data.soc = buf[idx++];
                } else {
                    idx++;
                }
            }

            if (detectedCells > 0) {
                data.cell_count = (detectedCells >= 14) ? 14 : detectedCells;
                data.min_cell_v = minV;
                data.min_cell_idx = minIdx;
                data.max_cell_v = maxV;
                data.max_cell_idx = maxIdx;
                data.delta_mv = (maxV - minV) * 1000.0f;
                data.avg_cell_v = (data.cell_count > 0) ? (sumV / data.cell_count) : 0.0f;
                data.data_valid = true;
                data.last_update = millis();
                data.last_status = "Connected (Live)";
                data.addLog("Live Stream OK: TLV Cells Decoded");
            }
            return;
        }

        // Format 3: Modbus Header 0x4E 0x57
        if (buf[0] == 0x4E && buf[1] == 0x57 && len >= 40) {
            uint8_t cells = buf[11] / 3;
            if (cells > 0 && cells <= 32) {
                float minV = 99.0f;
                float maxV = -99.0f;
                int minIdx = 1;
                int maxIdx = 1;
                float sumV = 0.0f;
                for (uint8_t i = 0; i < cells && (12 + i * 3 + 2 < len); i++) {
                    uint16_t rawMv = ((uint16_t)buf[12 + i * 3 + 1] << 8) | buf[12 + i * 3 + 2];
                    float volt = rawMv * 0.001f;
                    data.cell_voltages[i] = volt;
                    sumV += volt;
                    if (volt < minV) { minV = volt; minIdx = i + 1; }
                    if (volt > maxV) { maxV = volt; maxIdx = i + 1; }
                }
                data.cell_count = (cells >= 14) ? 14 : cells;
                data.min_cell_v = minV;
                data.min_cell_idx = minIdx;
                data.max_cell_v = maxV;
                data.max_cell_idx = maxIdx;
                data.delta_mv = (maxV - minV) * 1000.0f;
                data.avg_cell_v = (data.cell_count > 0) ? (sumV / data.cell_count) : 0.0f;
                data.pack_voltage = sumV;
                data.data_valid = true;
                data.last_update = millis();
                data.last_status = "Connected (Live)";
                data.addLog("Live Stream OK: Modbus Decoded");
            }
        }
    }

    class ClientCallbacks : public NimBLEClientCallbacks {
        JkBmsManager* mgr;
    public:
        ClientCallbacks(JkBmsManager* m) : mgr(m) {}
        void onConnect(NimBLEClient* pClient) override {
            mgr->data.addLog("BLE Client Connected Callback");
        }
        void onDisconnect(NimBLEClient* pClient) override {
            mgr->data.addLog("BLE Client Disconnected Callback");
            mgr->data.connected = false;
            mgr->data.data_valid = false;
            mgr->data.last_status = "Disconnected";
        }
    };
};
