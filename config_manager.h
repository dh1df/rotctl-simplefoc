#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

struct WifiConfig {
    String ssid;
    String password;
    String hostname;
};

struct NetworkConfig {
    uint16_t easycomm_port;
    uint16_t xmlrpc_port;
};

struct AxisLimits {
    float min_angle;
    float max_angle;
};

struct RotorConfig {
    AxisLimits azimuth;
    AxisLimits elevation;
    float default_speed_az;
    float default_speed_el;
    float angle_tolerance;
};

struct MotorPins {
    uint8_t pwm;
    uint8_t enable;
};

struct MotorConfig {
    MotorPins azimuth;
    MotorPins elevation;
};

class ConfigManager {
private:
    static constexpr const char* CONFIG_FILE = "/config.json";
    
    WifiConfig wifi;
    NetworkConfig network;
    RotorConfig rotor;
    MotorConfig motors;
    
    bool loaded;
    
public:
    ConfigManager() : loaded(false) {}
    
    bool begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("[Config] SPIFFS mount failed");
            return false;
        }
        
        if (!loadConfig()) {
            Serial.println("[Config] Configuration could not be loaded");
            return false;
        }
        
        printConfig();
        return true;
    }
    
    bool loadConfig() {
        File configFile = SPIFFS.open(CONFIG_FILE, "r");
        if (!configFile) {
            Serial.printf("[Config] %s not found\n", CONFIG_FILE);
            return false;
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, configFile);
        configFile.close();
        
        if (error) {
            Serial.printf("[Config] JSON Parse-Error: %s\n", error.c_str());
            return false;
        }
        
        // WiFi
        wifi.ssid = doc["wifi"]["ssid"] | "YOUR_SSID";
        wifi.password = doc["wifi"]["password"] | "YOUR_PASSWORD";
        wifi.hostname = doc["wifi"]["hostname"] | "antennenrotor";
        
        // Network
        network.easycomm_port = doc["network"]["easycomm_port"] | 4533;
        network.xmlrpc_port = doc["network"]["xmlrpc_port"] | 8080;
        
        // Rotor - Azimuth
        rotor.azimuth.min_angle = doc["rotor"]["azimuth"]["min_angle"] | -180.0f;
        rotor.azimuth.max_angle = doc["rotor"]["azimuth"]["max_angle"] | 540.0f;
        
        // Rotor - Elevation
        rotor.elevation.min_angle = doc["rotor"]["elevation"]["min_angle"] | 0.0f;
        rotor.elevation.max_angle = doc["rotor"]["elevation"]["max_angle"] | 90.0f;
        
        // Rotor - Defaults
        rotor.default_speed_az = doc["rotor"]["default_speed_az"] | 20.0f;
        rotor.default_speed_el = doc["rotor"]["default_speed_el"] | 15.0f;
        rotor.angle_tolerance = doc["rotor"]["angle_tolerance"] | 1.0f;
        
        // Motor Pins
        motors.azimuth.pwm = doc["motor_pins"]["azimuth"]["pwm"] | 12;
        motors.azimuth.enable = doc["motor_pins"]["azimuth"]["enable"] | 14;
        motors.elevation.pwm = doc["motor_pins"]["elevation"]["pwm"] | 13;
        motors.elevation.enable = doc["motor_pins"]["elevation"]["enable"] | 15;
        
        loaded = true;
        return true;
    }
    
    // Getter
    const WifiConfig& getWifi() const { return wifi; }
    const NetworkConfig& getNetwork() const { return network; }
    const RotorConfig& getRotor() const { return rotor; }
    const MotorConfig& getMotors() const { return motors; }
    bool isLoaded() const { return loaded; }
    
    // Short access for frequent values
    float getAzMinAngle() const { return rotor.azimuth.min_angle; }
    float getAzMaxAngle() const { return rotor.azimuth.max_angle; }
    float getElMinAngle() const { return rotor.elevation.min_angle; }
    float getElMaxAngle() const { return rotor.elevation.max_angle; }
    float getDefaultSpeedAz() const { return rotor.default_speed_az; }
    float getDefaultSpeedEl() const { return rotor.default_speed_el; }
    float getAngleTolerance() const { return rotor.angle_tolerance; }
    
    uint16_t getEasycommPort() const { return network.easycomm_port; }
    uint16_t getXmlrpcPort() const { return network.xmlrpc_port; }
    
private:
    void printConfig() {
        Serial.println("\n╔══════════════════════════════════════╗");
        Serial.println("║        Loaded Configuration        ║");
        Serial.println("╠══════════════════════════════════════╣");
        Serial.printf("║ WiFi SSID:    %s\n", wifi.ssid.c_str());
        Serial.printf("║ Hostname:     %s\n", wifi.hostname.c_str());
        Serial.printf("║ Easycomm:     Port %d\n", network.easycomm_port);
        Serial.printf("║ XMLRPC:       Port %d\n", network.xmlrpc_port);
        Serial.println("╠══════════════════════════════════════╣");
        Serial.printf("║ AZ Range:   %.0f° ... %.0f°\n", rotor.azimuth.min_angle, rotor.azimuth.max_angle);
        Serial.printf("║ EL Range:   %.0f° ... %.0f°\n", rotor.elevation.min_angle, rotor.elevation.max_angle);
        Serial.printf("║ Speed AZ:     %.1f °/s\n", rotor.default_speed_az);
        Serial.printf("║ Speed EL:     %.1f °/s\n", rotor.default_speed_el);
        Serial.printf("║ Tolerance:     ±%.1f°\n", rotor.angle_tolerance);
        Serial.println("╚══════════════════════════════════════╝\n");
    }
};

#endif
