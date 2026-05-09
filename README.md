# rotctl-simplefoc

A SimpleFOC-based rotctl server running on ESP32 for remote antenna rotor control via network protocols.

## Overview

This project integrates [SimpleFOC](https://github.com/simplefoc/Arduino-FOC) motor control with [rotctl](https://github.com/Hamlib/Hamlib) protocol support to enable precise azimuth and elevation control of antenna rotors over TCP/IP. Perfect for amateur radio applications, satellite tracking, and automated antenna systems.

### Key Features

- **SimpleFOC Motor Control**: Field-oriented control for smooth, efficient rotor movement
- **Multiple Protocol Support**: EasyComm and XML-RPC for rotctl compatibility
- **WiFi Connectivity**: ESP32-based wireless control
- **JSON Configuration**: Easy configuration management via JSON files stored on SPIFFS
- **Dual-Axis Support**: Control both azimuth (AZ) and elevation (EL) axes
- **Motor Simulation**: Built-in simulator for testing without hardware
- **Debug Output**: Serial debugging with detailed state logging

## Hardware Requirements

- **Microcontroller**: ESP32 development board
- **Motor Control**: SimpleFOC-compatible motor driver
- **Motor**: Brushless motor (BLDC) suitable for antenna rotor
- **Power Supply**: Appropriate power for motor and ESP32
- **USB-to-Serial**: For initial programming and debugging

### Pin Configuration (Default)

| Function | Pin |
|----------|-----|
| AZ PWM | GPIO 12 |
| AZ Enable | GPIO 14 |
| EL PWM | GPIO 13 |
| EL Enable | GPIO 15 |

Modify these in `config.h` for your hardware setup.

## Installation

### Prerequisites

- [Arduino CLI](https://github.com/arduino/arduino-cli) installed
- ESP32 board package installed via Arduino CLI
- SimpleFOC library installed via Arduino IDE/CLI

### Building

1. Clone the repository:
```bash
git clone https://github.com/dh1df/rotctl-simplefoc.git
cd rotctl-simplefoc
```

2. Compile the sketch:
```bash
make build
```

3. Upload to ESP32:
```bash
make upload
```

### Configuration

1. Edit `data/config.json` with your settings:
```json
{
  "wifi": {
    "ssid": "YOUR_SSID",
    "password": "YOUR_PASSWORD",
    "hostname": "antennenrotor"
  },
  "easycomm_port": 4533,
  "xmlrpc_port": 8080,
  "az_min_angle": -180.0,
  "az_max_angle": 540.0,
  "el_min_angle": 0.0,
  "el_max_angle": 90.0,
  "default_speed_az": 20.0,
  "default_speed_el": 15.0,
  "angle_tolerance": 1.0
}
```

2. Upload filesystem with configuration:
```bash
make deploy-fs
```

## Usage

### Connecting via rotctl

Once running, connect using rotctl:
```bash
# Via EasyComm protocol (port 4533)
rotctl -m 4 -r 192.168.x.x:4533

# Via XML-RPC protocol (port 8080)
rotctl -m 1 -r 192.168.x.x:8080
```

### Serial Monitoring

Monitor the ESP32's serial output during operation:
```bash
make monitor
```

You'll see output like:
```
[Setup] Ready for commands
[125] AZ:45.0°->90.0° (Δ=45.0°)
```

### Motor Control Commands

Once connected via rotctl, you can:
- **Set Azimuth**: `set_pos 90` (set rotor to 90°)
- **Set Elevation**: `set_pos 90 45` (set to 90° AZ, 45° EL)
- **Query Position**: `get_pos` (returns current rotor position)
- **Stop Rotor**: `move_vel 0` (stop all movement)

## Project Structure

```
.
├── work.ino                  # Main sketch with setup() and loop()
├── config.h                  # Hardware pin configuration & fallback values
├── config_manager.h          # Configuration loading from SPIFFS/JSON
├── debug_handler.h           # Debug output and state logging
├── motor/
│   ├── motor_state.h         # Rotor state tracking (azimuth/elevation)
│   └── motor_simulator.h     # Motor simulation without hardware
├── protocols/
│   └── protocol_manager.h    # Protocol handling (EasyComm, XML-RPC)
├── html/                     # Web interface files (optional)
├── data/
│   └── config.json           # Runtime configuration
├── Makefile                  # Build automation
└── README.md                 # This file
```

## Configuration Details

### Angle Limits

- **Azimuth (AZ)**: Range from `az_min_angle` to `az_max_angle` (e.g., -180° to 540°)
- **Elevation (EL)**: Range from `el_min_angle` to `el_max_angle` (e.g., 0° to 90°)

### Speed Control

- `default_speed_az`: Azimuth movement speed (degrees per control cycle)
- `default_speed_el`: Elevation movement speed (degrees per control cycle)
- `angle_tolerance`: Tolerance in degrees for reaching target position

## Development

### Building and Flashing

```bash
# Clean build
make clean
make build

# Deploy code and monitor
make deploy

# Deploy code, filesystem, and monitor
make deploy-fs

# Just monitor serial output
make monitor
```

### Motor Simulator

The project includes a `MotorSimulator` for testing without hardware:
```cpp
MotorSimulator azMotor;
azMotor.begin(0, &rotorState);  // Initialize
azMotor.loop();                  // Call in main loop
```

The simulator tracks position based on commanded velocity and simulates realistic rotor behavior.

## Troubleshooting

### WiFi Connection Issues

- Check SSID and password in `data/config.json`
- Verify the ESP32 can reach your network
- Check serial output for connection errors

### Motor Not Moving

1. Verify motor connections and pin configuration
2. Check power supply to motor driver
3. Ensure SimpleFOC library is properly installed
4. Use motor simulator to test software flow

### Port Already in Use

If you get connection errors:
```bash
# Change ports in config.json to avoid conflicts
"easycomm_port": 4534,
"xmlrpc_port": 8081
```

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Test your changes thoroughly
4. Submit a pull request with a clear description

## License

This project is provided as-is. Please respect all applicable licenses from dependencies.

## Related Projects

- [SimpleFOC](https://github.com/simplefoc/Arduino-FOC) - Motor control library
- [Hamlib/rotctl](https://github.com/Hamlib/Hamlib) - Ham radio control library
- [Arduino-FOC-Antenna-Rotor](https://github.com/simplefoc/Arduino-FOC-examples) - Related examples

## Support & Documentation

For issues, questions, or suggestions, please open a GitHub issue in this repository.

---

**Status**: Work in Progress (WIP) - Core functionality is operational; additional features and refinements ongoing.
