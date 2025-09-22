/**
 * Clean XR-2 Compatible OBD Server for ESP32
 * Testing standard OBD service UUID and immediate binary responses
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Service UUID - Trying standard OBD service UUID
#define SERVICE_UUID "0000aaa1-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "00002222-0000-1000-8000-00805f9b34fb"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 XR-2 connected!");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 XR-2 disconnected!");
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

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting XR-2 Compatible OBD Server...");

    // Create BLE Device with XR-2 expected name
    BLEDevice::init("CGOBD-5F72");

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service with standard OBD UUID
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

    // Start advertising with OBD service UUID
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    
    Serial.println("✅ BLE OBD Server ready for XR-2!");
    Serial.println("📡 Service UUID: " + String(SERVICE_UUID));
    Serial.println("🔗 Device Name: CGOBD-5F72");
    Serial.println("⚡ Waiting for XR-2 connection...");
}

void loop() {
    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("🔄 Restarting advertising...");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    delay(100);
}