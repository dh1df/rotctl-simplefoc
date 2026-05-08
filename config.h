#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID       "YOUR_SSID"       // Fallback, Will be overwritten
#define WIFI_PASSWORD   "YOUR_PASSWORD"   // Fallback
#define HOSTNAME        "antennenrotor"   // Fallback

// These values now come from config.json
// They remain as fallback defines if SPIFFS is not available
#define FALLBACK_EASYCOMM_PORT   4533
#define FALLBACK_XMLRPC_PORT     8080
#define FALLBACK_AZ_MIN_ANGLE    -180.0f
#define FALLBACK_AZ_MAX_ANGLE    540.0f
#define FALLBACK_EL_MIN_ANGLE    0.0f
#define FALLBACK_EL_MAX_ANGLE    90.0f
#define FALLBACK_DEFAULT_SPEED_AZ  20.0f
#define FALLBACK_DEFAULT_SPEED_EL  15.0f
#define FALLBACK_ANGLE_TOLERANCE   1.0f

// Pin-Defaults (Hardware-specific, rarely changed)
#define PIN_AZ_PWM      12
#define PIN_AZ_ENABLE   14
#define PIN_EL_PWM      13
#define PIN_EL_ENABLE   15

#endif
