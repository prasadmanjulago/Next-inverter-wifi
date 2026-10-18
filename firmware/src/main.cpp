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

#define FIRMWARE_VERSION "1.0.1"

// Hardware Serial 2 Pins for Inverter RS-232
#define INVERTER_RX_PIN 16
#define INVERTER_TX_PIN 17
#define INVERTER_BAUD   2400

const char *AP_SSID = "NextInverter-WIFI";

// Inverter Protocol & State
InverterTelemetry telemetry;
WebServer server(80);
Preferences preferences;

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
    json += "\"github_repo\":\"" + githubRepo + "\"";
    json += "}";
    server.send(200, "application/json", json);
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

String queryInverter(const char *cmd, unsigned long timeoutMs = 1200) {
    while (Serial2.available()) Serial2.read();

    Serial.printf("[Inverter TX] %s\n", cmd);
    VoltronicProtocol::sendCommand(Serial2, cmd);

    String resp = "";
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (Serial2.available()) {
            char c = Serial2.read();
            resp += c;
            if (c == '\r') break;
        }
    }
    resp.trim();
    return resp;
}

void pollInverter() {
    String qpigsResp = queryInverter("QPIGS");
    if (qpigsResp.length() > 0 && qpigsResp.startsWith("(")) {
        Serial.printf("[Inverter RX QPIGS] %s\n", qpigsResp.c_str());
        VoltronicProtocol::parseQPIGS(qpigsResp, telemetry);
    } else {
        Serial.println("[Inverter RX] Waiting for RS232 data (QPIGS)...");
        if (millis() - telemetry.last_update > 8000) {
            telemetry.valid = false;
        }
    }

    String qmodResp = queryInverter("QMOD");
    if (qmodResp.length() > 0 && qmodResp.startsWith("(")) {
        VoltronicProtocol::parseQMOD(qmodResp, telemetry);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n==========================================");
    Serial.println("  NEXT 6.2kW Inverter Wi-Fi Telemetry Card");
    Serial.printf("  Firmware: v%s (OTA Enabled)\n", FIRMWARE_VERSION);
    Serial.println("==========================================");

    // Initialize Inverter Serial2 (RX2=GPIO16, TX2=GPIO17)
    Serial2.begin(INVERTER_BAUD, SERIAL_8N1, INVERTER_RX_PIN, INVERTER_TX_PIN);
    Serial.printf("[UART2] Inverter Port Initialized (RX=GPIO%d, TX=GPIO%d, Baud=%d)\n", 
                  INVERTER_RX_PIN, INVERTER_TX_PIN, INVERTER_BAUD);

    // Read stored Wi-Fi credentials
    preferences.begin("wifi", true);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("password", "");
    preferences.end();

    // Read stored GitHub repository
    preferences.begin("ota", true);
    githubRepo = preferences.getString("repo", "prasadmanjulago/Next-inverter-wifi");
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

    // Periodic OTA check every 1 hour
    if (millis() - lastOtaCheckTime >= OTA_CHECK_INTERVAL) {
        lastOtaCheckTime = millis();
        checkGitHubUpdate();
    }
}
