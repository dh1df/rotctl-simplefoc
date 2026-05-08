#ifndef DEBUG_HANDLER_H
#define DEBUG_HANDLER_H

#include <Arduino.h>
#include <SPIFFS.h>

class DebugHandler {
public:
    static void handleDebugCommand(const String& cmd, Stream& output) {
        if (cmd == "help" || cmd == "?") {
            printHelp(output);
        }
        else if (cmd == "spiffs" || cmd == "fs") {
            handleSpiffsInfo(output);
        }
        else if (cmd.startsWith("spiffs init") || cmd == "fs init") {
            handleSpiffsInit(output);
        }
        else if (cmd.startsWith("spiffs format") || cmd == "fs format") {
            handleSpiffsFormat(output);
        }
        else if (cmd == "spiffs test" || cmd == "fs test") {
            handleSpiffsTest(output);
        }
        else if (cmd == "spiffs list" || cmd == "ls") {
            handleSpiffsList(output);
        }
        else if (cmd == "spiffs check" || cmd == "fs check") {
            handleSpiffsCheck(output);
        }
        else if (cmd.startsWith("spiffs read ")) {
            String filename = cmd.substring(12);
            filename.trim();
            handleSpiffsRead(filename, output);
        }
        else if (cmd == "sysinfo" || cmd == "info") {
            handleSysInfo(output);
        }
        else if (cmd == "reboot" || cmd == "reset") {
            output.println("Reboot in 2 Sekunden...");
            delay(2000);
            ESP.restart();
        }
        else {
            output.println("Unknown debug command. 'debug help' für Help");
        }
    }
    
private:
    static void printHelp(Stream& output) {
        output.println("\n╔══════════════════════════════════════════╗");
        output.println("║         Debug-Commands & Help          ║");
        output.println("╠══════════════════════════════════════════╣");
        output.println("║ SPIFFS Management:");
        output.println("║   spiffs init     - SPIFFS initialize");
        output.println("║   spiffs format   - SPIFFS format");
        output.println("║   spiffs check    - SPIFFS check");
        output.println("║   spiffs info     - SPIFFS Show info");
        output.println("║   spiffs list/ls  - List files");
        output.println("║   spiffs test     - Read/Write test");
        output.println("║   spiffs read <f> - Output file");
        output.println("╠══════════════════════════════════════════╣");
        output.println("║ System:");
        output.println("║   sysinfo/info    - System Information");
        output.println("║   reboot/reset    - Restart");
        output.println("║   help/?          - This help");
        output.println("╚══════════════════════════════════════════╝\n");
    }
    
    static void handleSpiffsInit(Stream& output) {
        output.println("[SPIFFS] Initialize...");
        
        // check ob Already mounted
        if (SPIFFS.begin(false)) {
            output.println("[SPIFFS] ✓ Already mounted");
            printSpiffsInfo(output);
            return;
        }
#if 0        
        // format und mounten
        output.println("[SPIFFS] Trying format...");
        if (!SPIFFS.begin(true)) {
            output.println("[SPIFFS] ✗ Error: SPIFFS could not be initialized!");
            output.println("[SPIFFS] Possible causes:");
            output.println("  - Flash-Memory corrupt");
            output.println("  - Partition table incorrect");
            output.println("  - Too little memory reserved");
            return;
        }
        
        output.println("[SPIFFS] ✓ Successfully initialized and formatted");
        printSpiffsInfo(output);
        
        // Create test file
        createDefaultConfig(output);
#endif
    }
    
    static void handleSpiffsFormat(Stream& output) {
        output.println("[SPIFFS] ATTENTION: Formatting SPIFFS...");
        
        if (SPIFFS.format()) {
            output.println("[SPIFFS] ✓ Format successful");
            
            if (SPIFFS.begin(true)) {
                output.println("[SPIFFS] ✓ Remounted");
            }
        } else {
            output.println("[SPIFFS] ✗ Format failed!");
        }
    }
    
    static void handleSpiffsInfo(Stream& output) {
        printSpiffsInfo(output);
    }
    
