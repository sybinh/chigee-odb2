#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Service & Characteristic UUIDs from XR-2 firmware analysis
#define SERVICE_UUID        "0000aaa1-0000-1000-8000-00805f9b34fb" // Primary OBD Service 
#define CHARACTERISTIC_UUID "30312d30-3030-302d-3261-616130303030" // OBD Data Characteristic

// BLE globals
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// OBD data functions matching XR-2 firmware
void GetOBDRpm() {
    uint8_t frame[] = {0x41, 0x0C, 0x1A, 0xF8}; // RPM data 
    uint8_t checksum = 0;
    for (int i = 1; i < sizeof(frame); i++) checksum ^= frame[i];
    pCharacteristic->setValue(frame, sizeof(frame));
    pCharacteristic->notify();
    Serial.printf("📤 RPM Frame: ");
    for (int i = 0; i < sizeof(frame); i++) Serial.printf("%02X ", frame[i]);
    Serial.printf("| checksum: %02X\n", checksum);
}

void GetOBDSpeed() {
    uint8_t frame[] = {0x41, 0x0D, 0x3C}; // Speed data
    uint8_t checksum = 0; 
    for (int i = 1; i < sizeof(frame); i++) checksum ^= frame[i];
    pCharacteristic->setValue(frame, sizeof(frame));
    pCharacteristic->notify();
    Serial.printf("📤 Speed Frame: ");
    for (int i = 0; i < sizeof(frame); i++) Serial.printf("%02X ", frame[i]);
    Serial.printf("| checksum: %02X\n", checksum);
}

void GetOBDEngCoolDeg() {
    uint8_t frame[] = {0x41, 0x05, 0x5A}; // Coolant temp
    uint8_t checksum = 0;
    for (int i = 1; i < sizeof(frame); i++) checksum ^= frame[i];
    pCharacteristic->setValue(frame, sizeof(frame));
    pCharacteristic->notify();
    Serial.printf("📤 Coolant Frame: ");
    for (int i = 0; i < sizeof(frame); i++) Serial.printf("%02X ", frame[i]);
    Serial.printf("| checksum: %02X\n", checksum);
}

void GetOBDThrottle() {
    uint8_t frame[] = {0x41, 0x11, 0x4D}; // Throttle position
    uint8_t checksum = 0;
    for (int i = 1; i < sizeof(frame); i++) checksum ^= frame[i];
    pCharacteristic->setValue(frame, sizeof(frame));
    pCharacteristic->notify();
    Serial.printf("📤 Throttle Frame: ");
    for (int i = 0; i < sizeof(frame); i++) Serial.printf("%02X ", frame[i]);
    Serial.printf("| checksum: %02X\n", checksum);
}

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);

    // Create BLE Device
    BLEDevice::init("CHIGEE-OBD2");

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                    );

    pCharacteristic->addDescriptor(new BLE2902());

    // Start service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting for a client connection to notify...");
}

void loop() {
    // OBD data stream
    if (deviceConnected) {
        // Update BLE characteristic with OBD data
        GetOBDRpm();
        delay(250);
        GetOBDSpeed(); 
        delay(250);
        GetOBDEngCoolDeg();
        delay(250);
        GetOBDThrottle();
        delay(250);
    }
    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Device connected");
    }
}