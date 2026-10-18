#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include "inverter_protocol.h"
#include "web_dashboard.h"

#define FIRMWARE_VERSION "1.0.3"

// Hardware Serial 2 Pins for Inverter RS-232
#define INVERTER_RX_PIN 16
#define INVERTER_TX_PIN 17
#define INVERTER_BAUD   2400

int currentBaud = 2400;
int currentRxPin = 16;
int currentTxPin = 17;
bool isPinsSwapped = false;
bool autoBaudEnabled = true;

const char *AP_SSID = "NextInverter-WIFI";

// Inverter Protocol & State
InverterTelemetry telemetry;
InverterAccumulator tbAccumulator;
WebServer server(80);
Preferences preferences;

// ThingsBoard Cloud Telemetry State
bool tbEnabled = false;
String tbServer = "https://thingsboard.cloud";
String tbToken = "";
int tbInterval = 10; // Default 10 seconds (~3 samples averaged)
String tbLastStatus = "Ready";
unsigned long lastTbSendTime = 0;

void initSerial2(int baud, int rxPin, int txPin) {
    Serial2.end();
    delay(30);
    Serial2.begin(baud, SERIAL_8N1, rxPin, txPin);
    currentBaud = baud;
    currentRxPin = rxPin;
    currentTxPin = txPin;
    telemetry.current_baud = baud;
    telemetry.rx_pin = rxPin;
    telemetry.tx_pin = txPin;
    telemetry.pins_swapped = isPinsSwapped;
    Serial.printf("[UART2] Configured: %d baud, RX=GPIO %d, TX=GPIO %d (Swapped: %s)\n",
                  baud, rxPin, txPin, isPinsSwapped ? "YES" : "NO");
}

unsigned long lastPollTime = 0;
const unsigned long POLL_INTERVAL = 3000; // Poll inverter every 3 seconds
bool isApModeActive = false;

// OTA & GitHub State
String githubRepo = "prasadmanjulago/Next-inverter-wifi";
String latestVersion = "";
String latestBinUrl = "";
String updateNotes = "";
bool updateAvailable = false;
unsigned long lastOtaCheckTime = 0;
const unsigned long OTA_CHECK_INTERVAL = 3600000; // Check every 1 hour

void startFallbackAP() {
    isApModeActive = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID);
    Serial.printf("\n[Wi-Fi] Fallback AP active: '%s'\n", AP_SSID);
    Serial.printf("[Wi-Fi AP] IP Address: http://%s\n", WiFi.softAPIP().toString().c_str());
}

void checkGitHubUpdate() {
    if (WiFi.status() != WL_CONNECTED || githubRepo.length() == 0) return;

    Serial.printf("[OTA] Checking GitHub: %s\n", githubRepo.c_str());
    WiFiClientSecure client;
    client.setInsecure(); // GitHub uses HTTPS; bypass SSL cert expiration check

    HTTPClient https;
    String url = "https://raw.githubusercontent.com/" + githubRepo + "/main/version.json";
    
    if (https.begin(client, url)) {
        https.setTimeout(7000);
        int httpCode = https.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = https.getString();
            Serial.printf("[OTA] version.json received: %s\n", payload.c_str());

            int vIdx = payload.indexOf("\"version\":");
            int bIdx = payload.indexOf("\"bin_url\":");
            int nIdx = payload.indexOf("\"notes\":");

            if (vIdx >= 0) {
                int q1 = payload.indexOf('"', vIdx + 10);
                int q2 = payload.indexOf('"', q1 + 1);
                if (q1 >= 0 && q2 > q1) latestVersion = payload.substring(q1 + 1, q2);
            }
            if (bIdx >= 0) {
                int q1 = payload.indexOf('"', bIdx + 10);
                int q2 = payload.indexOf('"', q1 + 1);
                if (q1 >= 0 && q2 > q1) latestBinUrl = payload.substring(q1 + 1, q2);
            }
            if (nIdx >= 0) {
                int q1 = payload.indexOf('"', nIdx + 8);
                int q2 = payload.indexOf('"', q1 + 1);
                if (q1 >= 0 && q2 > q1) updateNotes = payload.substring(q1 + 1, q2);
            }

            if (latestVersion.length() > 0 && latestVersion != FIRMWARE_VERSION) {
                updateAvailable = true;
                Serial.printf("[OTA] Update Available! v%s (Current: v%s)\n", latestVersion.c_str(), FIRMWARE_VERSION);
            } else {
                updateAvailable = false;
                Serial.println("[OTA] Firmware is up to date.");
            }
        } else {
            Serial.printf("[OTA] HTTP check failed, code: %d\n", httpCode);
        }
        https.end();
    }
}

