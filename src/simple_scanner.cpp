/*
 * Simple Bluetooth Scanner for CG OBD Protocol Analysis
 * Scans for nearby Bluetooth devices and looks for Chigee patterns
 */

#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// Target patterns to look for
String chigeePatterns[] = {
    "CG_OBD",
    "CHIGEE", 
    "OBD",
    "CG-",
    "MFP",
    "CGRC",
    "ELM327",
    "OBDII"
};
const int numPatterns = 8;

struct BTDevice {
    String name;
    String address;
    bool found;
};

BTDevice foundDevices[20];
int deviceCount = 0;

// Function declarations
void startScan();
void processDevice(BTAdvertisedDevice device);
void listDevices();
void connectToDevice(int deviceNum);

void setup() {
    Serial.begin(115200);
    Serial.println("=== Simple CG OBD Scanner ===");
    Serial.println("Looking for OBD and Chigee devices...");
    
    if (!SerialBT.begin("OBD_Scanner")) {
        Serial.println("Bluetooth initialization failed!");
        return;
    }
    
    Serial.println("Bluetooth initialized");
    Serial.println("Starting scan...");
    
    // Start simple scan
    startScan();
    
    Serial.println("\nCommands:");
    Serial.println("scan - Start new scan");
    Serial.println("list - Show found devices");
    Serial.println("clear - Clear device list");
    Serial.println("connect <number> - Connect to device");
    Serial.println("help - Show commands");
}

void loop() {
    static unsigned long lastScan = 0;
    
    // Auto-scan every 30 seconds
    if (millis() - lastScan > 30000) {
        lastScan = millis();
        Serial.println("\n--- Auto-scanning ---");
        startScan();
    }
    
    // Handle commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "scan") {
            startScan();
        } else if (cmd == "list") {
            listDevices();
        } else if (cmd == "clear") {
            deviceCount = 0;
            Serial.println("Device list cleared");
        } else if (cmd.startsWith("connect ")) {
            int deviceNum = cmd.substring(8).toInt();
            connectToDevice(deviceNum);
        } else if (cmd == "help") {
            Serial.println("\nCommands:");
            Serial.println("scan - Start new scan");
            Serial.println("list - Show found devices"); 
            Serial.println("clear - Clear device list");
            Serial.println("connect <number> - Connect to device");
            Serial.println("help - Show commands");
        }
    }
    
    delay(100);
}

void startScan() {
    Serial.println("Scanning for Bluetooth devices...");
    
    // Simple discovery using callback
    SerialBT.discoverAsync([](BTAdvertisedDevice* device) {
        if (device) {
            processDevice(*device);
        }
    });
    
    Serial.println("Scan initiated... (will run for 30 seconds)");
}

void processDevice(BTAdvertisedDevice device) {
    String name = device.getName().c_str();
    String address = device.getAddress().toString().c_str();
    
    if (name.length() == 0) {
        name = "Unknown";
    }
    
    // Check if we already have this device
    for (int i = 0; i < deviceCount; i++) {
        if (foundDevices[i].address == address) {
            return; // Already found
        }
    }
    
    if (deviceCount >= 20) return; // List full
    
    // Check for Chigee patterns
    bool isTarget = false;
    String upperName = name;
    upperName.toUpperCase();
    
    for (int i = 0; i < numPatterns; i++) {
        if (upperName.indexOf(chigeePatterns[i]) >= 0) {
            isTarget = true;
            break;
        }
    }
    
    // Store all devices but highlight targets
    foundDevices[deviceCount].name = name;
    foundDevices[deviceCount].address = address;
    foundDevices[deviceCount].found = true;
    deviceCount++;
    
    if (isTarget) {
        Serial.println("\n🎯 TARGET FOUND!");
        Serial.println("Name: " + name);
        Serial.println("MAC: " + address);
        Serial.println("---");
    } else {
        Serial.print(".");
    }
}

void listDevices() {
    Serial.println("\n=== Found Devices ===");
    if (deviceCount == 0) {
        Serial.println("No devices found yet");
        return;
    }
    
    for (int i = 0; i < deviceCount; i++) {
        Serial.println(String(i + 1) + ". " + foundDevices[i].name + " (" + foundDevices[i].address + ")");
        
        // Check if it matches patterns
        String upperName = foundDevices[i].name;
        upperName.toUpperCase();
        for (int j = 0; j < numPatterns; j++) {
            if (upperName.indexOf(chigeePatterns[j]) >= 0) {
                Serial.println("   ⭐ MATCHES: " + chigeePatterns[j]);
                break;
            }
        }
    }
    Serial.println("====================");
}

void connectToDevice(int deviceNum) {
    if (deviceNum < 1 || deviceNum > deviceCount) {
        Serial.println("Invalid device number");
        return;
    }
    
    BTDevice& device = foundDevices[deviceNum - 1];
    Serial.println("Connecting to: " + device.name + " (" + device.address + ")");
    
    // Parse MAC address
    uint8_t mac[6];
    sscanf(device.address.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    if (SerialBT.connect(mac)) {
        Serial.println("✅ Connected!");
        
        // Test communication
        Serial.println("Testing communication...");
        
        // Send ATZ command
        SerialBT.print("ATZ\r");
        delay(1000);
        
        if (SerialBT.available()) {
            String response = SerialBT.readString();
            Serial.println("Response: " + response);
        } else {
            Serial.println("No response");
        }
        
        // Send OBD test
        SerialBT.print("010C\r");
        delay(1000);
        
        if (SerialBT.available()) {
            String response = SerialBT.readString();
            Serial.println("OBD Response: " + response);
        }
        
        SerialBT.disconnect();
        Serial.println("Disconnected");
    } else {
        Serial.println("❌ Connection failed");
    }
}