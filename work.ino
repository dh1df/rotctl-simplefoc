#define ENABLE_WIFI
#define ENABLE_NET
#define ENABLE_JSON
#define DEBUG_SERIAL Serial

#ifdef ENABLE_WIFI
#include <WiFi.h>
#endif

#undef ENABLE_WIFI
#include "config.h"
#include "config_manager.h"
#include "motor/motor_state.h"
#include "motor/motor_simulator.h"
#include "protocols/protocol_manager.h"
#define ENABLE_WIFI

ConfigManager config;
RotorState rotorState;
ProtocolManager protoManager(&rotorState);
MotorSimulator azMotor;

unsigned long lastDebug = 0;

void setup() {
    DEBUG_SERIAL.begin(115200);
    delay(1000);
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║  Antenna Rotor - SPIFFS Config       ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝");
    // SPIFFS mit Configuration laden
    if (!config.begin()) {
        DEBUG_SERIAL.println("[Setup] Error: Configuration nicht loaded!");
        DEBUG_SERIAL.println("[Setup] Using fallback values from config.h");
    }
    
    // Initialize RotorState with loaded limits
    if (config.isLoaded()) {
#ifdef ENABLE_WIFI
  WiFi.setHostname(config.getWifi().hostname.c_str());
  WiFi.mode(WIFI_STA); //Optional
  WiFi.begin(config.getWifi().ssid, config.getWifi().password);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
#endif

        // Here you could set the limits dynamically
        // rotorState.setLimits(config.getAzMinAngle(), config.getAzMaxAngle(), ...);
    }
    
    protoManager.begin();
    azMotor.begin(0, &rotorState);
    DEBUG_SERIAL.println("[Setup] Ready for commands");
}

void loop() {
    protoManager.handle();
    
    // Motor control as before...

    azMotor.loop();   

    if (rotorState.azimuth.moving && !rotorState.azimuth.brake) {
        azMotor.setTargetAngle(rotorState.azimuth.target_angle);
    }
 
    if (millis() - lastDebug > 1000) {
        lastDebug = millis();
        if (rotorState.azimuth.moving) {
            float diff = rotorState.azimuth.target_angle - rotorState.azimuth.current_angle;
            DEBUG_SERIAL.printf("\r[%lu] AZ:%.1f°->%.1f° (Δ=%.1f°)  ", 
                millis()/1000,
                rotorState.azimuth.current_angle,
                rotorState.azimuth.target_angle,
                diff);
        }
    }
    
    delay(1);
}
