/*
 * Bluetooth Scanner for CG OBD Protocol Analysis
 * Captures device names, MAC patterns, and advertising data
 * for reverse engineering Chigee OBD module communication
 */

#include <BluetoothSerial.h>
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>
#include <esp_spp_api.h>

BluetoothSerial SerialBT;

// Structure to store discovered device info
struct BTDevice {
    String name;
    String address;
    int rssi;
    uint32_t cod;  // Class of Device
    bool hasName;
    String rawData;
};

// Store discovered devices
BTDevice discoveredDevices[50];
int deviceCount = 0;
bool scanRunning = false;

// Target patterns to look for
String chigeePatterns[] = {
    "CG_OBD",
    "CHIGEE", 
    "OBD",
    "CG-",
    "MFP",
    "CGRC",
    "ELM327"
};

// Function declarations
void printOwnDeviceInfo();
void printDiscoveredDevices();
void connectToDevice(String macAddress);
void printHelp();
void startScan();
void stopScan();
void handleGapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

void setup() {
    Serial.begin(115200);
    Serial.println("=== CG OBD Protocol Scanner ===");
    Serial.println("Scanning for Chigee-compatible devices...");
    
    // Initialize Bluetooth
    if (!SerialBT.begin("OBD_Scanner")) {
        Serial.println("Bluetooth initialization failed!");
        return;
    }
    
    Serial.println("Bluetooth initialized successfully");
    
    // Register GAP callback for device discovery
    esp_bt_gap_register_callback(handleGapCallback);
    
    // Print our own device info for reference
    printOwnDeviceInfo();
    
    // Start scan
    startScan();
    
    printHelp();
}

void loop() {
    // Print discovered devices every 15 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 15000) {
        lastPrint = millis();
        if (deviceCount > 0) {
            printDiscoveredDevices();
        } else {
            Serial.println("No devices discovered yet...");
        }
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "scan" || command == "start") {
            Serial.println("Starting scan...");
            startScan();
        } else if (command == "stop") {
            Serial.println("Stopping scan...");
            stopScan();
        } else if (command == "print" || command == "list") {
            printDiscoveredDevices();
        } else if (command == "clear") {
            deviceCount = 0;
            Serial.println("Device list cleared");
        } else if (command.startsWith("connect ")) {
            String targetMac = command.substring(8);
            connectToDevice(targetMac);
        } else if (command == "help" || command == "?") {
            printHelp();
        } else if (command == "status") {
            Serial.println("Scan status: " + String(scanRunning ? "RUNNING" : "STOPPED"));
            Serial.println("Devices found: " + String(deviceCount));
        } else {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
    
    delay(100);
}

void handleGapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT: {
            esp_bt_gap_dev_prop_t *props = param->disc_res.prop;
            esp_bd_addr_t *bda = &param->disc_res.bda;
            
            // Convert MAC address to string
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    (*bda)[0], (*bda)[1], (*bda)[2], (*bda)[3], (*bda)[4], (*bda)[5]);
            
            // Check if we already have this device
            String currentAddr = String(macStr);
            bool alreadyExists = false;
            for (int i = 0; i < deviceCount; i++) {
                if (discoveredDevices[i].address == currentAddr) {
                    alreadyExists = true;
                    break;
                }
            }
            
            if (alreadyExists || deviceCount >= 50) return;
            
            // Store device info
            BTDevice& dev = discoveredDevices[deviceCount];
            dev.address = currentAddr;
            dev.name = "Unknown";
            dev.rssi = param->disc_res.rssi;
            dev.cod = param->disc_res.cod;
            dev.hasName = false;
            
            // Parse properties
            for (int i = 0; i < param->disc_res.num_prop; i++) {
                switch (props[i].type) {
                    case ESP_BT_GAP_DEV_PROP_BDNAME:
                        dev.name = String((char*)props[i].val);
                        dev.hasName = true;
                        break;
                    case ESP_BT_GAP_DEV_PROP_COD:
                        dev.cod = *(uint32_t*)props[i].val;
                        break;
                    case ESP_BT_GAP_DEV_PROP_RSSI:
                        dev.rssi = *(int8_t*)props[i].val;
                        break;
                }
            }
            
            // Create raw data string for analysis
            dev.rawData = "COD:0x" + String(dev.cod, HEX);
            
            deviceCount++;
            
            // Check if this looks like a Chigee device
            bool isChigeeCandidate = false;
            String upperName = dev.name;
            upperName.toUpperCase();
            
            for (const String& pattern : chigeePatterns) {
                if (upperName.indexOf(pattern) >= 0) {
                    isChigeeCandidate = true;
                    break;
                }
            }
            
            // Print immediately if it's a candidate
            if (isChigeeCandidate) {
                Serial.println("\n🎯 CHIGEE CANDIDATE FOUND!");
                Serial.println("Name: " + dev.name);
                Serial.println("MAC: " + dev.address);
                Serial.println("RSSI: " + String(dev.rssi) + " dBm");
                Serial.println("COD: 0x" + String(dev.cod, HEX));
                Serial.println("Data: " + dev.rawData);
                Serial.println("---");
            } else {
                Serial.print(".");  // Progress indicator
            }
            break;
        }
        
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                Serial.println("\nDiscovery stopped");
                scanRunning = false;
            } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
                Serial.println("Discovery started");
                scanRunning = true;
            }
            break;
            
        default:
            break;
    }
}