void handleRoot() {
    server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleWifiPage() {
    server.send_P(200, "text/html", WIFI_HTML);
}

void handleWifiScan() {
    Serial.println("[Wi-Fi] Scanning nearby networks...");
    int n = WiFi.scanNetworks(false, true);
    String json = "[";
    for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;
        if (json.length() > 1) json += ",";
        json += "{";
        json += "\"ssid\":\"" + ssid + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
        json += "}";
    }
    json += "]";
    WiFi.scanDelete();
    server.send(200, "application/json", json);
}

void handleWifiSave() {
    if (server.hasArg("ssid")) {
        String ssid = server.arg("ssid");
        String pass = server.arg("password");
        
        Serial.printf("[Wi-Fi] Saving credentials: SSID=%s\n", ssid.c_str());
        preferences.begin("wifi", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", pass);
        preferences.end();

        server.send(200, "text/plain", "Credentials saved. ESP32 is connecting to " + ssid + "...");
        delay(1500);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Missing SSID");
    }
}

void handleSaveGithubRepo() {
    if (server.hasArg("repo")) {
        githubRepo = server.arg("repo");
        githubRepo.trim();
        preferences.begin("ota", false);
        preferences.putString("repo", githubRepo);
        preferences.end();
        Serial.printf("[OTA] Saved GitHub Repo: %s\n", githubRepo.c_str());

        checkGitHubUpdate();
        server.send(200, "text/plain", "GitHub Repo saved! Checking for updates...");
    } else {
        server.send(400, "text/plain", "Missing repo parameter");
    }
}

void handleCheckUpdate() {
    checkGitHubUpdate();
    String json = "{";
    json += "\"current_version\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"latest_version\":\"" + latestVersion + "\",";
    json += "\"update_available\":" + String(updateAvailable ? "true" : "false") + ",";
    json += "\"notes\":\"" + updateNotes + "\",";
    json += "\"github_repo\":\"" + githubRepo + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void handleDoGithubUpdate() {
    if (latestBinUrl.length() == 0) {
        server.send(400, "text/plain", "No firmware binary URL configured.");
        return;
    }

    server.send(200, "text/plain", "Starting wireless update from GitHub... ESP32 will reboot in ~20 seconds.");
    delay(800);

    Serial.printf("[OTA] Updating wirelessly from: %s\n", latestBinUrl.c_str());
    WiFiClientSecure client;
    client.setInsecure();

    httpUpdate.rebootOnUpdate(true);
    t_httpUpdate_return ret = httpUpdate.update(client, latestBinUrl);

    if (ret == HTTP_UPDATE_FAILED) {
        Serial.printf("[OTA] Failed! Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    }
}

void handleTelemetryApi() {
    bool wifiOk = (WiFi.status() == WL_CONNECTED);

    String json = "{";
    json += "\"valid\":" + String(telemetry.valid ? "true" : "false") + ",";
    json += "\"mode\":\"" + telemetry.mode_string + "\",";
    json += "\"grid_voltage\":" + String(telemetry.grid_voltage, 1) + ",";
    json += "\"grid_frequency\":" + String(telemetry.grid_frequency, 1) + ",";
    json += "\"ac_output_voltage\":" + String(telemetry.ac_output_voltage, 1) + ",";
    json += "\"ac_output_frequency\":" + String(telemetry.ac_output_frequency, 1) + ",";
    json += "\"ac_output_va\":" + String(telemetry.ac_output_va) + ",";
    json += "\"ac_output_watts\":" + String(telemetry.ac_output_watts) + ",";
    json += "\"load_percent\":" + String(telemetry.load_percent) + ",";
    json += "\"battery_voltage\":" + String(telemetry.battery_voltage, 1) + ",";
    json += "\"battery_charge_current\":" + String(telemetry.battery_charge_current) + ",";
    json += "\"battery_discharge_current\":" + String(telemetry.battery_discharge_current) + ",";
    json += "\"battery_power_watts\":" + String(telemetry.battery_power_watts, 1) + ",";
    json += "\"battery_soc\":" + String(telemetry.battery_soc) + ",";
    json += "\"heatsink_temp\":" + String(telemetry.heatsink_temp, 1) + ",";
    json += "\"pv_voltage\":" + String(telemetry.pv_voltage, 1) + ",";
    json += "\"pv_current\":" + String(telemetry.pv_current, 1) + ",";
    json += "\"pv_watts\":" + String(telemetry.pv_watts, 1) + ",";
    json += "\"solar_charging\":" + String(telemetry.solar_charging ? "true" : "false") + ",";
    json += "\"grid_charging\":" + String(telemetry.grid_charging ? "true" : "false") + ",";
    json += "\"load_on_inverter\":" + String(telemetry.load_on_inverter ? "true" : "false") + ",";
    
    // Wi-Fi Status
    json += "\"wifi_connected\":" + String(wifiOk ? "true" : "false") + ",";
    json += "\"wifi_ssid\":\"" + (wifiOk ? WiFi.SSID() : "") + "\",";
    json += "\"wifi_ip\":\"" + (wifiOk ? WiFi.localIP().toString() : "") + "\",";
    json += "\"wifi_rssi\":" + String(wifiOk ? WiFi.RSSI() : 0) + ",";
    json += "\"ap_mode\":" + String(isApModeActive ? "true" : "false") + ",";
    json += "\"ap_ssid\":\"" + String(AP_SSID) + "\",";
    json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";

    // OTA Status
    json += "\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"update_available\":" + String(updateAvailable ? "true" : "false") + ",";
    json += "\"latest_version\":\"" + latestVersion + "\",";
    json += "\"github_repo\":\"" + githubRepo + "\",";

    // ThingsBoard Status
    json += "\"tb_enabled\":" + String(tbEnabled ? "true" : "false") + ",";
    json += "\"tb_interval\":" + String(tbInterval) + ",";
    json += "\"tb_status\":\"" + tbLastStatus + "\",";

    // Serial Diagnostics
    json += "\"last_tx_cmd\":\"" + telemetry.last_tx_cmd + "\",";
    String escapedRaw = telemetry.last_rx_raw;
    escapedRaw.replace("\\", "\\\\");
    escapedRaw.replace("\"", "\\\"");
    escapedRaw.replace("\r", "");
    escapedRaw.replace("\n", "");
    json += "\"last_rx_raw\":\"" + escapedRaw + "\",";
    json += "\"last_rx_hex\":\"" + telemetry.last_rx_hex + "\",";
    json += "\"rx_byte_count\":" + String(telemetry.rx_byte_count) + ",";
    json += "\"tx_byte_count\":" + String(telemetry.tx_byte_count) + ",";
    json += "\"current_baud\":" + String(telemetry.current_baud) + ",";
    json += "\"rx_pin\":" + String(telemetry.rx_pin) + ",";
    json += "\"tx_pin\":" + String(telemetry.tx_pin) + ",";
    json += "\"pins_swapped\":" + String(telemetry.pins_swapped ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
}

String queryInverter(const char *cmd, unsigned long timeoutMs = 800);

void handleSetBaud() {
    if (server.hasArg("baud")) {
        int b = server.arg("baud").toInt();
        if (b == 2400 || b == 9600 || b == 19200 || b == 115200) {
            autoBaudEnabled = false;
            initSerial2(b, currentRxPin, currentTxPin);
            server.send(200, "text/plain", "Baud set to " + String(b));
            return;
        }
    }
    server.send(400, "text/plain", "Invalid baud rate");
}

void handleSwapPins() {
    isPinsSwapped = !isPinsSwapped;
    int newRx = isPinsSwapped ? 17 : 16;
    int newTx = isPinsSwapped ? 16 : 17;
    initSerial2(currentBaud, newRx, newTx);
    server.send(200, "text/plain", isPinsSwapped ? "Swapped (RX:17, TX:16)" : "Standard (RX:16, TX:17)");
}

void handleSendCmd() {
    if (!server.hasArg("cmd")) {
        server.send(400, "text/plain", "Missing cmd parameter");
        return;
    }
    String cmd = server.arg("cmd");
    cmd.trim();
    String resp = queryInverter(cmd.c_str(), 1000);
    server.send(200, "text/plain", resp);
}

void handleSetPriority() {
    if (!server.hasArg("val")) {
        server.send(400, "text/plain", "Missing val parameter");
        return;
    }
    String val = server.arg("val");
    const char *cmd = "";
    if (val == "USB") cmd = "POP00";
    else if (val == "SUB") cmd = "POP01";
    else if (val == "SBU") cmd = "POP02";
    else {
        server.send(400, "text/plain", "Invalid priority. Use USB, SUB, or SBU.");
        return;
    }

    Serial.printf("[Inverter] Output Priority Command: %s\n", cmd);
    VoltronicProtocol::sendCommand(Serial2, cmd);
    server.send(200, "text/plain", "Sent " + String(cmd) + " to Inverter!");
}

String queryInverter(const char *cmd, unsigned long timeoutMs) {
    while (Serial2.available()) Serial2.read();

    telemetry.last_tx_cmd = String(cmd);
    VoltronicProtocol::sendCommand(Serial2, cmd);
    telemetry.tx_byte_count += strlen(cmd) + 3;

    String resp = "";
    String hexDump = "";
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        server.handleClient();
        if (Serial2.available()) {
            char c = (char)Serial2.read();
            telemetry.rx_byte_count++;

            char hBuf[6];
            snprintf(hBuf, sizeof(hBuf), "%02X ", (uint8_t)c);
            hexDump += hBuf;

            if (c >= 32 && c <= 126) {
                resp += c;
            } else if (c == '\r') {
                resp += "\\r";
                break;
            } else if (c == '\n') {
                resp += "\\n";
            } else {
                resp += '.';
            }
        }
        delay(2);
    }
    hexDump.trim();
    telemetry.last_rx_raw = (resp.length() > 0) ? resp : "(No response / Timeout)";
    telemetry.last_rx_hex = (hexDump.length() > 0) ? hexDump : "None";
    return resp;
}

void pollInverter() {
    // If not connected, test alternating baud rates every 4 cycles (~12s)
    if (!telemetry.valid && autoBaudEnabled) {
        static int autoBaudCycle = 0;
        autoBaudCycle++;
        if (autoBaudCycle >= 4) {
            autoBaudCycle = 0;
            int nextBaud = (currentBaud == 2400) ? 9600 : 2400;
            initSerial2(nextBaud, currentRxPin, currentTxPin);
        }
    }

    String qpigsResp = queryInverter("QPIGS");
    if (qpigsResp.length() > 0 && qpigsResp.indexOf('(') >= 0) {
        Serial.printf("[Inverter RX QPIGS] %s\n", qpigsResp.c_str());
        VoltronicProtocol::parseQPIGS(qpigsResp, telemetry);
        tbAccumulator.add(telemetry); // Accumulate sample for ThingsBoard interval averaging
        autoBaudEnabled = false; // Lock in working baud rate!
    } else {
        if (millis() - telemetry.last_update > 8000) {
            telemetry.valid = false;
        }
    }

    if (telemetry.valid) {
        String qmodResp = queryInverter("QMOD");
        if (qmodResp.length() > 0 && qmodResp.indexOf('(') >= 0) {
            VoltronicProtocol::parseQMOD(qmodResp, telemetry);
        }
    }
}

void sendThingsBoardTelemetry() {
    if (!tbEnabled || tbToken.length() == 0 || WiFi.status() != WL_CONNECTED) return;
    if (tbAccumulator.count == 0) return;

    int n = tbAccumulator.count;
    float avgGridV = tbAccumulator.sum_grid_voltage / n;
    float avgGridHz = tbAccumulator.sum_grid_frequency / n;
    float avgAcOutV = tbAccumulator.sum_ac_output_voltage / n;
    float avgAcOutHz = tbAccumulator.sum_ac_output_frequency / n;
    int avgAcOutVA = round(tbAccumulator.sum_ac_output_va / n);
    int avgAcOutW = round(tbAccumulator.sum_ac_output_watts / n);
    int avgLoadPct = round(tbAccumulator.sum_load_percent / n);
    float avgBatV = tbAccumulator.sum_battery_voltage / n;
    int avgBatChgA = round(tbAccumulator.sum_battery_charge_current / n);
    int avgBatDisA = round(tbAccumulator.sum_battery_discharge_current / n);
    int avgBatSoc = round(tbAccumulator.sum_battery_soc / n);
    float avgTemp = tbAccumulator.sum_heatsink_temp / n;
    float avgPvV = tbAccumulator.sum_pv_voltage / n;
    float avgPvA = tbAccumulator.sum_pv_current / n;
    float avgPvW = tbAccumulator.sum_pv_watts / n;

    // Direct Power Channels for ThingsBoard Line Charts
    // Battery Input: Power contributed by battery to the load (Positive Watts when discharging)
    float avgBatInputW = (avgBatDisA > 0) ? ((float)avgBatDisA * avgBatV) : 0.0;
    float avgBatChgW = (avgBatChgA > 0) ? ((float)avgBatChgA * avgBatV) : 0.0;
    float avgBatDisW = avgBatInputW;

    // Positive (+) when powering load, negative (-) when charging
    float avgBatPwrW = (avgBatDisA > 0) ? ((float)avgBatDisA * avgBatV) : ((avgBatChgA > 0) ? (-(float)avgBatChgA * avgBatV) : 0.0);

    float avgGridW = 0.0;
    if (telemetry.mode_char == 'L' || telemetry.grid_charging) {
        float demand = (float)avgAcOutW + avgBatChgW;
        avgGridW = (demand > avgPvW) ? (demand - avgPvW) : 0.0;
    }

    // Reset accumulator for next interval
    tbAccumulator.reset();

    String payload = "{";
    // Explicit Chart Keys
    payload += "\"load_watts\":" + String(avgAcOutW) + ",";
    payload += "\"solar_watts\":" + String(avgPvW, 1) + ",";
    payload += "\"grid_watts\":" + String(avgGridW, 1) + ",";
    payload += "\"battery_power_watts\":" + String(avgBatPwrW, 1) + ",";
    payload += "\"battery_input_watts\":" + String(avgBatInputW, 1) + ",";
    payload += "\"battery_charge_watts\":" + String(avgBatChgW, 1) + ",";
    payload += "\"battery_discharge_watts\":" + String(avgBatDisW, 1) + ",";

    // Standard Telemetry Keys
    payload += "\"grid_voltage\":" + String(avgGridV, 1) + ",";
    payload += "\"grid_frequency\":" + String(avgGridHz, 1) + ",";
    payload += "\"ac_output_voltage\":" + String(avgAcOutV, 1) + ",";
    payload += "\"ac_output_frequency\":" + String(avgAcOutHz, 1) + ",";
    payload += "\"ac_output_va\":" + String(avgAcOutVA) + ",";
    payload += "\"ac_output_watts\":" + String(avgAcOutW) + ",";
    payload += "\"load_percent\":" + String(avgLoadPct) + ",";
    payload += "\"battery_voltage\":" + String(avgBatV, 1) + ",";
    payload += "\"battery_soc\":" + String(avgBatSoc) + ",";
    payload += "\"battery_charge_current\":" + String(avgBatChgA) + ",";
    payload += "\"battery_discharge_current\":" + String(avgBatDisA) + ",";
    payload += "\"pv_voltage\":" + String(avgPvV, 1) + ",";
    payload += "\"pv_current\":" + String(avgPvA, 1) + ",";
    payload += "\"pv_watts\":" + String(avgPvW, 1) + ",";
    payload += "\"heatsink_temp\":" + String(avgTemp, 1) + ",";
    payload += "\"mode\":\"" + telemetry.mode_string + "\",";
    payload += "\"solar_charging\":" + String(telemetry.solar_charging ? "true" : "false") + ",";
    payload += "\"grid_charging\":" + String(telemetry.grid_charging ? "true" : "false") + ",";
    payload += "\"samples_averaged\":" + String(n);
    payload += "}";

    String url = tbServer;
    url.trim();
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        url = "https://" + url;
    }
    if (!url.endsWith("/")) url += "/";
    url += "api/v1/" + tbToken + "/telemetry";

    HTTPClient http;
    bool beginOk = false;
    WiFiClientSecure secClient;
    WiFiClient plainClient;

    if (url.startsWith("https://")) {
        secClient.setInsecure();
        beginOk = http.begin(secClient, url);
    } else {
        beginOk = http.begin(plainClient, url);
    }

    if (beginOk) {
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(4000);
        int httpCode = http.POST(payload);
        if (httpCode == HTTP_CODE_OK || httpCode == 200) {
            tbLastStatus = "Sent OK (" + String(n) + " avg)";
            Serial.printf("[ThingsBoard] Sent averaged telemetry (%d samples) -> HTTP 200 OK\n", n);
        } else {
            tbLastStatus = "HTTP " + String(httpCode);
            Serial.printf("[ThingsBoard] HTTP Response: %d\n", httpCode);
        }
        http.end();
    } else {
        tbLastStatus = "Conn Failed";
        Serial.println("[ThingsBoard] Failed to begin HTTP connection");
    }
}

void handleTbConfigGet() {
    String json = "{";
    json += "\"enabled\":" + String(tbEnabled ? "true" : "false") + ",";
    json += "\"server\":\"" + tbServer + "\",";
    json += "\"token\":\"" + tbToken + "\",";
    json += "\"interval\":" + String(tbInterval) + ",";
    json += "\"last_status\":\"" + tbLastStatus + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void handleTbConfigPost() {
    if (server.hasArg("enabled")) tbEnabled = (server.arg("enabled") == "true" || server.arg("enabled") == "1");
    if (server.hasArg("server")) {
        tbServer = server.arg("server");
        tbServer.trim();
        if (tbServer.length() == 0) tbServer = "https://thingsboard.cloud";
    }
    if (server.hasArg("token")) {
        tbToken = server.arg("token");
        tbToken.trim();
    }
    if (server.hasArg("interval")) {
        tbInterval = server.arg("interval").toInt();
        if (tbInterval < 5) tbInterval = 10;
    }

    preferences.begin("tb", false);
    preferences.putBool("enabled", tbEnabled);
    preferences.putString("server", tbServer);
    preferences.putString("token", tbToken);
    preferences.putInt("interval", tbInterval);
    preferences.end();

    tbAccumulator.reset();
    server.send(200, "text/plain", "ThingsBoard settings saved!");
    Serial.printf("[ThingsBoard] Saved: Enabled=%s, Server=%s, Interval=%ds\n",
                  tbEnabled ? "YES" : "NO", tbServer.c_str(), tbInterval);
}


void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n==========================================");
    Serial.println("  NEXT 6.2kW Inverter Wi-Fi Telemetry Card");
    Serial.printf("  Firmware: v%s (OTA Enabled)\n", FIRMWARE_VERSION);
    Serial.println("==========================================");

    // Initialize Inverter Serial2
    initSerial2(INVERTER_BAUD, INVERTER_RX_PIN, INVERTER_TX_PIN);

    // Read stored Wi-Fi credentials
    preferences.begin("wifi", true);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("password", "");
    preferences.end();

    // Read stored GitHub repository
    preferences.begin("ota", true);
    githubRepo = preferences.getString("repo", "prasadmanjulago/Next-inverter-wifi");
    preferences.end();

    // Read stored ThingsBoard preferences
    preferences.begin("tb", true);
    tbEnabled = preferences.getBool("enabled", false);
    tbServer = preferences.getString("server", "https://thingsboard.cloud");
    tbToken = preferences.getString("token", "");
    tbInterval = preferences.getInt("interval", 10);
    if (tbInterval < 5) tbInterval = 10;
    preferences.end();

    bool connected = false;
    if (ssid.length() > 0) {
        Serial.printf("[Wi-Fi] Connecting to '%s'...\n", ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 9000) {
            delay(300);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            isApModeActive = false;
            Serial.println("\n[Wi-Fi] Connected to network successfully!");
            Serial.printf("[Wi-Fi] IP Address: http://%s\n", WiFi.localIP().toString().c_str());
            
            // Check for updates on startup
            checkGitHubUpdate();
        } else {
            Serial.println("\n[Wi-Fi] Network not found or failed to connect.");
        }
    }

    // Fallback to AP if not connected
    if (!connected) {
        startFallbackAP();
    }

    // mDNS responder (http://next-inverter.local)
    if (MDNS.begin("next-inverter")) {
        Serial.println("[mDNS] http://next-inverter.local responder started");
    }

    // Configure Web Server
    server.on("/", handleRoot);
    server.on("/wifi", handleWifiPage);
    server.on("/wifi_save", HTTP_POST, handleWifiSave);
    server.on("/api/telemetry", handleTelemetryApi);
    server.on("/api/set_priority", handleSetPriority);
    server.on("/api/wifi_scan", handleWifiScan);
    server.on("/api/check_update", handleCheckUpdate);
    server.on("/api/do_github_update", HTTP_POST, handleDoGithubUpdate);
    server.on("/api/save_github_repo", HTTP_POST, handleSaveGithubRepo);
    server.on("/api/set_baud", HTTP_POST, handleSetBaud);
    server.on("/api/swap_pins", HTTP_POST, handleSwapPins);
    server.on("/api/send_cmd", HTTP_POST, handleSendCmd);
    server.on("/api/tb_config", HTTP_GET, handleTbConfigGet);
    server.on("/api/tb_config", HTTP_POST, handleTbConfigPost);

    // Browser Direct Web OTA (/update)
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[Web OTA] Starting Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[Web OTA] Update Success: %u bytes! Rebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();
    Serial.println("[Web Server] Active on port 80 (Web OTA endpoint: /update)");
}

void loop() {
    server.handleClient();

    if (millis() - lastPollTime >= POLL_INTERVAL) {
        lastPollTime = millis();
        pollInverter();
    }

    // ThingsBoard Periodic Cloud Telemetry with Averaged Inverter Data
    if (tbEnabled && millis() - lastTbSendTime >= (unsigned long)tbInterval * 1000) {
        lastTbSendTime = millis();
        sendThingsBoardTelemetry();
    }

    // Periodic OTA check every 1 hour
    if (millis() - lastOtaCheckTime >= OTA_CHECK_INTERVAL) {
        lastOtaCheckTime = millis();
        checkGitHubUpdate();
    }
}
