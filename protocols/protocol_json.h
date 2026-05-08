#ifndef PROTOCOL_JSON_H
#define PROTOCOL_JSON_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <functional>
#include "../config.h"

#define JSON_PORT 8080

typedef std::function<String(String)> JsonMethodHandler;

class JsonApiServer {
private:
    WebServer http;
    ProtocolCallbacks* callbacks;
    
public:
    JsonApiServer(uint16_t port = JSON_PORT) 
        : http(port), callbacks(nullptr) {}
    
    void setCallbacks(ProtocolCallbacks* cb) { callbacks = cb; }

    void begin() {
        http.enableCORS(true);

	http.on("/api/position", HTTP_OPTIONS, [this]() { handleOptions(); });        
        // RESTful JSON API Endpoints
        http.on("/api/position", HTTP_GET, [this]() { handleGetPosition(); });
        http.on("/api/position", HTTP_POST, [this]() { handleSetPosition(); });
        http.on("/api/stop", HTTP_POST, [this]() { handleStop(); });
        http.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
        
        // rotctld-compatible endpoint (XML/JSON mixed)
        http.on("/rotctl", HTTP_POST, [this]() { handleJsonRpcRequest(); });
        
        // Root Endpoint mit API Info
        http.on("/", HTTP_GET, [this]() { handleApiInfo(); });
        
        http.begin();
        Serial.println("[JSON API] Server started auf Port " + String(JSON_PORT));
    }
    
    void handle() {
        http.handleClient();
    }
    
private:
    void sendJsonResponse(int code, JsonDocument& doc) {
        String response;
        serializeJson(doc, response);
        http.send(code, "application/json", response);
    }
    
    void sendError(int code, const String& message) {
        JsonDocument doc;
        doc["success"] = false;
        doc["error"] = message;
        doc["code"] = code;
        sendJsonResponse(code, doc);
    }
    
    void sendSuccess(JsonDocument& doc) {
        doc["success"] = true;
        sendJsonResponse(200, doc);
    }

    void handleOptions() {
	http.send(204);
    }
    
    void handleApiInfo() {
        JsonDocument doc;
        doc["name"] = "Antenna Rotor API";
        doc["version"] = "1.0";
        doc["endpoints"] = JsonArray();
        
        doc["endpoints"].add("/api/position [GET] - current Position query");
        doc["endpoints"].add("/api/position [POST] - New Set position");
        doc["endpoints"].add("/api/stop [POST] - Stop movement");
        doc["endpoints"].add("/api/status [GET] - Detailed status");
        doc["endpoints"].add("/rotctl [POST] - JSON-RPC compatible");
        
        doc["examples"] = JsonObject();
        doc["examples"]["set_position"] = "POST /api/position mit Body: {\"azimuth\": 180.0, \"elevation\": 45.0}";
        doc["examples"]["json_rpc"] = "POST /rotctl mit Body: {\"method\": \"set_position\", \"params\": [180.0, 45.0], \"id\": 1}";
        
        sendJsonResponse(200, doc);
    }
    
    void handleGetPosition() {
        if (!callbacks || !callbacks->onGetPosition) {
            sendError(503, "Position service not available");
            return;
        }
        
        RotorPosition pos = callbacks->onGetPosition();
        
        JsonDocument doc;
        doc["azimuth"] = pos.azimuth;
        doc["elevation"] = pos.elevation;
        
        if (callbacks->onGetPosition) {
            // Zusätzliche Statusinformationen
            doc["moving"] = false; // Can be expanded when available
        }
        
        sendSuccess(doc);
    }
    
    void handleSetPosition() {
        if (!callbacks || !callbacks->onSetPosition) {
            sendError(503, "Position service not available");
            return;
        }
        
        if (!http.hasArg("plain")) {
            sendError(400, "Missing JSON body");
            return;
        }
        
        String body = http.arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            JsonDocument errDoc;
            errDoc["success"] = false;
            errDoc["error"] = "Invalid JSON: " + String(error.c_str());
            sendJsonResponse(400, errDoc);
            return;
        }
        
        float azimuth = doc["azimuth"].as<float>();
        float elevation = doc["elevation"].as<float>();
        
