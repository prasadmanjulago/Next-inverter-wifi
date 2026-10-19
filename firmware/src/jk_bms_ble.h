#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>

// -----------------------------------------------------------------------------
// JK-BMS Bluetooth Low Energy (BLE) Client & Decoder
// Supports both JK02 (32S/24S frame type 0x02) and JK04 (TLV 0x01) protocols
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
        NimBLEDevice::setMTU(247);
        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

        Serial.printf("[JK-BMS] Initialized. Enabled=%s, MAC=%s, Name=%s\n",
                      data.enabled ? "YES" : "NO",
                      data.device_mac.c_str(),
                      data.device_name.c_str());
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

        Serial.printf("[JK-BMS] Config saved: Enabled=%s, MAC=%s, PIN=%s\n",
                      enabled ? "YES" : "NO", mac.c_str(), pin.c_str());

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

            // Prioritize JK devices at the top
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
            if (now - lastConnectAttempt >= 8000) {
                lastConnectAttempt = now;
                connectToBms();
            }
            return;
        }

        // 2. Watchdog: check if telemetry stopped arriving (>15 seconds)
        if (data.connected && data.data_valid && (now - data.last_update > 15000)) {
            Serial.println("[JK-BMS] Telemetry timeout (>15s). Disconnecting to re-establish...");
            if (pClient) pClient->disconnect();
            data.connected = false;
            data.data_valid = false;
            data.last_status = "Connection Lost (Timeout)";
            return;
        }

        // 3. Heartbeat Query: send 0x96 cell info poll every 4 seconds to maintain stream
        if (data.connected && pWriteChar != nullptr && (now - lastQueryTime >= 4000)) {
            lastQueryTime = now;
            sendQueryFrame(0x96);
        }
    }

