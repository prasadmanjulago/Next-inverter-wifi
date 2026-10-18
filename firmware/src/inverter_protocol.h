#pragma once
#include <Arduino.h>

struct InverterTelemetry {
    bool valid = false;
    unsigned long last_update = 0;

    // Grid (AC In)
    float grid_voltage = 0.0;
    float grid_frequency = 0.0;

    // Inverter AC Output
    float ac_output_voltage = 0.0;
    float ac_output_frequency = 0.0;
    int ac_output_va = 0;
    int ac_output_watts = 0;
    int load_percent = 0;

    // Battery
    float battery_voltage = 0.0;
    int battery_charge_current = 0;
    int battery_soc = 0;
    int battery_discharge_current = 0;
    float battery_power_watts = 0.0;

    // Solar PV
    float pv_voltage = 0.0;
    float pv_current = 0.0;
    float pv_watts = 0.0;

    // System
    float heatsink_temp = 0.0;
    char mode_char = 'S';
    String mode_string = "Standby";
    String device_status_bits = "00000000";
    bool load_on_inverter = false;
    bool solar_charging = false;
    bool grid_charging = false;
};

class VoltronicProtocol {
public:
    static uint16_t calculateCRC(const char *buf, int len) {
        uint16_t crc = 0;
        for (int i = 0; i < len; i++) {
            crc = (uint16_t)((crc >> 8) | (crc << 8));
            crc ^= (uint8_t)buf[i];
            crc ^= (uint8_t)((crc & 0xFF) >> 4);
            crc ^= (uint16_t)((crc << 8) << 4);
            crc ^= (uint16_t)(((crc & 0xFF) << 4) << 1);
        }
        uint8_t h = (crc >> 8) & 0xFF;
        uint8_t l = crc & 0xFF;
        if (h == 0x0A || h == 0x0D || h == 0x28) h++;
        if (l == 0x0A || l == 0x0D || l == 0x28) l++;
        return ((uint16_t)h << 8) | l;
    }

    static void sendCommand(HardwareSerial &serial, const char *cmd) {
        int len = strlen(cmd);
        uint16_t crc = calculateCRC(cmd, len);
        serial.print(cmd);
        serial.write((uint8_t)(crc >> 8));
        serial.write((uint8_t)(crc & 0xFF));
        serial.write('\r');
    }

    static bool parseQPIGS(const String &response, InverterTelemetry &data) {
        // Expected format starts with '('
        int start = response.indexOf('(');
        if (start < 0) return false;

        String payload = response.substring(start + 1);
        payload.trim();

        // Split by space
        int count = 0;
        String tokens[32];
        int lastIdx = 0;
        for (int i = 0; i <= payload.length(); i++) {
            if (i == payload.length() || payload[i] == ' ') {
                if (i > lastIdx) {
                    if (count < 32) {
                        tokens[count++] = payload.substring(lastIdx, i);
                    }
                }
                lastIdx = i + 1;
            }
        }

        if (count < 16) return false;

        data.grid_voltage = tokens[0].toFloat();
        data.grid_frequency = tokens[1].toFloat();
        data.ac_output_voltage = tokens[2].toFloat();
        data.ac_output_frequency = tokens[3].toFloat();
        data.ac_output_va = tokens[4].toInt();
        data.ac_output_watts = tokens[5].toInt();
        data.load_percent = tokens[6].toInt();
        // tokens[7] is bus voltage
        data.battery_voltage = tokens[8].toFloat();
        data.battery_charge_current = tokens[9].toInt();
        data.battery_soc = tokens[10].toInt();
        data.heatsink_temp = tokens[11].toFloat();
        data.pv_current = tokens[12].toFloat();
        data.pv_voltage = tokens[13].toFloat();
        data.pv_watts = data.pv_voltage * data.pv_current;

        // tokens[14] is battery voltage from SCC
        data.battery_discharge_current = tokens[15].toInt();
        
        // Calculate net battery watts (Discharge - Charge)
        if (data.battery_charge_current > 0) {
            data.battery_power_watts = (float)data.battery_charge_current * data.battery_voltage;
        } else if (data.battery_discharge_current > 0) {
            data.battery_power_watts = -(float)data.battery_discharge_current * data.battery_voltage;
        } else {
            data.battery_power_watts = 0.0;
        }

        if (count > 16) {
            data.device_status_bits = tokens[16];
            if (data.device_status_bits.length() >= 8) {
                data.solar_charging = (data.device_status_bits[2] == '1');
                data.grid_charging = (data.device_status_bits[4] == '1');
                data.load_on_inverter = (data.device_status_bits[6] == '1');
            }
        }

        data.valid = true;
        data.last_update = millis();
        return true;
    }

    static void parseQMOD(const String &response, InverterTelemetry &data) {
        int start = response.indexOf('(');
        if (start < 0 || start + 1 >= response.length()) return;
        data.mode_char = response[start + 1];
        switch (data.mode_char) {
            case 'P': data.mode_string = "Power On"; break;
            case 'S': data.mode_string = "Standby"; break;
            case 'L': data.mode_string = "Line Mode (Grid)"; break;
            case 'B': data.mode_string = "Battery / Solar"; break;
            case 'F': data.mode_string = "Fault Mode"; break;
            case 'H': data.mode_string = "Power Saving"; break;
            default:  data.mode_string = "Unknown (" + String(data.mode_char) + ")"; break;
        }
    }
};