        if (isnan(azimuth) || isnan(elevation)) {
            sendError(400, "Missing azimuth or elevation field");
            return;
        }
        
        callbacks->onSetPosition(azimuth, elevation);
        
        JsonDocument respDoc;
        respDoc["azimuth"] = azimuth;
        respDoc["elevation"] = elevation;
        respDoc["message"] = "Moving to new position";
        
        sendSuccess(respDoc);
    }
    
    void handleStop() {
        if (!callbacks || !callbacks->onStop) {
            sendError(503, "Stop service not available");
            return;
        }
        
        callbacks->onStop();
        
        JsonDocument doc;
        doc["message"] = "Movement stopped";
        sendSuccess(doc);
    }
    
    void handleStatus() {
        JsonDocument doc;
        
        if (callbacks && callbacks->onGetPosition) {
            RotorPosition pos = callbacks->onGetPosition();
            doc["position"]["azimuth"] = pos.azimuth;
            doc["position"]["elevation"] = pos.elevation;
        }
        
        doc["uptime_ms"] = millis();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["chip_model"] = ESP.getChipModel();
        
        sendSuccess(doc);
    }
    
    void handleJsonRpcRequest() {
        if (!http.hasArg("plain")) {
            sendJsonRpcError(-32700, "Parse error", nullptr);
            return;
        }
        
        String body = http.arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            sendJsonRpcError(-32700, "Parse error: " + String(error.c_str()), nullptr);
            return;
        }
        
        const char* method = doc["method"];
        JsonArray params = doc["params"].as<JsonArray>();
        int id = doc["id"] | 0;
        
        if (!method) {
            sendJsonRpcError(-32600, "Invalid Request: missing method", &id);
            return;
        }
        
        // JSON-RPC Methods
        if (strcmp(method, "set_position") == 0 || strcmp(method, "rotator_set_position") == 0) {
            if (params.size() < 2) {
                sendJsonRpcError(-32602, "Invalid params: need azimuth and elevation", &id);
                return;
            }
            
            float az = params[0];
            float el = params[1];
            
            if (callbacks && callbacks->onSetPosition) {
                callbacks->onSetPosition(az, el);
                
                JsonDocument result;
                result["azimuth"] = az;
                result["elevation"] = el;
                result["status"] = "moving";
                sendJsonRpcResult(result, id);
            } else {
                sendJsonRpcError(-32000, "Server error: position service unavailable", &id);
            }
        }
        else if (strcmp(method, "get_position") == 0 || strcmp(method, "rotator_get_position") == 0) {
            if (callbacks && callbacks->onGetPosition) {
                RotorPosition pos = callbacks->onGetPosition();
                
                JsonDocument result;
                result["azimuth"] = pos.azimuth;
                result["elevation"] = pos.elevation;
                sendJsonRpcResult(result, id);
            } else {
                sendJsonRpcError(-32000, "Server error: position service unavailable", &id);
            }
        }
        else if (strcmp(method, "stop") == 0 || strcmp(method, "rotator_stop") == 0) {
            if (callbacks && callbacks->onStop) {
                callbacks->onStop();
                
                JsonDocument result;
                result["status"] = "stopped";
                sendJsonRpcResult(result, id);
            } else {
                sendJsonRpcError(-32000, "Server error: stop service unavailable", &id);
            }
        }
        else {
            sendJsonRpcError(-32601, "Method not found: " + String(method), &id);
        }
    }
    
    void sendJsonRpcResult(JsonDocument& result, int id) {
        JsonDocument response;
        response["jsonrpc"] = "2.0";
        response["result"] = result;
        response["id"] = id;
        
        String output;
        serializeJson(response, output);
        http.send(200, "application/json", output);
    }
    
    void sendJsonRpcError(int code, const String& message, const int* id) {
        JsonDocument response;
        response["jsonrpc"] = "2.0";
        response["error"]["code"] = code;
        response["error"]["message"] = message;
        
        if (id) {
            response["id"] = *id;
        } else {
            response["id"] = nullptr;
        }
        
        String output;
        serializeJson(response, output);
        http.send(200, "application/json", output);
    }
};

#endif
