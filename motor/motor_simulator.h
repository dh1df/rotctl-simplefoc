#ifndef MOTOR_SIMULATOR_H
#define MOTOR_SIMULATOR_H

#include <Arduino.h>
#include "../config.h"

#define ANGLE_TOLERANCE   1.0f

/**
 * Simulates a motor with:
 * - Configurable speed and acceleration
 * - Inertia (delayed start and braking)
 * - Noise (simulates encoder inaccuracy)
 * - Backlash (backlash simulation)
 * - Overflow capability (more than 360°)
 */
class MotorSimulator {
public:
    struct Config {
        // Physical parameters
        float maxSpeed;          // Maximum speed in °/s
        float acceleration;      // Acceleration in °/s²
        float deceleration;      // Deceleration in °/s²
        float inertia;           // Inertia (0=immediate, 1=very sluggish)
        float backlash;          // Reversal backlash in degrees
        float noiseAmplitude;    // Noise amplitude in degrees
        float noiseFrequency;    // Noise frequency in Hz
        
        // Default values
        Config() :
            maxSpeed(45.0f),
            acceleration(90.0f),
            deceleration(90.0f),
            inertia(0.8f),
            backlash(2.0f),
            noiseAmplitude(0.5f),
            noiseFrequency(10.0f)
        {}
    };
    
private:
    Config config;
    RotorState* state;
    
    // State
    float currentPosition;      // current position in degrees
    float targetPosition;       // Target position
    float currentSpeed;         // current speed in °/s
    float targetSpeed;          // Desired speed
    bool moving;
    bool brake;
    
    // Backlash management
    float backlashOffset;
    float lastDirection;        // -1, 0, +1
    
    // Noise
    float noisePhase;
    unsigned long lastUpdate;
    float lastNoise;
    
public:
    MotorSimulator() :
        currentPosition(0),
        targetPosition(0),
        currentSpeed(0),
        targetSpeed(0),
        moving(false),
        brake(false),
        backlashOffset(0),
        lastDirection(0),
        noisePhase(0),
        lastUpdate(0),
        lastNoise(0)
    {}
    
    // Initialization
    void begin(float startPosition, RotorState *rotor_state) {
        state = rotor_state;
        currentPosition = startPosition;
        targetPosition = startPosition;
        currentSpeed = 0;
        moving = false;
        brake = false;
        lastUpdate = millis();
        
        Serial.println("[Sim] Motor simulator started");
        Serial.printf("[Sim] Start position: %.1f°\n", startPosition);
	setProfileSlow();
    }
    
    // Main update (im loop() aufrufen)
    void loop() {
        unsigned long now = millis();
        float dt = (now - lastUpdate) / 1000.0f;
        lastUpdate = now;
        
        // Limit time step (prevents jumps after pauses)
        if (dt > 0.1f) dt = 0.1f;
        if (dt <= 0) return;
        
        // Brake
        if (brake) {
            applyBrake(dt);
            return;
        }
        
        // Target reached?
        float remaining = targetPosition - currentPosition;
        if (fabs(remaining) < ANGLE_TOLERANCE && fabs(currentSpeed) < 0.1f) {
            currentSpeed = 0;
            moving = false;
            return;
        }
        
        // Backlash check
        float effectiveTarget = targetPosition + calculateBacklash(remaining);
        
        // Calculate desired direction and speed
        float direction = (effectiveTarget > currentPosition) ? 1.0f : -1.0f;
        float distance = fabs(effectiveTarget - currentPosition);
        
        // Calculate braking distance
        float brakingDistance = (currentSpeed * currentSpeed) / (2.0f * config.deceleration);
        
        // Decision: accelerating or braking
        if (distance > brakingDistance * 1.1f) {
            // Still accelerating
            targetSpeed = direction * config.maxSpeed;
        } else {
            // Initiate braking but avoid overshooting target
            float decelSpeed = sqrt(2.0f * config.deceleration * distance) * 0.9f;
            targetSpeed = direction * min(decelSpeed, config.maxSpeed);
        }
        
        // Minimum speed for very small distances
        if (distance < 0.1f) {
            targetSpeed = 0;
            currentPosition = targetPosition;
        }
        
        // Adjust speed (with inertia)
        updateSpeed(targetSpeed, dt);
        
        // Update position
        currentPosition += currentSpeed * dt;
        
        // Add noise
        currentPosition += getNoise(dt);
        if (state) {
            state->azimuth.current_angle = currentPosition;
        }

    }
    
    // Set position
    void setTargetAngle(float target) {
        targetPosition = target;
        moving = true;
        brake = false;
#if 0
        Serial.printf("[Sim] New target: %.1f° (current: %.1f°)\n", 
                     targetPosition, currentPosition);
#endif
    }
    
    // Stop
    void stop() {
        brake = true;
        moving = false;
        Serial.printf("[Sim] Stop at %.1f°\n", currentPosition);
    }
    
    // Emergency stop (stop immediately)
    void emergencyStop() {
        currentSpeed = 0;
        targetSpeed = 0;
        moving = false;
        brake = true;
        Serial.printf("[Sim] Emergency stop! Position: %.1f°\n", currentPosition);
    }
    
    // Getter
    float getPosition() const { return currentPosition; }
    float getTarget() const { return targetPosition; }
    float getSpeed() const { return currentSpeed; }
    bool isMoving() const { return moving && fabs(currentSpeed) > 0.01f; }
    bool isBrake() const { return brake; }
    float getRemainingDistance() const { return targetPosition - currentPosition; }
    
