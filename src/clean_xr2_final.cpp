/**
 * Clean XR-2 Compatible OBD Server for ESP32
 * Testing CUSTOM UUID from firmware analysis to compare behavior
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Service UUID - Using custom UUID discovered from XR-2 firmware analysis
#define SERVICE_UUID "30312d30-3030-302d-3261-616130303030"
#define CHARACTERISTIC_UUID "30312d31-3030-302d-3261-616130303030"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// 🔄 Continuous streaming variables (from firmware analysis)
unsigned long lastDataUpdate = 0;
unsigned long lastCoreData = 0;
const unsigned long CORE_DATA_INTERVAL = 150;  // 150ms for RPM, Speed (firmware: 100-200ms)
const unsigned long EXTENDED_DATA_INTERVAL = 750;  // 750ms for other data (firmware: 500-1000ms)
int dataSequence = 0;  // Rotate through different data types

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 XR-2 connected! (CUSTOM UUID)");
        
        // 🔑 PROACTIVE DATA PUSH - Key pattern from firmware analysis!
        Serial.println("🚀 Starting proactive data push (BINARY FORMAT)...");
        delay(500);
        
        // Send supported PIDs first (BINARY FORMAT)
        uint8_t supportedPIDs[] = {0x41, 0x00, 0xBE, 0x3E, 0xB8, 0x11};
        pCharacteristic->setValue(supportedPIDs, 6);
        pCharacteristic->notify();
        Serial.println("📤 Sent supported PIDs (binary)");
        
        delay(200);
        
        // Send live OBD data (BINARY FORMAT)
        uint8_t rpmData[] = {0x41, 0x0C, 0x1A, 0xF8};  // ~1720 RPM
        pCharacteristic->setValue(rpmData, 4);
        pCharacteristic->notify();
        Serial.println("📤 Sent RPM data (binary)");
        
        delay(100);
        
        uint8_t speedData[] = {0x41, 0x0D, 0x3C};    // 60 km/h
        pCharacteristic->setValue(speedData, 3);
        pCharacteristic->notify();
        Serial.println("📤 Sent speed data (binary)");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 XR-2 disconnected! (CUSTOM UUID)");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        String cmd = String(value.c_str());
        
        Serial.println("📨 Received: " + cmd);
        
        // Simplified responses for XR-2
        if (cmd.startsWith("AT")) {
            pCharacteristic->setValue("OK\r>");
            pCharacteristic->notify();
            Serial.println("📤 AT Response: OK");
        }
        else if (cmd == "0100") {
            // Immediate binary response for supported PIDs
            uint8_t response[] = {0x41, 0x00, 0xFE, 0x3F, 0xF8, 0x11};
            pCharacteristic->setValue(response, 6);
            pCharacteristic->notify();
            Serial.println("📤 PID 0100: Binary supported PIDs sent");
        }
        else if (cmd == "010C") {
            // RPM response
            uint8_t response[] = {0x41, 0x0C, 0x1A, 0xF8};
            pCharacteristic->setValue(response, 4);
            pCharacteristic->notify();
            Serial.println("📤 PID 010C: RPM sent");
        }
        else if (cmd == "010D") {
            // Speed response
            uint8_t response[] = {0x41, 0x0D, 0x3C};
            pCharacteristic->setValue(response, 3);
            pCharacteristic->notify();
            Serial.println("📤 PID 010D: Speed sent");
        }
        else {
            pCharacteristic->setValue("?\r>");
            pCharacteristic->notify();
        }
    }
};

// 🔄 Continuous Data Streaming Functions (from firmware analysis)
void sendCoreOBDData() {
    if (!deviceConnected || !pCharacteristic) return;
    
    // Rotate through core data types every call
    switch(dataSequence % 3) {
        case 0: {
            // RPM data (simulate varying RPM)
            static int rpmCounter = 0;
            int baseRpm = 1500 + (rpmCounter % 500);  // 1500-2000 RPM
            uint8_t rpmResponse[] = {0x41, 0x0C, (uint8_t)(baseRpm >> 8), (uint8_t)(baseRpm & 0xFF)};
            pCharacteristic->setValue(rpmResponse, 4);
            pCharacteristic->notify();
            Serial.printf("📤 [STREAM] RPM: %d\n", baseRpm);
            rpmCounter++;
            break;
        }
        case 1: {
            // Speed data (simulate varying speed)
            static int speedCounter = 0;
            int speed = 50 + (speedCounter % 30);  // 50-80 km/h
            uint8_t speedResponse[] = {0x41, 0x0D, (uint8_t)speed};
            pCharacteristic->setValue(speedResponse, 3);
            pCharacteristic->notify();
            Serial.printf("📤 [STREAM] Speed: %d km/h\n", speed);
            speedCounter++;
            break;
        }
        case 2: {
            // Engine temperature
            uint8_t tempResponse[] = {0x41, 0x05, 0x5A};  // 50°C
            pCharacteristic->setValue(tempResponse, 3);
            pCharacteristic->notify();
            Serial.println("📤 [STREAM] Engine Temp: 50°C");
            break;
        }
    }
    dataSequence++;
}

void sendExtendedOBDData() {
    if (!deviceConnected || !pCharacteristic) return;
    
    // Send supported PIDs periodically to remind XR-2 of capabilities (BINARY FORMAT)
    uint8_t supportedPIDs[] = {0x41, 0x00, 0xBE, 0x3E, 0xB8, 0x11};
    pCharacteristic->setValue(supportedPIDs, 6);
    pCharacteristic->notify();
    Serial.println("📤 [STREAM] Supported PIDs refresh (binary)");
}

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting XR-2 Compatible OBD Server...");

    // Create BLE Device with XR-2 expected name
    BLEDevice::init("CGOBD-5F72");

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service with CUSTOM UUID from firmware analysis
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                      );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    // Start service
    pService->start();

    // Start advertising with CUSTOM service UUID from XR-2 firmware
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    
    Serial.println("✅ BLE OBD Server ready for XR-2! (CUSTOM UUID)");
    Serial.println("📡 Service UUID: " + String(SERVICE_UUID));
    Serial.println("🔗 Device Name: CGOBD-5F72");
    Serial.println("⚡ Waiting for XR-2 connection...");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("🔄 Restarting advertising...");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        // Reset timing when newly connected
        lastCoreData = currentTime;
        lastDataUpdate = currentTime;
        dataSequence = 0;
        Serial.println("🔄 Started continuous data streaming...");
    }
    
    // 🔄 CONTINUOUS DATA STREAMING (Key XR-2 expectation!)
    if (deviceConnected) {
        // Core data every 150ms (RPM, Speed, Temp)
        if (currentTime - lastCoreData >= CORE_DATA_INTERVAL) {
            sendCoreOBDData();
            lastCoreData = currentTime;
        }
        
        // Extended data every 750ms (Supported PIDs, etc.)
        if (currentTime - lastDataUpdate >= EXTENDED_DATA_INTERVAL) {
            sendExtendedOBDData();
            lastDataUpdate = currentTime;
        }
    }
    
    delay(10);  // Small delay to prevent overwhelming
}