private:
    NimBLEClient* pClient = nullptr;
    NimBLERemoteCharacteristic* pWriteChar = nullptr;
    NimBLERemoteCharacteristic* pNotifyChar = nullptr;
    unsigned long lastConnectAttempt = 0;
    unsigned long lastQueryTime = 0;
    std::vector<uint8_t> frameBuffer;

    JkBmsManager() {}

    void connectToBms() {
        data.last_status = "Connecting...";
        Serial.printf("[JK-BMS] Connecting to %s (%s)...\n",
                      data.device_name.c_str(), data.device_mac.c_str());

        if (pClient == nullptr) {
            pClient = NimBLEDevice::createClient();
            pClient->setClientCallbacks(new ClientCallbacks(this), false);
            pClient->setConnectionParams(12, 12, 0, 300); // Low latency connection
            pClient->setConnectTimeout(6);
        }

        NimBLEAddress targetAddr(data.device_mac.c_str());
        if (!pClient->connect(targetAddr)) {
            Serial.println("[JK-BMS] Failed to connect.");
            data.last_status = "Connection Failed";
            return;
        }

        data.rssi = pClient->getRssi();
        Serial.printf("[JK-BMS] Connected! Discovering services...\n");

        // Discover Service 0xFFE0
        NimBLERemoteService* pService = pClient->getService("FFE0");
        if (pService == nullptr) {
            Serial.println("[JK-BMS] Service 0xFFE0 not found.");
            pClient->disconnect();
            data.last_status = "Service 0xFFE0 Not Found";
            return;
        }

        // Discover Characteristic 0xFFE1
        pWriteChar = nullptr;
        pNotifyChar = nullptr;

        std::vector<NimBLERemoteCharacteristic*>* pChars = pService->getCharacteristics(true);
        if (pChars != nullptr) {
            for (auto* pCh : *pChars) {
                if (pCh->getUUID().toString() == "ffe1" || pCh->getUUID().toString() == "0000ffe1-0000-1000-8000-00805f9b34fb") {
                    if (pCh->canNotify()) pNotifyChar = pCh;
                    if (pCh->canWrite() || pCh->canWriteNoResponse()) pWriteChar = pCh;
                }
            }
        }

        // Fallback if single characteristic has both properties
        if (pNotifyChar == nullptr) pNotifyChar = pService->getCharacteristic("FFE1");
        if (pWriteChar == nullptr) pWriteChar = pService->getCharacteristic("FFE1");

        if (pNotifyChar == nullptr || !pNotifyChar->canNotify()) {
            Serial.println("[JK-BMS] Characteristic 0xFFE1 (Notify) not found.");
            pClient->disconnect();
            data.last_status = "Notify Char 0xFFE1 Not Found";
            return;
        }

        // Subscribe to notifications
        frameBuffer.clear();
        if (!pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            this->handleNotification(pData, length);
        })) {
            Serial.println("[JK-BMS] Failed to subscribe to notifications.");
            pClient->disconnect();
            data.last_status = "Subscription Failed";
            return;
        }

        data.connected = true;
        data.last_status = "Connected (Streaming)";
        Serial.println("[JK-BMS] Subscribed! Sending initialization query frames (0x97 and 0x96)...");

        // Send handshake frames
        delay(100);
        sendQueryFrame(0x97); // Device Info
        delay(150);
        sendQueryFrame(0x96); // Cell Info
        lastQueryTime = millis();
    }

    void sendQueryFrame(uint8_t cmdType) {
        if (!pWriteChar) return;
        uint8_t frame[20] = {0};
        frame[0] = 0xAA;
        frame[1] = 0x55;
        frame[2] = 0x90;
        frame[3] = 0xEB;
        frame[4] = cmdType;
        frame[5] = 0x00;
        // bytes 6..15 padding = 0x00
        frame[16] = 0x00; // Counter
        frame[17] = 0x00;
        frame[18] = 0x00;

        uint8_t crc = 0;
        for (int i = 0; i < 19; i++) crc += frame[i];
        frame[19] = crc;

        pWriteChar->writeValue(frame, sizeof(frame), false);
    }

    void handleNotification(const uint8_t* pData, size_t length) {
        if (length == 0 || pData == nullptr) return;

        // Check if this chunk is the start of a new frame: 0x55 0xAA 0xEB 0x90
        if (length >= 4 && pData[0] == 0x55 && pData[1] == 0xAA && pData[2] == 0xEB && pData[3] == 0x90) {
            frameBuffer.clear();
        }

        // Append incoming bytes
        frameBuffer.insert(frameBuffer.end(), pData, pData + length);

        // Typical JK02 / JK04 frame is 300 to 320 bytes
        if (frameBuffer.size() >= 300) {
            // Verify header
            if (frameBuffer[0] == 0x55 && frameBuffer[1] == 0xAA && frameBuffer[2] == 0xEB && frameBuffer[3] == 0x90) {
                decodeFrame(frameBuffer.data(), frameBuffer.size());
                frameBuffer.clear();
            } else {
                // Search for start sequence if offset
                for (size_t i = 1; i < frameBuffer.size() - 4; i++) {
                    if (frameBuffer[i] == 0x55 && frameBuffer[i+1] == 0xAA &&
                        frameBuffer[i+2] == 0xEB && frameBuffer[i+3] == 0x90) {
                        frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + i);
                        break;
                    }
                }
            }
        }
    }

    void decodeFrame(const uint8_t* buf, size_t len) {
        uint8_t recordType = buf[4];

        // Format 1: JK02 Cell Info Frame (Record Type 0x02)
        if (recordType == 0x02) {
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

                // Valid lithium cell voltage range (typically 2.0V - 4.5V)
                // For 14S Leaf battery, check first 14 cells or all cells > 1.5V
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

            // Power tube (MOSFET) Temperature: bytes 144-145 (int16_t * 0.1°C)
            if (len >= 146) {
                int16_t rawMos = (int16_t)((uint16_t)buf[144] | ((uint16_t)buf[145] << 8));
                data.temp_mos = rawMos * 0.1f;
            }

            // Battery Temperature Sensor 1: bytes 162-163 (int16_t * 0.1°C)
            if (len >= 164) {
                int16_t rawT1 = (int16_t)((uint16_t)buf[162] | ((uint16_t)buf[163] << 8));
                data.temp_sensor1 = rawT1 * 0.1f;
            }

            // Battery Temperature Sensor 2: bytes 164-165 (int16_t * 0.1°C)
            if (len >= 166) {
                int16_t rawT2 = (int16_t)((uint16_t)buf[164] | ((uint16_t)buf[165] << 8));
                data.temp_sensor2 = rawT2 * 0.1f;
            }

            // Balancing Status: byte 142 (bitmask / status)
            if (len >= 143) {
                data.balancing = (buf[142] > 0);
            }

            data.data_valid = true;
            data.last_update = millis();
            data.last_status = "Connected (Live)";
            return;
        }

        // Format 2: JK04 / Tag-Length-Value (Record Type 0x01)
        if (recordType == 0x01) {
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
                    int nCells = subLen / 5;
                    for (int c = 0; c < nCells && c < 32; c++) {
                        if (idx + 5 > len) break;
                        uint8_t cellNum = buf[idx++];
                        uint16_t mV = ((uint16_t)buf[idx] << 8) | buf[idx + 1]; idx += 2;
                        idx += 2; // skip resistance
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
                    idx++; // skip unknown tag
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
        }
    }

    class ClientCallbacks : public NimBLEClientCallbacks {
        JkBmsManager* mgr;
    public:
        ClientCallbacks(JkBmsManager* m) : mgr(m) {}
        void onConnect(NimBLEClient* pClient) override {
            Serial.println("[JK-BMS] BLE Connected callback.");
        }
        void onDisconnect(NimBLEClient* pClient) override {
            Serial.println("[JK-BMS] BLE Disconnected callback.");
            mgr->data.connected = false;
            mgr->data.data_valid = false;
            mgr->data.last_status = "Disconnected";
        }
    };
};