    // Configuration
    Config& getConfig() { return config; }
    
    void setConfig(const Config& cfg) {
        config = cfg;
        Serial.println("[Sim] New Configuration:");
        printConfig();
    }
    
    void printConfig() {
        Serial.println("--- Motor-Simulator Configuration ---");
        Serial.printf("  Max Speed:      %.1f °/s\n", config.maxSpeed);
        Serial.printf("  Acceleration: %.1f °/s²\n", config.acceleration);
        Serial.printf("  Braking:        %.1f °/s²\n", config.deceleration);
        Serial.printf("  Inertia:       %.2f\n", config.inertia);
        Serial.printf("  Backlash:       %.2f°\n", config.backlash);
        Serial.printf("  Noise:       %.2f° @ %.1f Hz\n", 
                     config.noiseAmplitude, config.noiseFrequency);
        Serial.println("--------------------------------------");
    }
    
    // Predefined profiles
    void setProfileLight() {
        // Light, fast rotor (e.g. small 2m/70cm Yagi)
        config.maxSpeed = 60.0f;
        config.acceleration = 120.0f;
        config.deceleration = 120.0f;
        config.inertia = 0.5f;
        config.backlash = 0.5f;
        config.noiseAmplitude = 0.2f;
        Serial.println("[Sim] Profile: Light rotor");
    }
    
    void setProfileMedium() {
        // Medium rotor (default)
        config.maxSpeed = 30.0f;
        config.acceleration = 60.0f;
        config.deceleration = 60.0f;
        config.inertia = 0.8f;
        config.backlash = 2.0f;
        config.noiseAmplitude = 0.5f;
        Serial.println("[Sim] Profile: Medium rotor");
    }
    
    void setProfileHeavy() {
        // Heavy rotor (e.g. large shortwave beam)
        config.maxSpeed = 15.0f;
        config.acceleration = 30.0f;
        config.deceleration = 30.0f;
        config.inertia = 0.95f;
        config.backlash = 3.0f;
        config.noiseAmplitude = 1.0f;
        Serial.println("[Sim] Profile: Heavy rotor");
    }
    
    void setProfileSlow() {
        config.maxSpeed = 2.0f;
        config.acceleration = 5.0f;
        config.deceleration = 5.0f;
        config.inertia = 0.1f;
        config.backlash = 0.0f;
        config.noiseAmplitude = 0.0f;
        Serial.println("[Sim] Profile: Slow rotor");
    }
    
private:
    void updateSpeed(float desiredSpeed, float dt) {
        // Inertia: current speed approaches target speed
        float maxChange = config.acceleration * dt;
        
        if (fabs(desiredSpeed) > fabs(currentSpeed)) {
            // Accelerating
            float diff = desiredSpeed - currentSpeed;
            if (fabs(diff) > maxChange) {
                currentSpeed += (diff > 0 ? maxChange : -maxChange);
            } else {
                currentSpeed = desiredSpeed;
            }
        } else {
            // Braking
            float diff = desiredSpeed - currentSpeed;
            float maxDecel = config.deceleration * dt;
            if (fabs(diff) > maxDecel) {
                currentSpeed += (diff > 0 ? maxDecel : -maxDecel);
            } else {
                currentSpeed = desiredSpeed;
            }
        }
        
        // Inertia filter
        currentSpeed = currentSpeed * config.inertia + 
                       targetSpeed * (1.0f - config.inertia);
        targetSpeed = desiredSpeed;
    }
    
    void applyBrake(float dt) {
        // Braking with constant deceleration
        if (fabs(currentSpeed) > 0.01f) {
            float decel = config.deceleration * dt;
            if (currentSpeed > 0) {
                currentSpeed = max(0.0f, currentSpeed - decel);
            } else {
                currentSpeed = min(0.0f, currentSpeed + decel);
            }
            currentPosition += currentSpeed * dt;
        } else {
            currentSpeed = 0;
            moving = false;
        }
    }
    
    float calculateBacklash(float remainingDistance) {
        float direction = (remainingDistance > 0) ? 1.0f : -1.0f;
        
        // Direction change?
        if (direction != lastDirection && lastDirection != 0) {
            backlashOffset = -direction * config.backlash;
#if 0
            Serial.printf("[Sim] Backlash: %.2f° (Direction change)\n", 
                         config.backlash);
#endif
        }
        lastDirection = direction;
        
        return backlashOffset;
    }
    
    float getNoise(float dt) {
        // Simple but realistic noise
        noisePhase += config.noiseFrequency * dt * TWO_PI;
        if (noisePhase > TWO_PI) noisePhase -= TWO_PI;
        
        // Combination of sine (systematic) and random
        float sinusoidal = sin(noisePhase) * config.noiseAmplitude * 0.7f;
        float rnd = ((float)random(-1000, 1001) / 1000.0f) * 
                       config.noiseAmplitude * 0.3f;
        
        // Limit change (realistic encoder behavior)
        float noise = sinusoidal + rnd;
        float maxChange = config.noiseAmplitude * 0.1f;
        noise = constrain(noise, lastNoise - maxChange, lastNoise + maxChange);
        lastNoise = noise;
        
        return noise;
    }
};

#endif
