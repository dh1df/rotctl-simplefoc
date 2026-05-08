#ifndef MOTOR_SIMPLEFOC_H
#define MOTOR_SIMPLEFOC_H

// ATTENTION: SimpleFOC-library must be installed!
// PlatformIO: lib_deps = askuric/Simple FOC@^2.3.0
// Arduino: Library Manager -> "SimpleFOC"

#include <SimpleFOC.h>
#include "../config.h"
#include "motor_state.h"

class SimpleFocMotor {
private:
    BLDC motor;
    Commander* controller;
    RotorState* state;
    
public:
    SimpleFocMotor() : controller(nullptr), state(nullptr) {}
    
    bool begin(int pwm_pin, int enable_pin, RotorState* rotor_state) {
        state = rotor_state;
        
        // Placeholder - your specific Motor/Encoder-Configuration
        // Example for a simple DC-Motor mit Encoder:
        motor.voltage_power_supply = 12;
        motor.voltage_limit = 6;
        motor.velocity_limit = 20; // rad/s
        
        // Pin Setup
        motor.enable_pin = enable_pin;
        
        // Motor initialize
        motor.init();
        motor.initFOC();
        
        Serial.println("[Motor] SimpleFOC initialized");
        return true;
    }
    
    void setTargetAngle(float target) {
        // Convert angle to radians for SimpleFOC
        motor.target = target * PI / 180.0f;
    }
    
    void loop() {
        // Execute FOC control loop
        motor.loopFOC();
        motor.move();
        
        // Read current position (rad -> degree)
        if (state) {
            state->current_angle = motor.shaft_angle * 180.0f / PI;
        }
    }
    
    void stop() {
        motor.target = motor.shaft_angle; // Hold current position
    }
    
    float getAngle() {
        return motor.shaft_angle * 180.0f / PI;
    }
};

#endif
