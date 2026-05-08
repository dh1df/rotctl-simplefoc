#ifndef MOTOR_STATE_H
#define MOTOR_STATE_H

#include <Arduino.h>

struct AxisState {
    float current_angle;    // Current angle position
    float target_angle;     // Target angle
    float max_speed;        // Maximum speed °/s
    bool  moving;           // Currently moving
    bool  brake;            // Brake active
    unsigned long movement_start_ms;
    
    AxisState() : current_angle(0), target_angle(0), max_speed(20),
                  moving(false), brake(false), movement_start_ms(0) {}
};

struct RotorPosition {
    float azimuth;
    float elevation;
    
    RotorPosition() : azimuth(0), elevation(0) {}
    RotorPosition(float az, float el) : azimuth(az), elevation(el) {}
    
    // Helper methods
    String toString() const {
        return String(azimuth, 1) + " " + String(elevation, 1);
    }
    
    bool isValid() const {
        return !isnan(azimuth) && !isnan(elevation);
    }
};

class RotorState {
private:
    float azMin = FALLBACK_AZ_MIN_ANGLE;
    float azMax = FALLBACK_AZ_MAX_ANGLE;
    float elMin = FALLBACK_EL_MIN_ANGLE;
    float elMax = FALLBACK_EL_MAX_ANGLE;
public:
    AxisState azimuth;
    AxisState elevation;
    
    void stop() {
        azimuth.moving = false;
        elevation.moving = false;
        azimuth.brake = true;
        elevation.brake = true;
    }
    
    // Check if target reached
    bool isAzimuthOnTarget() {
        return abs(azimuth.current_angle - azimuth.target_angle) < FALLBACK_ANGLE_TOLERANCE;
    }
    
    bool isElevationOnTarget() {
        return abs(elevation.current_angle - elevation.target_angle) < FALLBACK_ANGLE_TOLERANCE;
    }

    void setLimits(float az_min, float az_max, float el_min, float el_max) {
        azMin = az_min;
        azMax = az_max;
        elMin = el_min;
        elMax = el_max;
    }
    
    void setTarget(float az, float el, float az_speed = -1, float el_speed = -1) {
        if (az_speed > 0) azimuth.max_speed = az_speed;
        if (el_speed > 0) elevation.max_speed = el_speed;
        
        azimuth.target_angle = normalizeAzimuth(az);
        elevation.target_angle = constrain(el, elMin, elMax);
        
        azimuth.moving = true;
        elevation.moving = true;
        azimuth.brake = false;
        elevation.brake = false;
    }
    
private:
    float normalizeAzimuth(float angle) {
        // Seamless overflow: 450° -> 90° mechanically, but tracking preserves overflow
        return constrain(fmod(angle + 180.0f, 360.0f) - 180.0f, azMin, azMax);
    }
};

#endif
