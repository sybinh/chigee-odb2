#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BluetoothSerial.h>
#include <map>
#include <vector>

// RSSI tracking for device identification
std::map<String, int> deviceRSSI;
std::map<String, String> deviceNames;
std::map<String, int> deviceCounts;

BluetoothSerial SerialBT;
BLEScan* pBLEScan;
bool scanningBLE = true;

class XR2RSSICallback: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String address = advertisedDevice.getAddress().toString().c_str();
        String name = advertisedDevice.getName().c_str();
        int rssi = advertisedDevice.getRSSI();
        
        // Track device appearances and RSSI changes
        deviceCounts[address]++;
        deviceRSSI[address] = rssi;
        if (name.length() > 0) {
            deviceNames[address] = name;
        }
        
        // Look for potential XR-2 patterns
        bool isCandidate = false;
        String reason = "";
        
        // Check for strong signal (close device)
        if (rssi > -40) {
            isCandidate = true;
            reason += "STRONG_SIGNAL ";
        }
        
        // Check for XR-2 related names
        String nameLower = name;
        nameLower.toLowerCase();
        if (nameLower.indexOf("xr") >= 0 || nameLower.indexOf("chigee") >= 0 || 
            nameLower.indexOf("cgrc") >= 0 || nameLower.indexOf("obd") >= 0) {
            isCandidate = true;
            reason += "NAME_MATCH ";
        }
        
        // Check for common device patterns
        if (name.startsWith("ESP") || name.startsWith("Arduino") || 
            name.startsWith("BT-") || name.startsWith("HC-")) {
            reason += "COMMON_DEV ";
        }
        
        // Print candidates or strong signals
        if (isCandidate || rssi > -50 || deviceCounts[address] == 1) {
            Serial.println("=== DEVICE TRACKED ===");
            Serial.println("Address: " + address);
            Serial.println("Name: " + (name.length() > 0 ? name : "(Unknown)"));
            Serial.println("RSSI: " + String(rssi) + " dBm");
            Serial.println("Count: " + String(deviceCounts[address]));
            if (reason.length() > 0) {
                Serial.println("Flags: " + reason);
            }
            Serial.println("======================");
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("XR-2 RSSI Tracker Starting...");
    Serial.println("This scanner tracks signal strength changes");
    Serial.println("to help identify XR-2 when you turn it on/off");
    Serial.println("========================================");
    
    // Initialize BLE
    BLEDevice::init("ESP32-XR2-Tracker");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new XR2RSSICallback());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    // Initialize Classic Bluetooth
    if (!SerialBT.begin("ESP32-XR2-Tracker")) {
        Serial.println("Bluetooth Classic init failed!");
    }
    
    Serial.println("=== INSTRUCTIONS ===");
    Serial.println("1. Let scanner run for 30 seconds");
    Serial.println("2. Turn OFF XR-2 device");
    Serial.println("3. Wait 10 seconds");
    Serial.println("4. Turn ON XR-2 device");
    Serial.println("5. Look for devices that appear/disappear");
    Serial.println("====================");
}

void performBLEScan() {
    Serial.println("\n--- BLE Scan (10s) ---");
    BLEScanResults foundDevices = pBLEScan->start(10, false);
    Serial.println("BLE scan completed: " + String(foundDevices.getCount()) + " devices");
    pBLEScan->clearResults();
}

void performClassicScan() {
    Serial.println("\n--- Classic BT Scan ---");
    SerialBT.discoverAsync([](BTAdvertisedDevice* pDevice) {
        String address = pDevice->getAddress().toString().c_str();
        String name = pDevice->getName().c_str();
        
        Serial.println("=== CLASSIC BT DEVICE ===");
        Serial.println("Address: " + address);
        Serial.println("Name: " + (name.length() > 0 ? name : "(Unknown)"));
        Serial.println("RSSI: " + String(pDevice->getRSSI()) + " dBm");
        
        // Check for XR-2 patterns in Classic BT
        String nameLower = name;
        nameLower.toLowerCase();
        if (nameLower.indexOf("xr") >= 0 || nameLower.indexOf("chigee") >= 0 || 
            nameLower.indexOf("cgrc") >= 0) {
            Serial.println("*** POTENTIAL XR-2 MATCH! ***");
        }
        Serial.println("==========================");
        
        return true;
    });
    
    delay(15000); // Let Classic BT scan run
    SerialBT.discoverAsyncStop();
}

void printSummary() {
    Serial.println("\n=== DEVICE SUMMARY ===");
    Serial.println("Total unique devices: " + String(deviceRSSI.size()));
    
    // Sort by RSSI (strongest first)
    std::vector<std::pair<String, int>> sortedDevices;
    for (auto& pair : deviceRSSI) {
        sortedDevices.push_back({pair.first, pair.second});
    }
    
    std::sort(sortedDevices.begin(), sortedDevices.end(), 
              [](const std::pair<String, int>& a, const std::pair<String, int>& b) { 
                  return a.second > b.second; 
              });
    
    Serial.println("\nTop devices by signal strength:");
    for (int i = 0; i < min(10, (int)sortedDevices.size()); i++) {
        String addr = sortedDevices[i].first;
        int rssi = sortedDevices[i].second;
        String name = deviceNames.count(addr) ? deviceNames[addr] : "(Unknown)";
        int count = deviceCounts[addr];
        
        Serial.println(String(i+1) + ". " + addr + " | " + String(rssi) + "dBm | " + 
                      name + " (seen " + String(count) + "x)");
    }
    Serial.println("=====================");
}

void loop() {
    static unsigned long lastScan = 0;
    static int scanCycle = 0;
    
    if (millis() - lastScan > 20000) { // Every 20 seconds
        scanCycle++;
        Serial.println("\n========== SCAN CYCLE " + String(scanCycle) + " ==========");
        
        // Alternate between BLE and Classic BT
        if (scanCycle % 2 == 1) {
            performBLEScan();
        } else {
            performClassicScan();
        }
        
        printSummary();
        
        Serial.println("\nNow is a good time to turn XR-2 ON/OFF!");
        Serial.println("Watch for devices that appear/disappear...");
        
        lastScan = millis();
    }
    
    delay(1000);
}