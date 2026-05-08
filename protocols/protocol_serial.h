#ifndef PROTOCOL_SERIAL_H
#define PROTOCOL_SERIAL_H

#include <Arduino.h>
#include "../config.h"
#include "protocol_callbacks.h"
#include "protocol_easycomm.h"
#include "../debug_handler.h"

class SerialInterface {
private:
    HardwareSerial* serial;
    Easycomm easycomm;
    ProtocolCallbacks* callbacks;
    
    String commandBuffer;
    bool echo;
    
public:
    std::function<void(String&)> onDebugCommand;
    
    SerialInterface(HardwareSerial* port = &Serial) 
        : serial(port), echo(false), onDebugCommand(nullptr), callbacks(nullptr) {}
    
    void setCallbacks(ProtocolCallbacks* cb) { 
        callbacks = cb; 
        easycomm.setCallbacks(cb);
    }
    
    void begin(unsigned long baud = 115200) {
        serial->begin(baud);
        while (!*serial && millis() < 3000);
        serial->println("\n=== Antenna Rotor - Serial interface ===");
        serial->println("Commands: help, az/el, az, el, stop, status, debug");
        serial->print("> ");
    }
    
    void handle() {
        while (serial->available()) {
            char c = serial->read();
            
            if (echo && c != '\n' && c != '\r') {
                serial->print(c);
            }
            
            if (c == '\n' || c == '\r') {
                if (commandBuffer.length() > 0) {
                    if (echo) serial->println();
                    parseSerialCommand(commandBuffer);
                    commandBuffer = "";
                    serial->print("> ");
                }
            } else if (c == '\b' || c == 127) {
                if (commandBuffer.length() > 0) {
                    commandBuffer.remove(commandBuffer.length() - 1);
                    if (echo) serial->print("\b \b");
                }
            } else {
                commandBuffer += c;
            }
        }
    }
    
private:
    void parseSerialCommand(String cmd) {
        if (!callbacks) return;
        cmd.trim();
        if (cmd.length() == 0) return;
        
        // Erst Try Easycomm parser
        if (easycomm.parseEasycommCommand(cmd, *serial)) {
            return;
        }
        
        // Internal commands
        if (cmd.startsWith("az/el ") || cmd.startsWith("azel ")) {
            float az, el;
            if (sscanf(cmd.c_str() + cmd.indexOf(' '), "%f %f", &az, &el) == 2) {
                if (callbacks->onSetPosition) callbacks->onSetPosition(az, el);
                serial->printf("OK: Moving to AZ=%.1f° EL=%.1f°\n", az, el);
            } else {
                serial->println("Error: Syntax: az/el <azimuth> <elevation>");
            }
        }
        else if (cmd.startsWith("az ")) {
            float az;
            if (sscanf(cmd.c_str(), "az %f", &az) == 1) {
                float currentEl = (callbacks->onGetPosition) ? callbacks->onGetPosition().elevation : 0;
                if (callbacks->onSetPosition) callbacks->onSetPosition(az, currentEl);
                serial->printf("OK: Moving AZ zu %.1f°\n", az);
            }
        }
        else if (cmd.startsWith("el ")) {
            float el;
            if (sscanf(cmd.c_str(), "el %f", &el) == 1) {
                float currentAz = (callbacks->onGetPosition) ? callbacks->onGetPosition().azimuth : 0;
                if (callbacks->onSetPosition) callbacks->onSetPosition(currentAz, el);
                serial->printf("OK: Moving EL zu %.1f°\n", el);
            }
        }
        else if (cmd == "stop" || cmd == "s") {
            if (callbacks->onStop) callbacks->onStop();
            serial->println("OK: Stopped");
        }
        else if (cmd == "status" || cmd == "st") {
            if (callbacks->onGetPosition) {
                RotorPosition pos = callbacks->onGetPosition();
                serial->println("Position: " + pos.toString());
            } else {
                serial->println("Status not available");
            }
        }
        else if (cmd == "help" || cmd == "h" || cmd == "?") {
            printHelp();
        }
        else if (cmd == "echo off") {
            echo = false;
            serial->println("Echo off");
        }
        else if (cmd == "echo on") {
            echo = true;
            serial->println("Echo on");
        }
        else if (cmd.startsWith("debug ")) {
            String debugCmd = cmd.substring(6);
            debugCmd.trim();
            DebugHandler::handleDebugCommand(debugCmd, *serial);
        }
        else {
            serial->println("?");
        }
    }
    
    void printHelp() {
        serial->println("\n--- Serial Interface Commands ---");
        serial->println("az/el <az> <el>  - Set Azimuth und Elevation");
        serial->println("az <angle>       - Set azimuth only");
        serial->println("el <angle>       - Set elevation only");
        serial->println("stop/s            - Stop movement");
        serial->println("status/st         - Position query");
        serial->println("echo on/off       - Local echo on/aus");
        serial->println("debug <befehl>    - Send debug command");
        serial->println("help/h/?          - This help");
        serial->println("--- Easycomm-Compatible (auch über Serial) ---");
        serial->println("SE <az> <el>      - Easycomm Set Position");
        serial->println("AZ EL             - Easycomm Position query");
        serial->println("VE                - Easycomm Version");
        serial->println("ST                - Easycomm Stop");
        serial->println("----------------------------------------------\n");
    }
};

#endif
