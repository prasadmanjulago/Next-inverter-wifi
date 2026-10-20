#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// JK-BMS Bluetooth Low Energy (BLE) Client & Decoder
// Dual-Core FreeRTOS Architecture:
// - Pinned to Core 0 (Background BLE)
// - Leaves Core 1 100% Free for Web Server, Dashboard & Inverter UART
// - Lock-Free Ring Buffer, Zero-Deadlock, Coexistence-Friendly
// -----------------------------------------------------------------------------

// Global OTA flag from main.cpp
extern bool isOtaUpdating;

struct JkBmsTelemetry {
    bool enabled = false;
    bool connected = false;
    bool data_valid = false;
    String device_name = "";
    String device_mac = "";
    String pin = "114433";
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
        if (logs.size() >= 10) logs.erase(logs.begin());
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
        data.pin = prefs.getString("pin", "114433");
        prefs.end();

        NimBLEDevice::init("NextInverter-BLE");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
        NimBLEDevice::setMTU(517);
        NimBLEDevice::setSecurityAuth(false, false, false);

        Serial.printf("[JK-BMS] Initialized. Enabled=%s, MAC=%s, Name=%s, PIN=%s\n",
                      data.enabled ? "YES" : "NO",
                      data.device_mac.c_str(),
                      data.device_name.c_str(),
                      data.pin.c_str());

