#ifndef PROTOCOL_EASYCOMM_H
#define PROTOCOL_EASYCOMM_H

#include "../config.h"
#include "protocol_callbacks.h"

class Easycomm {
private:
    ProtocolCallbacks* callbacks;
    String commandBuffer;
    
public:
    Easycomm() : callbacks(nullptr) {}
    
    void setCallbacks(ProtocolCallbacks* cb) { callbacks = cb; }
    
    void begin() {
        Serial.println("[Easycomm] Server started");
    }
    
    bool parseEasycommCommand(String cmd, Stream &client) {
        if (!callbacks) return false;
        cmd.trim();
        if (cmd.length() < 2) return false;
        
        if (cmd == "AZ EL") {
            if (callbacks->onGetPosition) {
                RotorPosition pos = callbacks->onGetPosition();
                client.printf("AZ%f EL%f\n", pos.azimuth, pos.elevation);
            }
            return true;
        }
        else if (cmd == "VE") {
            client.println("VE Easycomm III ESP32 Rotor");
            return true;
        }
        else if (cmd.startsWith("SE ")) {
            float az, el;
            if (sscanf(cmd.c_str(), "SE %f %f", &az, &el) == 2) {
                if (callbacks->onSetPosition) callbacks->onSetPosition(az, el);
                client.println("OK");
            } else {
                client.println("ERROR");
            }
            return true;
        }
        else if (cmd == "ST") {
            if (callbacks->onStop) callbacks->onStop();
            client.println("OK");
            return true;
        }
        return false;
    }
};

#endif