    static void handleSpiffsCheck(Stream& output) {
        output.println("[SPIFFS] Check filesystem...");
        
        if (!SPIFFS.begin(false)) {
            output.println("[SPIFFS] ✗ Not mounted! Führe 'debug spiffs init' aus");
            return;
        }
        
        size_t total = SPIFFS.totalBytes();
        size_t used = SPIFFS.usedBytes();
        size_t free = total - used;
        
        output.printf("[SPIFFS] Status: %s\n", 
            free > 0 ? "✓ OK" : "✗ VOLL");
        output.printf("  Total: %u Bytes (%.1f KB)\n", total, total / 1024.0);
        output.printf("  Used:  %u Bytes (%.1f KB)\n", used, used / 1024.0);
        output.printf("  Free:  %u Bytes (%.1f KB)\n", free, free / 1024.0);
        
        if (free < 1024) {
            output.println("[SPIFFS] ⚠ WARNING: Low memory free!");
        }
    }
    
    static void handleSpiffsList(Stream& output) {
        if (!SPIFFS.begin(false)) {
            output.println("[SPIFFS] ✗ Not mounted! Führe 'debug spiffs init' aus");
            return;
        }
        
        output.println("\n╔══════════════════════════════════════════╗");
        output.println("║           SPIFFS File list              ║");
        output.println("╠══════════════════════════════════════════╣");
        
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        int count = 0;
        
        while (file) {
            count++;
            String filename = String(file.name());
            size_t size = file.size();
            
            output.printf("║ %-25s %8u Bytes\n", filename.c_str(), size);
            file.close();
            file = root.openNextFile();
        }
        
        if (count == 0) {
            output.println("║         (no files)                  ║");
        }
        
        output.println("╚══════════════════════════════════════════╝");
        
        size_t total = SPIFFS.totalBytes();
        size_t used = SPIFFS.usedBytes();
        output.printf("Total: %u/%u Bytes (%d Dateien)\n\n", used, total, count);
    }
    
    static void handleSpiffsTest(Stream& output) {
        if (!SPIFFS.begin(false)) {
            output.println("[SPIFFS] ✗ Not mounted! Führe 'debug spiffs init' aus");
            return;
        }
        
        output.println("[SPIFFS] Read/Write test...");
        
        const char* testFile = "/test.txt";
        const char* testContent = "SPIFFS Test successful!";
        
        // Write
        File f = SPIFFS.open(testFile, "w");
        if (!f) {
            output.println("[SPIFFS] ✗ Error: Cannot create test file");
            return;
        }
        
        size_t written = f.print(testContent);
        f.close();
        output.printf("[SPIFFS] ✓ %u Bytes written\n", written);
        
        // Read
        f = SPIFFS.open(testFile, "r");
        if (!f) {
            output.println("[SPIFFS] ✗ Error: Cannot read test file");
            return;
        }
        
        String content = f.readString();
        f.close();
        output.printf("[SPIFFS] ✓ Read: '%s'\n", content.c_str());
        
        // Delete
        SPIFFS.remove(testFile);
        output.println("[SPIFFS] ✓ Test file deleted");
        output.println("[SPIFFS] ✓ SPIFFS working correctly!");
    }
    
    static void handleSpiffsRead(const String& filename, Stream& output) {
        if (!SPIFFS.begin(false)) {
            output.println("[SPIFFS] ✗ Not mounted!");
            return;
        }
        
        if (filename.length() == 0) {
            output.println("Usage: debug spiffs read <filename>");
            return;
        }
        
        File f = SPIFFS.open(filename, "r");
        if (!f) {
            output.printf("[SPIFFS] ✗ File '%s' not found\n", filename.c_str());
            return;
        }
        
        output.printf("\n--- %s (%u Bytes) ---\n", filename.c_str(), f.size());
        while (f.available()) {
            output.write(f.read());
        }
        output.println("\n--- EOF ---\n");
        f.close();
    }
    