        // Spawn dedicated BLE FreeRTOS task on Core 0
        startBleTask();
    }

    void startBleTask() {
        if (taskHandle == nullptr) {
            xTaskCreatePinnedToCore(
                [](void* param) {
                    JkBmsManager* mgr = (JkBmsManager*)param;
                    mgr->taskLoop();
                },
                "jkbms_task",
                4096,
                this,
                1,
                &taskHandle,
                0 // Core 0 (Leaves Core 1 100% free for Web Server)
            );
        }
    }

    void taskLoop() {
        while (true) {
            // During wireless OTA upload, pause BLE completely so Wi-Fi has 100% bandwidth
            if (isOtaUpdating) {
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            if (data.enabled && data.device_mac.length() >= 12 && !isScanning) {
                stepBle();
            }

            // Yield 50ms so Core 0 stays relaxed
            vTaskDelay(pdMS_TO_TICKS(50));
        }
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
            pCharWrite1 = nullptr;
            pCharWrite2 = nullptr;
            allNotifyChars.clear();
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

    // Called from Arduino main loop (kept ultra-lightweight)
    void loop() {
        // BLE execution is isolated to Core 0 task.
    }

    // Ultra-fast, zero-allocation notification callback running in NimBLE Host Task
    void onNotifyRaw(uint16_t hdl, const uint8_t* pData, size_t length) {
        if (pData == nullptr || length == 0) return;
        data.rx_packet_count++;
        data.rx_byte_count += length;
        data.notify_handle = hdl;

        for (size_t i = 0; i < length; i++) {
            size_t nextHead = (rxHead + 1) % RX_RING_SIZE;
            if (nextHead != rxTail) {
                rxRing[rxHead] = pData[i];
                rxHead = nextHead;
            }
        }
    }

    bool sendBmsCommand(uint8_t cmdType) {
        if (pCharWrite1 == nullptr && pCharWrite2 == nullptr) return false;
        uint8_t frame[20] = {0};
        frame[0] = 0xAA;
        frame[1] = 0x55;
        frame[2] = 0x90;
        frame[3] = 0xEB;
        frame[4] = cmdType; // 0x97: DeviceInfo, 0x96: CellInfo
        frame[5] = 0x00;
        uint8_t sum = 0;
        for (int i = 0; i < 19; i++) sum += frame[i];
        frame[19] = sum;

        data.tx_cmd_count++;
        // Strictly Write Without Response (false) - 100% non-blocking!
        bool ok = false;
        if (pCharWrite1 != nullptr) {
            ok = pCharWrite1->writeValue(frame, sizeof(frame), false);
        }
        if (pCharWrite2 != nullptr) {
            pCharWrite2->writeValue(frame, sizeof(frame), false);
        }

        char buf[36];
        snprintf(buf, sizeof(buf), "TX 0x%02X: %s", cmdType, ok ? "OK" : "FAIL");
        data.addLog(String(buf));
        return ok;
    }

    bool sendAuthFrame(const String& pin) {
        if ((pCharWrite1 == nullptr && pCharWrite2 == nullptr) || pin.length() == 0) return false;
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
        // Strictly Write Without Response (false) - 100% non-blocking!
        bool ok = false;
        if (pCharWrite1 != nullptr) {
            ok = pCharWrite1->writeValue(frame, sizeof(frame), false);
        }
        if (pCharWrite2 != nullptr) {
            pCharWrite2->writeValue(frame, sizeof(frame), false);
        }

        data.addLog("TX Auth (" + pin + "): " + (ok ? "OK" : "FAIL"));
        return ok;
    }

private:
    TaskHandle_t taskHandle = nullptr;
    NimBLEClient* pClient = nullptr;
    NimBLERemoteCharacteristic* pCharWrite1 = nullptr;
    NimBLERemoteCharacteristic* pCharWrite2 = nullptr;
    std::vector<NimBLERemoteCharacteristic*> allNotifyChars;
    unsigned long lastConnectAttempt = 0;
    unsigned long lastQueryTime = 0;
    unsigned long connectTime = 0;

    // Fast lock-free ring buffer for notification reception
    static const size_t RX_RING_SIZE = 1024;
    uint8_t rxRing[RX_RING_SIZE];
    volatile size_t rxHead = 0;
    volatile size_t rxTail = 0;

    // Reassembly buffer in main thread
    std::vector<uint8_t> frameBuffer;

    JkBmsManager() {}

    void stepBle() {
        unsigned long now = millis();

        // 1. Connection Management
        if (!data.connected) {
            if (now - lastConnectAttempt >= 6000) {
                lastConnectAttempt = now;
                connectToBms();
            }
            return;
        }

        // 2. Drain incoming BLE ring buffer safely in task thread (Zero Deadlock)
        drainRxRingBuffer();

        // 3. Stalled / Unresponsive Connection Watchdog
        if (data.connected && !data.data_valid && (now - connectTime > 30000)) {
            data.addLog("Watchdog: No telemetry in 30s. Reconnecting...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.last_status = "Connection Timeout (No Packets)";
            pCharWrite1 = nullptr;
            pCharWrite2 = nullptr;
            allNotifyChars.clear();
            return;
        }

        if (data.connected && data.data_valid && (now - data.last_update > 20000)) {
            data.addLog("Watchdog: Live stream stalled (>20s). Reconnecting...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.data_valid = false;
            data.last_status = "Connection Lost (Timeout)";
            pCharWrite1 = nullptr;
            pCharWrite2 = nullptr;
            allNotifyChars.clear();
            return;
        }

        // 4. Periodic Query Scheduling (Using Write Without Response ONLY)
        unsigned long queryInterval = data.data_valid ? 5000 : 2500;
        if (data.connected && (now - lastQueryTime >= queryInterval)) {
            lastQueryTime = now;
            sendBmsCommand(0x96);
        }
    }

    void connectToBms() {
        data.last_status = "Connecting...";
        data.addLog("Connecting to " + data.device_name + " (" + data.device_mac + ")");

        if (pClient == nullptr) {
            pClient = NimBLEDevice::createClient();
            pClient->setClientCallbacks(new ClientCallbacks(this), false);
            // Coexistence-friendly: 30ms-50ms interval (24-40 * 1.25ms), 4.0s supervision timeout
            pClient->setConnectionParams(24, 40, 0, 400);
            pClient->setConnectTimeout(5);
        }

        NimBLEAddress targetAddr(data.device_mac.c_str());
        if (!pClient->connect(targetAddr)) {
            data.last_status = "Connection Failed";
            data.addLog("BLE Connect failed to " + data.device_mac);
            return;
        }

        data.rssi = pClient->getRssi();
        connectTime = millis();
        pCharWrite1 = nullptr;
        pCharWrite2 = nullptr;
        allNotifyChars.clear();

        // Reset buffers
        rxHead = 0;
        rxTail = 0;
        frameBuffer.clear();
        data.rx_packet_count = 0;
        data.rx_byte_count = 0;
        data.tx_cmd_count = 0;
        data.last_rx_hex = "None";

        // Discover services and find write & notify characteristics
        NimBLERemoteService* pSvcE0 = pClient->getService(NimBLEUUID((uint16_t)0xFFE0));
        NimBLERemoteService* pSvc10 = pClient->getService(NimBLEUUID((uint16_t)0xFF10));

        if (pSvcE0 != nullptr) {
            auto* c1 = pSvcE0->getCharacteristic(NimBLEUUID((uint16_t)0xFFE1));
            auto* c2 = pSvcE0->getCharacteristic(NimBLEUUID((uint16_t)0xFFE2));
            if (c1) {
                if (c1->canWrite() || c1->canWriteNoResponse()) pCharWrite1 = c1;
                if (c1->canNotify() || c1->canIndicate()) allNotifyChars.push_back(c1);
            }
            if (c2) {
                if (!pCharWrite1 && (c2->canWrite() || c2->canWriteNoResponse())) pCharWrite1 = c2;
                if (c2->canNotify() || c2->canIndicate()) allNotifyChars.push_back(c2);
            }
        }

        if (pSvc10 != nullptr) {
            auto* c11 = pSvc10->getCharacteristic(NimBLEUUID((uint16_t)0xFF11));
            auto* c12 = pSvc10->getCharacteristic(NimBLEUUID((uint16_t)0xFF12));
            if (c11 && (c11->canNotify() || c11->canIndicate())) allNotifyChars.push_back(c11);
            if (c12 && (c12->canNotify() || c12->canIndicate())) allNotifyChars.push_back(c12);
        }

        // Fallback: If neither found by UUID, scan all services
        if (pCharWrite1 == nullptr && allNotifyChars.empty()) {
            std::vector<NimBLERemoteService*>* pServices = pClient->getServices(true);
            if (pServices != nullptr) {
                for (auto* s : *pServices) {
                    String sUuid = s->getUUID().toString().c_str();
                    if (sUuid.indexOf("1800") >= 0 || sUuid.indexOf("1801") >= 0) continue;
                    std::vector<NimBLERemoteCharacteristic*>* pChars = s->getCharacteristics(true);
                    if (pChars != nullptr) {
                        for (auto* c : *pChars) {
                            if ((c->canWrite() || c->canWriteNoResponse()) && pCharWrite1 == nullptr) {
                                pCharWrite1 = c;
                            }
                            if (c->canNotify() || c->canIndicate()) {
                                allNotifyChars.push_back(c);
                            }
                        }
                    }
                }
            }
        }

        if (pCharWrite1 == nullptr || allNotifyChars.empty()) {
            data.last_status = "Data Characteristic Not Found";
            data.addLog("Error: Write or Notify Characteristic not found");
            pClient->disconnect();
            return;
        }

        data.write_handle = pCharWrite1 ? pCharWrite1->getHandle() : 0;
        data.notify_handle = allNotifyChars[0]->getHandle();

        // Subscribe to notify characteristics with safe, non-blocking callback
        for (auto* ch : allNotifyChars) {
            uint16_t h = ch->getHandle();
            bool subOk = ch->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->onNotifyRaw(pChar->getHandle(), pData, length);
            });
            if (!subOk) {
                subOk = ch->subscribe(false, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                    this->onNotifyRaw(pChar->getHandle(), pData, length);
                });
            }
            data.addLog("Sub 0x" + String(h, HEX) + ": " + (subOk ? "OK" : "FAIL"));
        }

        data.connected = true;
        data.last_status = "Connected (Waiting for data)";
        lastConnectAttempt = millis();

        // Initial handshake: query device info (0x97) and status (0x96)
        sendBmsCommand(0x97);
        sendBmsCommand(0x96);
        lastQueryTime = millis();
    }

    void drainRxRingBuffer() {
        if (rxTail == rxHead) return;

        while (rxTail != rxHead) {
            uint8_t b = rxRing[rxTail];
            rxTail = (rxTail + 1) % RX_RING_SIZE;
            frameBuffer.push_back(b);
        }

        // Format recent hex for UI diagnostics
        if (!frameBuffer.empty()) {
            char hexBuf[50] = {0};
            size_t n = std::min((size_t)12, frameBuffer.size());
            for (size_t i = 0; i < n; i++) {
                snprintf(hexBuf + i * 3, 4, "%02X ", frameBuffer[i]);
            }
            data.last_rx_hex = String(hexBuf);
        }

        // Check for 3-byte status packet: FC AA 06 (Auth request or status code)
        if (frameBuffer.size() >= 3) {
            for (size_t i = 0; i + 3 <= frameBuffer.size(); i++) {
                if (frameBuffer[i] == 0xFC && frameBuffer[i+1] == 0xAA && frameBuffer[i+2] == 0x06) {
                    data.addLog("RX: FC AA 06 -> Sending Auth PIN (" + data.pin + ")");
                    sendAuthFrame(data.pin);
                    sendBmsCommand(0x96);
                    frameBuffer.erase(frameBuffer.begin() + i, frameBuffer.begin() + i + 3);
                    break;
                }
            }
        }

        // Prevent buffer bloat
        if (frameBuffer.size() > 600) {
            frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + 200);
        }

        // Check for JK02 preamble: 0x55 0xAA 0xEB 0x90
        for (size_t i = 0; i + 4 <= frameBuffer.size(); i++) {
            if (frameBuffer[i] == 0x55 && frameBuffer[i+1] == 0xAA &&
                frameBuffer[i+2] == 0xEB && frameBuffer[i+3] == 0x90) {

                if (i > 0) {
                    frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + i);
                }

                // JK02 full telemetry is ~300 bytes
                if (frameBuffer.size() >= 300) {
                    decodeFrame(frameBuffer.data(), frameBuffer.size());
                    frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + 300);
                }
                return;
            }
        }

        // Check for Modbus preamble: 0x4E 0x57
        for (size_t i = 0; i + 2 <= frameBuffer.size(); i++) {
            if (frameBuffer[i] == 0x4E && frameBuffer[i+1] == 0x57) {
                if (i > 0) {
                    frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + i);
                }
                if (frameBuffer.size() >= 130) {
                    decodeFrame(frameBuffer.data(), frameBuffer.size());
                    frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + 130);
                }
                return;
            }
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

                // For 14S Nissan Leaf pack, evaluate valid cell voltages (typically 2.5V - 4.3V)
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
            if (len >= 146) {
                int16_t rawMos = (int16_t)((uint16_t)buf[144] | ((uint16_t)buf[145] << 8));
                if (rawMos == 0 && len >= 132) {
                    rawMos = (int16_t)((uint16_t)buf[130] | ((uint16_t)buf[131] << 8));
                }
                data.temp_mos = rawMos * 0.1f;
            }

            // Battery Temperature Sensor 1
            if (len >= 164) {
                int16_t rawT1 = (int16_t)((uint16_t)buf[162] | ((uint16_t)buf[163] << 8));
                if (rawT1 == 0 && len >= 134) {
                    rawT1 = (int16_t)((uint16_t)buf[132] | ((uint16_t)buf[133] << 8));
                }
                data.temp_sensor1 = rawT1 * 0.1f;
            }

            // Battery Temperature Sensor 2
            if (len >= 166) {
                int16_t rawT2 = (int16_t)((uint16_t)buf[164] | ((uint16_t)buf[165] << 8));
                if (rawT2 == 0 && len >= 136) {
                    rawT2 = (int16_t)((uint16_t)buf[134] | ((uint16_t)buf[135] << 8));
                }
                data.temp_sensor2 = rawT2 * 0.1f;
            }

            // Balancing Status
            if (len >= 143) {
                data.balancing = (buf[142] > 0);
            }

            data.data_valid = true;
            data.last_update = millis();
            data.last_status = "Connected (Live)";
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
                    idx++;
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
            }
        }
    }

    class ClientCallbacks : public NimBLEClientCallbacks {
        JkBmsManager* mgr;
    public:
        ClientCallbacks(JkBmsManager* m) : mgr(m) {}
        void onConnect(NimBLEClient* pClient) override {
            mgr->data.addLog("BLE Client Connected");
        }
        void onDisconnect(NimBLEClient* pClient) override {
            mgr->data.addLog("BLE Client Disconnected");
            mgr->data.connected = false;
            mgr->data.data_valid = false;
            mgr->data.last_status = "Disconnected";
            mgr->pCharWrite1 = nullptr;
            mgr->pCharWrite2 = nullptr;
            mgr->allNotifyChars.clear();
        }
    };
};
