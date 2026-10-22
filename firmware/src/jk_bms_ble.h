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
// - Alternates JK02 and Modbus (0x4E 0x57) queries every 4s while waiting for data
// - Subscribes across 0xFFE0 & 0xFF10 with lock-free ring buffer
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
        NimBLEDevice::setPower(ESP_PWR_LVL_P3); // +3dBm prevents 3.3V power dips/brownouts while easily covering inverter-to-battery range
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
            if (isOtaUpdating) {
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            if (data.enabled && data.device_mac.length() >= 12 && !isScanning) {
                stepBle();
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    void saveConfig(bool enabled, const String& mac, const String& name, const String& pin) {
        data.enabled = enabled;
        if (mac.length() >= 12) data.device_mac = mac;
        if (name.length() > 0) data.device_name = name;
        if (pin.length() > 0) data.pin = pin;

        Preferences prefs;
        prefs.begin("jkbms", false);
        prefs.putBool("enabled", enabled);
        if (data.device_mac.length() >= 12) prefs.putString("mac", data.device_mac);
        if (data.device_name.length() > 0) prefs.putString("name", data.device_name);
        if (data.pin.length() > 0) prefs.putString("pin", data.pin);
        prefs.end();

        Serial.printf("[JK-BMS] Config saved: Enabled=%s, MAC=%s, Name=%s\n",
                      enabled ? "YES" : "NO", data.device_mac.c_str(), data.device_name.c_str());

        if (!enabled && pClient != nullptr && pClient->isConnected()) {
            pClient->disconnect();
            data.connected = false;
            data.data_valid = false;
            data.last_status = "Disabled";
            pWriteChar = nullptr;
            pNotifyChar = nullptr;
        }
    }

    void startScan(int durationSec = 4) {
        if (isScanning) return;
        isScanning = true;
        scanResults.clear();

        Serial.println("[JK-BMS] Starting BLE Scan for nearby BMS devices...");
        NimBLEScan* pScan = NimBLEDevice::getScan();
        pScan->setActiveScan(true);
        pScan->setInterval(160);
        pScan->setWindow(40); // Balanced duty cycle prevents starving 2.4GHz Wi-Fi radio

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

    // Ultra-fast lock-free ring buffer
    void onNotifyRaw(uint16_t hdl, const uint8_t* pData, size_t length) {
        if (pData == nullptr || length == 0) return;
        data.rx_packet_count++;
        data.rx_byte_count += length;
        data.notify_handle = hdl;
        data.last_update = millis();

        for (size_t i = 0; i < length; i++) {
            size_t nextHead = (rxHead + 1) % RX_RING_SIZE;
            if (nextHead != rxTail) {
                rxRing[rxHead] = pData[i];
                rxHead = nextHead;
            }
        }
    }

    bool sendBmsCommand(uint8_t cmdType) {
        if (pWriteChar == nullptr) return false;
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
        bool ok = pWriteChar->writeValue(frame, sizeof(frame), false);

        char buf[36];
        snprintf(buf, sizeof(buf), "TX 0x%02X: %s", cmdType, ok ? "OK" : "FAIL");
        data.addLog(String(buf));
        return ok;
    }

private:
    TaskHandle_t taskHandle = nullptr;
    NimBLEClient* pClient = nullptr;
    NimBLERemoteCharacteristic* pWriteChar = nullptr;
    NimBLERemoteCharacteristic* pNotifyChar = nullptr;
    unsigned long lastConnectAttempt = 0;
    unsigned long lastHeartbeatTime = 0;
    unsigned long connectTime = 0;
    uint32_t queryCount = 0;

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
        // Only trigger if no packets received for > 35s to reconnect cleanly
        if (data.connected && !data.data_valid && (now - connectTime > 35000)) {
            data.addLog("Watchdog: No data stream in 35s. Reconnecting...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.last_status = "Connection Timeout (No Packets)";
            pWriteChar = nullptr;
            pNotifyChar = nullptr;
            return;
        }

        // ZERO BEEPING POLICY:
        // Do NOT send periodic query commands! The JK-BMS hardware buzzer beeps on
        // every single received command packet. Once 0x96 is transmitted on connection,
        // the BMS auto-streams Frame 0x02 continuously over BLE notifications.
        // Leaving the BLE transmitter silent ensures 100% silent, buzzer-free operation.
    }

    void connectToBms() {
        data.last_status = "Connecting...";
        data.addLog("Connecting to " + data.device_name + " (" + data.device_mac + ")");

        if (pClient == nullptr) {
            pClient = NimBLEDevice::createClient();
            pClient->setClientCallbacks(new ClientCallbacks(this), false);
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
        pWriteChar = nullptr;
        pNotifyChar = nullptr;

        rxHead = 0;
        rxTail = 0;
        frameBuffer.clear();
        data.rx_packet_count = 0;
        data.rx_byte_count = 0;
        data.tx_cmd_count = 0;
        data.last_rx_hex = "None";
        queryCount = 0;

        // Connect exclusively to Service 0xFFE0 and Characteristic 0xFFE1 (Standard JK-BMS BLE protocol)
        // Never touch 0xFF10 or other vendor test services to avoid beeping and protocol errors.
        NimBLERemoteService* pSvcE0 = pClient->getService(NimBLEUUID((uint16_t)0xFFE0));
        if (pSvcE0 == nullptr) {
            pSvcE0 = pClient->getService(NimBLEUUID("0000ffe0-0000-1000-8000-00805f9b34fb"));
        }

        if (pSvcE0 == nullptr) {
            data.last_status = "Service 0xFFE0 Not Found";
            data.addLog("Error: Service 0xFFE0 not found");
            pClient->disconnect();
            return;
        }

        NimBLERemoteCharacteristic* pCharE1 = pSvcE0->getCharacteristic(NimBLEUUID((uint16_t)0xFFE1));
        if (pCharE1 == nullptr) {
            pCharE1 = pSvcE0->getCharacteristic(NimBLEUUID("0000ffe1-0000-1000-8000-00805f9b34fb"));
        }

        if (pCharE1 == nullptr) {
            data.last_status = "Char 0xFFE1 Not Found";
            data.addLog("Error: Characteristic 0xFFE1 not found");
            pClient->disconnect();
            return;
        }

        pWriteChar = pCharE1;
        pNotifyChar = pCharE1;
        data.write_handle = pCharE1->getHandle();
        data.notify_handle = pCharE1->getHandle();

        // Subscribe to notifications on 0xFFE1 with write-response = true (required by BLE spec for CCCD 0x2902)
        bool subOk = pCharE1->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            this->onNotifyRaw(pChar->getHandle(), pData, length);
        }, true);

        // Fallback to no-response if peripheral rejected write-with-response
        if (!subOk) {
            subOk = pCharE1->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->onNotifyRaw(pChar->getHandle(), pData, length);
            }, false);
        }

        // If notify fails, try indications
        if (!subOk && pCharE1->canIndicate()) {
            subOk = pCharE1->subscribe(false, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->onNotifyRaw(pChar->getHandle(), pData, length);
            }, true);
        }

        data.addLog("Sub 0xFFE1 (0x" + String(data.notify_handle, HEX) + "): " + (subOk ? "OK" : "FAIL"));
        if (!subOk) {
            data.last_status = "Notify Sub Failed";
            pClient->disconnect();
            return;
        }

        data.connected = true;
        data.last_status = "Connected (Waiting for data)";
        lastConnectAttempt = millis();
        lastHeartbeatTime = millis();

        // Initial handshake: Send 0x97 then 0x96 ONCE on connect to start auto-streaming Frame 0x02
        // Absolutely zero periodic queries sent while streaming, ensuring 100% silent operation (no beeping).
        vTaskDelay(pdMS_TO_TICKS(150));
        sendBmsCommand(0x97);
        vTaskDelay(pdMS_TO_TICKS(200));
        sendBmsCommand(0x96);
        data.addLog("Stream initiated (0x97 + 0x96 sent once)");
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
        if (frameBuffer.size() > 800) {
            frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + 300);
        }

        // Check for JK02 / JK04 preamble: 0x55 0xAA 0xEB 0x90 or 0xAA 0x55 0x90 0xEB
        for (size_t i = 0; i + 4 <= frameBuffer.size(); i++) {
            if ((frameBuffer[i] == 0x55 && frameBuffer[i+1] == 0xAA &&
                 frameBuffer[i+2] == 0xEB && frameBuffer[i+3] == 0x90) ||
                (frameBuffer[i] == 0xAA && frameBuffer[i+1] == 0x55 &&
                 frameBuffer[i+2] == 0x90 && frameBuffer[i+3] == 0xEB)) {

                if (i > 0) {
                    frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + i);
                }

                // In JK04 (Record Type 0x01) or shorter packs, frame can be 140-300 bytes
                // In JK02 (Record Type 0x02), frame is 300 bytes
                if (frameBuffer.size() >= 5) {
                    uint8_t fType = frameBuffer[4];
                    size_t requiredLen = (fType == 0x01) ? 140 : 300;
                    if (frameBuffer.size() >= requiredLen) {
                        decodeFrame(frameBuffer.data(), frameBuffer.size());
                        frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + requiredLen);
                    }
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

        // Format 1: JK02 Cell Info Frame (Record Type 0x02)
        if (((buf[0] == 0x55 && buf[1] == 0xAA) || (buf[0] == 0xAA && buf[1] == 0x55)) && buf[4] == 0x02) {
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

        // Format 1b: Frame 0x03 (Device Info)
        if (((buf[0] == 0x55 && buf[1] == 0xAA) || (buf[0] == 0xAA && buf[1] == 0x55)) && buf[4] == 0x03) {
            data.addLog("Received Frame 0x03 (Device Info)");
            return;
        }

        // Format 2: JK04 / Tag-Length-Value (Record Type 0x01)
        if (((buf[0] == 0x55 && buf[1] == 0xAA) || (buf[0] == 0xAA && buf[1] == 0x55)) && buf[4] == 0x01) {
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
            mgr->pWriteChar = nullptr;
            mgr->pNotifyChar = nullptr;
        }
    };
};