    static void handleSysInfo(Stream& output) {
        output.println("\n╔══════════════════════════════════════════╗");
        output.println("║         System Information             ║");
        output.println("╠══════════════════════════════════════════╣");
        
        output.printf("║ Chip:        %s\n", ESP.getChipModel());
        output.printf("║ CPU Freq:    %u MHz\n", ESP.getCpuFreqMHz());
        output.printf("║ Flash:       %u MB\n", ESP.getFlashChipSize() / (1024*1024));
        output.printf("║ Free Heap:   %u KB\n", ESP.getFreeHeap() / 1024);
        output.printf("║ PSRAM:       %u KB\n", ESP.getPsramSize() / 1024);
        output.printf("║ SDK:         %s\n", ESP.getSdkVersion());
        
        // SPIFFS Status
        output.println("╠══════════════════════════════════════════╣");
        if (SPIFFS.begin(false)) {
            output.printf("║ SPIFFS:      ✓ Mounted\n");
            output.printf("║ SPIFFS Size: %u KB\n", SPIFFS.totalBytes() / 1024);
            output.printf("║ SPIFFS Free: %u KB\n", 
                (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024);
        } else {
            output.printf("║ SPIFFS:      ✗ NOT mounted\n");
        }
        
        output.printf("║ Uptime:      %lu Sekunden\n", millis() / 1000);
        output.println("╚══════════════════════════════════════════╝\n");
    }
    
    static void printSpiffsInfo(Stream& output) {
        size_t total = SPIFFS.totalBytes();
        size_t used = SPIFFS.usedBytes();
        size_t free = total - used;
        
        output.println("\n╔══════════════════════════════════════════╗");
        output.println("║           SPIFFS Status                  ║");
        output.println("╠══════════════════════════════════════════╣");
        output.printf("║ Total: %8u Bytes (%6.1f KB)    ║\n", total, total / 1024.0);
        output.printf("║ Used:  %8u Bytes (%6.1f KB)    ║\n", used, used / 1024.0);
        output.printf("║ Free:  %8u Bytes (%6.1f KB)    ║\n", free, free / 1024.0);
        output.println("╚══════════════════════════════════════════╝\n");
    }
    
    static void createDefaultConfig(Stream& output) {
        const char* configFile = "/config.json";
        
        // check ob already exists
        if (SPIFFS.exists(configFile)) {
            output.printf("[SPIFFS] '%s' already exists\n", configFile);
            return;
        }
        
        output.printf("[SPIFFS] Creating default config '%s'...\n", configFile);
        
        File f = SPIFFS.open(configFile, "w");
        if (!f) {
            output.println("[SPIFFS] ✗ Error creating config!");
            return;
        }
        
        f.println("{");
        f.println("  \"wifi\": {");
        f.println("    \"ssid\": \"YOUR_SSID\",");
        f.println("    \"password\": \"YOUR_PASSWORD\",");
        f.println("    \"hostname\": \"antennenrotor\"");
        f.println("  },");
        f.println("  \"network\": {");
        f.println("    \"easycomm_port\": 4533,");
        f.println("    \"xmlrpc_port\": 8080");
        f.println("  },");
        f.println("  \"rotor\": {");
        f.println("    \"azimuth\": {");
        f.println("      \"min_angle\": -180.0,");
        f.println("      \"max_angle\": 540.0");
        f.println("    },");
        f.println("    \"elevation\": {");
        f.println("      \"min_angle\": 0.0,");
        f.println("      \"max_angle\": 90.0");
        f.println("    },");
        f.println("    \"default_speed_az\": 20.0,");
        f.println("    \"default_speed_el\": 15.0,");
        f.println("    \"angle_tolerance\": 1.0");
        f.println("  },");
        f.println("  \"motor_pins\": {");
        f.println("    \"azimuth\": {");
        f.println("      \"pwm\": 12,");
        f.println("      \"enable\": 14");
        f.println("    },");
        f.println("    \"elevation\": {");
        f.println("      \"pwm\": 13,");
        f.println("      \"enable\": 15");
        f.println("    }");
        f.println("  }");
        f.println("}");
        
        f.close();
        output.println("[SPIFFS] ✓ Default config created");
    }
};

#endif