void startScan() {
    if (scanRunning) {
        Serial.println("Scan already running");
        return;
    }
    
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 30, 0);
}

void stopScan() {
    if (!scanRunning) {
        Serial.println("No scan running");
        return;
    }
    
    esp_bt_gap_cancel_discovery();
}

void printOwnDeviceInfo() {
    Serial.println("\n=== Our Device Info ===");
    const uint8_t* mac = esp_bt_dev_get_address();
    Serial.printf("Our MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println("Our Name: OBD_Scanner");
    Serial.println("========================\n");
}

void printDiscoveredDevices() {
    Serial.println("\n=== Discovered Devices ===");
    Serial.println("Total devices found: " + String(deviceCount));
    
    if (deviceCount == 0) {
        Serial.println("No devices discovered yet...");
        return;
    }
    
    for (int i = 0; i < deviceCount; i++) {
        BTDevice& dev = discoveredDevices[i];
        Serial.println("\nDevice " + String(i + 1) + ":");
        Serial.println("  Name: " + (dev.name.length() > 0 ? dev.name : "Unknown"));
        Serial.println("  MAC: " + dev.address);
        Serial.println("  RSSI: " + String(dev.rssi) + " dBm");
        Serial.println("  COD: 0x" + String(dev.cod, HEX));
        Serial.println("  Raw: " + dev.rawData);
        
        // Check if it matches Chigee patterns
        String upperName = dev.name;
        upperName.toUpperCase();
        for (const String& pattern : chigeePatterns) {
            if (upperName.indexOf(pattern) >= 0) {
                Serial.println("  ⭐ MATCHES PATTERN: " + pattern);
                break;
            }
        }
    }
    Serial.println("==========================\n");
}

void connectToDevice(String macAddress) {
    Serial.println("Attempting to connect to: " + macAddress);
    
    // Stop discovery first
    stopScan();
    delay(1000);
    
    // Parse MAC address
    uint8_t mac[6];
    sscanf(macAddress.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    // Try to connect
    if (SerialBT.connect(mac)) {
        Serial.println("✅ Connected successfully!");
        Serial.println("Analyzing connection...");
        
        // Try to receive data
        unsigned long startTime = millis();
        while (millis() - startTime < 10000) {  // Listen for 10 seconds
            if (SerialBT.available()) {
                String receivedData = SerialBT.readString();
                Serial.println("📥 Received: " + receivedData);
            }
            
            // Send some test commands
            if (millis() - startTime == 2000) {
                Serial.println("📤 Sending: ATZ");
                SerialBT.println("ATZ\r");
            } else if (millis() - startTime == 4000) {
                Serial.println("📤 Sending: 010C");
                SerialBT.println("010C\r");
            }
            
            delay(100);
        }
        
        SerialBT.disconnect();
        Serial.println("Disconnected");
    } else {
        Serial.println("❌ Connection failed");
    }
    
    // Resume scanning
    delay(1000);
    startScan();
}

void printHelp() {
    Serial.println("\n=== Available Commands ===");
    Serial.println("scan/start - Start device discovery");
    Serial.println("stop - Stop device discovery");
    Serial.println("print/list - Print all discovered devices");
    Serial.println("clear - Clear device list");
    Serial.println("connect <MAC> - Connect to specific device (format: AA:BB:CC:DD:EE:FF)");
    Serial.println("status - Show scan status and device count");
    Serial.println("help/? - Show this help");
    Serial.println("==========================\n");
}