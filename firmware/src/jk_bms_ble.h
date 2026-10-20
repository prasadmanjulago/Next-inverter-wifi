#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// JK-BMS Bluetooth Low Energy (BLE) Client & Decoder
// Open-Source Auto-Streaming Architecture (syssi/esphome-jk-bms standard)
// - NO Auth / PIN spam (Pin is only for settings, NOT for reading telemetry)
// - Sends 0x97 and 0x96 ONCE on connect -> BMS auto-streams Frame 0x02
// - Subscribes to notify channels across 0xFFE0 & 0xFF10
// - ZERO constant beeping, ZERO radio saturation
// - Dedicated FreeRTOS Task pinned to Core 0 (WebServer on Core 1 remains 100% fluid)
// -----------------------------------------------------------------------------

extern bool isOtaUpdating;

struct JkBmsTelemetry {
    bool enabled = false;
    bool connected = false;
    bool data_valid = false;
    String device_name = "";
    String device_mac = "";
    String pin = "";
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
        data.pin = prefs.getString("pin", "");
        prefs.end();

        NimBLEDevice::init("NextInverter-BLE");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
        NimBLEDevice::setMTU(517);
        NimBLEDevice::setSecurityAuth(false, false, false);

        Serial.printf("[JK-BMS] Initialized. Enabled=%s, MAC=%s, Name=%s\n",
                      data.enabled ? "YES" : "NO",
                      data.device_mac.c_str(),
                      data.device_name.c_str());

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
                0 // Core 0 (Leaves Core 1 completely free for WebServer)
            );
        }
    }

    void taskLoop() {
        while (true) {
            // While receiving OTA updates, pause BLE completely
            if (isOtaUpdating) {
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            if (data.enabled && data.device_mac.length() >= 12 && !isScanning) {
                stepBle();
            }

            // Relaxed 100ms task rate: keeps Core 0 cool and leaves plenty of 2.4GHz RF for Wi-Fi
            vTaskDelay(pdMS_TO_TICKS(100));
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

        Serial.printf("[JK-BMS] Config saved: Enabled=%s, MAC=%s, Name=%s\n",
                      enabled ? "YES" : "NO", mac.c_str(), name.c_str());

        if (!enabled && pClient != nullptr && pClient->isConnected()) {
            pClient->disconnect();
            data.connected = false;
            data.data_valid = false;
            data.last_status = "Disabled";
            allWriteChars.clear();
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

    void loop() {
        // BLE runs in dedicated FreeRTOS task on Core 0
    }

    // Ultra-fast lock-free ring buffer
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
        if (allWriteChars.empty()) return false;
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
        bool ok = false;
        for (auto* wCh : allWriteChars) {
            if (wCh->writeValue(frame, sizeof(frame), false)) {
                ok = true;
            }
        }
        char buf[36];
        snprintf(buf, sizeof(buf), "TX 0x%02X: %s", cmdType, ok ? "OK" : "FAIL");
        data.addLog(String(buf));
        return ok;
    }

private:
    TaskHandle_t taskHandle = nullptr;
    NimBLEClient* pClient = nullptr;
    std::vector<NimBLERemoteCharacteristic*> allWriteChars;
    std::vector<NimBLERemoteCharacteristic*> allNotifyChars;
    unsigned long lastConnectAttempt = 0;
    unsigned long lastHeartbeatTime = 0;
    unsigned long connectTime = 0;

    static const size_t RX_RING_SIZE = 1024;
    uint8_t rxRing[RX_RING_SIZE];
    volatile size_t rxHead = 0;
    volatile size_t rxTail = 0;

    std::vector<uint8_t> frameBuffer;

    JkBmsManager() {}

    void stepBle() {
        unsigned long now = millis();

        // 1. Connection Management
        if (!data.connected) {
            if (now - lastConnectAttempt >= 7000) {
                lastConnectAttempt = now;
                connectToBms();
            }
            return;
        }

        // 2. Drain incoming BLE ring buffer
        drainRxRingBuffer();

        // 3. Stalled / Unresponsive Connection Watchdog
        if (data.connected && !data.data_valid && (now - connectTime > 30000)) {
            data.addLog("Watchdog: No data stream in 30s. Reconnecting...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.last_status = "Connection Timeout (No Packets)";
            allWriteChars.clear();
            allNotifyChars.clear();
            return;
        }

        if (data.connected && data.data_valid && (now - data.last_update > 25000)) {
            data.addLog("Watchdog: Stream stalled (>25s). Re-sending 0x96...");
            sendBmsCommand(0x96);
            data.last_update = now;
        }
    }

    void connectToBms() {
        data.last_status = "Connecting...";
        data.addLog("Connecting to " + data.device_name + " (" + data.device_mac + ")");

        if (pClient == nullptr) {
            pClient = NimBLEDevice::createClient();
            pClient->setClientCallbacks(new ClientCallbacks(this), false);
            // Coexistence-friendly: 30ms-50ms interval, 4.0s supervision timeout
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
        allWriteChars.clear();
        allNotifyChars.clear();

        rxHead = 0;
        rxTail = 0;
        frameBuffer.clear();
        data.rx_packet_count = 0;
        data.rx_byte_count = 0;
        data.tx_cmd_count = 0;
        data.last_rx_hex = "None";

        // Find characteristics across both Service 0xFFE0 and Service 0xFF10
        NimBLERemoteService* pSvcE0 = pClient->getService(NimBLEUUID((uint16_t)0xFFE0));
        NimBLERemoteService* pSvc10 = pClient->getService(NimBLEUUID((uint16_t)0xFF10));

        if (pSvcE0 != nullptr) {
            auto* c1 = pSvcE0->getCharacteristic(NimBLEUUID((uint16_t)0xFFE1));
            auto* c2 = pSvcE0->getCharacteristic(NimBLEUUID((uint16_t)0xFFE2));
            if (c1) {
                if (c1->canWrite() || c1->canWriteNoResponse()) allWriteChars.push_back(c1);
                if (c1->canNotify() || c1->canIndicate()) allNotifyChars.push_back(c1);
            }
            if (c2) {
                if (c2->canWrite() || c2->canWriteNoResponse()) allWriteChars.push_back(c2);
                if (c2->canNotify() || c2->canIndicate()) allNotifyChars.push_back(c2);
            }
        }

        if (pSvc10 != nullptr) {
            auto* c11 = pSvc10->getCharacteristic(NimBLEUUID((uint16_t)0xFF11));
            auto* c12 = pSvc10->getCharacteristic(NimBLEUUID((uint16_t)0xFF12));
            if (c11) {
                if (c11->canWrite() || c11->canWriteNoResponse()) allWriteChars.push_back(c11);
                if (c11->canNotify() || c11->canIndicate()) allNotifyChars.push_back(c11);
            }
            if (c12) {
                if (c12->canWrite() || c12->canWriteNoResponse()) allWriteChars.push_back(c12);
                if (c12->canNotify() || c12->canIndicate()) allNotifyChars.push_back(c12);
            }
        }

        if (allWriteChars.empty() || allNotifyChars.empty()) {
            data.last_status = "Data Characteristics Not Found";
            data.addLog("Error: Write or Notify Characteristic not found");
            pClient->disconnect();
            return;
        }

        data.write_handle = allWriteChars[0]->getHandle();
        data.notify_handle = allNotifyChars[0]->getHandle();

        // Subscribe to all available notify channels (0xFFE1, 0xFFE2, 0xFF11, 0xFF12)
        for (auto* nCh : allNotifyChars) {
            uint16_t h = nCh->getHandle();
            bool subOk = nCh->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->onNotifyRaw(pChar->getHandle(), pData, length);
            });
            if (!subOk) {
                subOk = nCh->subscribe(false, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                    this->onNotifyRaw(pChar->getHandle(), pData, length);
                });
            }
            data.addLog("Sub 0x" + String(h, HEX) + ": " + (subOk ? "OK" : "FAIL"));
        }

        data.connected = true;
        data.last_status = "Connected (Waiting for data)";
        lastConnectAttempt = millis();

        // syssi standard: Send 0x97 once, wait 200ms, send 0x96 once
        // This triggers the JK-BMS to automatically start streaming Frame 0x02
        vTaskDelay(pdMS_TO_TICKS(150));
        sendBmsCommand(0x97);
        vTaskDelay(pdMS_TO_TICKS(200));
        sendBmsCommand(0x96);
        lastHeartbeatTime = millis();
        data.addLog("Stream initiated (0x97 + 0x96 sent)");
    }

    void drainRxRingBuffer() {
        if (rxTail == rxHead) return;

        while (rxTail != rxHead) {
            uint8_t b = rxRing[rxTail];
            rxTail = (rxTail + 1) % RX_RING_SIZE;
            frameBuffer.push_back(b);
        }

        if (!frameBuffer.empty()) {
            char hexBuf[50] = {0};
            size_t n = std::min((size_t)12, frameBuffer.size());
            for (size_t i = 0; i < n; i++) {
                snprintf(hexBuf + i * 3, 4, "%02X ", frameBuffer[i]);
            }
            data.last_rx_hex = String(hexBuf);
        }

        // Prevent buffer bloat
        if (frameBuffer.size() > 700) {
            frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + 250);
        }

        // Check for JK02 / JK04 preamble: 0x55 0xAA 0xEB 0x90
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
            mgr->allWriteChars.clear();
            mgr->allNotifyChars.clear();
        }
    };
};
