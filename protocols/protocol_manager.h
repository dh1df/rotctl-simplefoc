#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include "protocol_callbacks.h"
#include "protocol_serial.h"

#ifdef ENABLE_NET
#include "protocol_net.h" 
#endif
#ifdef ENABLE_JSON
#include "protocol_json.h"
#endif

#include "../motor/motor_state.h"

class ProtocolManager {
private:
    ProtocolCallbacks callbacks;
    
#ifdef ENABLE_NET
    Net net;
#endif
#ifdef ENABLE_JSON
    JsonApiServer json;
#endif
    
    RotorState* rotorState;
    
public:
    SerialInterface serial;
    
    ProtocolManager(RotorState* state) : rotorState(state) {
        setupCallbacks();
        serial.setCallbacks(&callbacks);
#ifdef ENABLE_NET
        net.setCallbacks(&callbacks);
#endif
#ifdef ENABLE_JSON
        json.setCallbacks(&callbacks);
#endif
    }
    
    void begin() {
        serial.begin();
#ifdef ENABLE_NET
        net.begin();
#endif
#ifdef ENABLE_JSON
        json.begin();
#endif
    }
    
    void handle() {
        serial.handle();
#ifdef ENABLE_NET
        net.handle();
#endif
#ifdef ENABLE_JSON
        json.handle();
#endif
    }
    
private:
    void setupCallbacks() {
        callbacks.onSetPosition = [this](float az, float el) {
            rotorState->setTarget(az, el);
        };
        
        callbacks.onStop = [this]() {
            rotorState->stop();
        };
        
        callbacks.onGetPosition = [this]() -> RotorPosition {
            return RotorPosition(
                rotorState->azimuth.current_angle,
                rotorState->elevation.current_angle
            );
        };
    }
};

#endif
