#ifndef PROTOCOL_NET_H
#define PROTOCOL_NET_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <functional>
#include "../config.h"

#define NET_PORT 4533

class Net {
private:
    WiFiServer server;
    WiFiClient client;
    ProtocolCallbacks* callbacks;
    
    String commandBuffer;
    
public:
    Net(uint16_t port = NET_PORT) 
        : server(port), callbacks(nullptr) {}

    void setCallbacks(ProtocolCallbacks* cb) { callbacks = cb; }
    
    void begin() {
        server.begin();
        Serial.println("[Net] Server started auf Port " + String(NET_PORT));
    }
    
    void handle() {
        if (!client || !client.connected()) {
            client = server.accept();
            if (client) {
                Serial.println("[Net] Client connected");
                commandBuffer = "";
            }
        }
        
        if (client && client.available()) {
            char c = client.read();
            if (c == '\n' || c == '\r') {
                if (commandBuffer.length() > 0) {
                    parseNetCommand(commandBuffer);
                    commandBuffer = "";
                }
            } else {
                commandBuffer += c;
            }
        }
    }
    
private:
    void parseNetCommand(String cmd) {
        cmd.trim();
	Serial.print("Net got '");
	Serial.print(cmd);
	Serial.println("'");
        
        // Net Commands sind CR/LF terminiert
        // Basic commands:
        
        if (cmd == "\\dump_state") {
            client.println("1");
            client.println("204");
            client.println("min_az=0.0");
            client.println("max_az=360.0");
            client.println("min_el=0.0");
            client.println("max_el=180.0");
            client.println("south_zero=0");
            client.println("rot_type=Other");
            client.println("done");
	} else if (cmd == "p") {
            // Positionsabfrage
            if (callbacks->onGetPosition) {
                String pos = callbacks->onGetPosition().toString();
                client.println("1.1");
                client.println("2.2");
            }
        }
        else if (cmd.startsWith("P ")) {
            // Set Position: SE <az> <el>
            float az, el;
            if (sscanf(cmd.c_str(), "P %f %f", &az, &el) == 2) {
                if (callbacks->onSetPosition) callbacks->onSetPosition(az, el);
                client.println("RPTR 0");
            } else {
                client.println("RPTR -1");
            }
        }
        else if (cmd == "S") {
            // Stop
            if (callbacks->onStop) callbacks->onStop();
            client.println("RPTR 0");
        }
        else {
            client.println("RPTR -2");
        }
    }
};

#endif
