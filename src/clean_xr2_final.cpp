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
        
        // 🔑 FIRMWARE-BASED BINARY DATA PUSH
        Serial.println("🚀 Starting firmware-matched binary data push...");
        delay(500);
        
        // Send supported PIDs first (BINARY FORMAT - firmware confirmed)
        uint8_t supported_pids[] = {0x41, 0x00, 0xBE, 0x3E, 0xB8, 0x11};
        pCharacteristic->setValue(supported_pids, sizeof(supported_pids));
        pCharacteristic->notify();
        Serial.println("📤 Sent supported PIDs (binary)");
        
        delay(200);
        
        // Send live OBD data (BINARY FORMAT matching firmware patterns)
        uint8_t rpm_data[] = {0x41, 0x0C, 0x1A, 0xF8};  // RPM ~1720
        pCharacteristic->setValue(rpm_data, sizeof(rpm_data));
        pCharacteristic->notify();
        Serial.println("📤 Sent RPM data (binary)");
        
        delay(100);
        
        uint8_t speed_data[] = {0x41, 0x0D, 0x3C};  // Speed 60 km/h
        pCharacteristic->setValue(speed_data, sizeof(speed_data));
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
            // RPM response (firmware-matched pattern)
            uint8_t response[] = {0x41, 0x0C, 0x1A, 0xF8};
            pCharacteristic->setValue(response, sizeof(response));
            pCharacteristic->notify();
            Serial.println("📤 PID 010C: RPM sent (binary)");
        }
        else if (cmd == "010D") {
            // Speed response (firmware-matched pattern)
            uint8_t response[] = {0x41, 0x0D, 0x3C};
            pCharacteristic->setValue(response, sizeof(response));
            pCharacteristic->notify();
            Serial.println("📤 PID 010D: Speed sent (binary)");
        }
        else if (cmd == "0105") {
            // Temperature response (firmware-matched pattern)
            uint8_t response[] = {0x41, 0x05, 0x5A};  // 90°C
            pCharacteristic->setValue(response, sizeof(response));
            pCharacteristic->notify();
            Serial.println("📤 PID 0105: Temperature sent (binary)");
        }
        else if (cmd == "0111") {
            // Throttle response (firmware-matched pattern)
            uint8_t response[] = {0x41, 0x11, 0x4D};  // ~30%
            pCharacteristic->setValue(response, sizeof(response));
            pCharacteristic->notify();
            Serial.println("📤 PID 0111: Throttle sent (binary)");
        }
        else {
            pCharacteristic->setValue("?\r>");
            pCharacteristic->notify();
        }
    }
};

// 🔄 Continuous Data Streaming Functions (firmware-matched binary format)
void sendCoreOBDData() {
    if (!deviceConnected || !pCharacteristic) return;
    
    // Rotate through core data types every call
    switch(dataSequence % 3) {
        case 0: {
            // RPM data (simulate varying RPM) - BINARY FORMAT
            static int rpmCounter = 0;
            int baseRpm = 1500 + (rpmCounter % 500);  // 1500-2000 RPM
            // OBD PID 0x0C: RPM = ((A*256)+B)/4, so we need to send RPM*4
            int obd_rpm_value = baseRpm * 4;
            uint8_t rpm_frame[] = {
                0x41, 0x0C,
                (uint8_t)((obd_rpm_value >> 8) & 0xFF),
                (uint8_t)(obd_rpm_value & 0xFF)
            };
            pCharacteristic->setValue(rpm_frame, sizeof(rpm_frame));
            pCharacteristic->notify();
            Serial.printf("📤 [STREAM] RPM: %d (binary: %02X %02X %02X %02X)\n", 
                         baseRpm, rpm_frame[0], rpm_frame[1], rpm_frame[2], rpm_frame[3]);
            rpmCounter++;
            break;
        }
        case 1: {
            // Speed data (simulate varying speed) - BINARY FORMAT
            static int speedCounter = 0;
            int speed = 50 + (speedCounter % 30);  // 50-80 km/h
            uint8_t speed_frame[] = {0x41, 0x0D, (uint8_t)speed};
            pCharacteristic->setValue(speed_frame, sizeof(speed_frame));
            pCharacteristic->notify();
            Serial.printf("📤 [STREAM] Speed: %d km/h (binary: %02X %02X %02X)\n", 
                         speed, speed_frame[0], speed_frame[1], speed_frame[2]);
            speedCounter++;
            break;
        }
        case 2: {
            // Engine temperature - STRING FORMAT
            String tempResponse = "41 05 5A\r\n>";  // 50°C
            pCharacteristic->setValue(tempResponse.c_str());
            pCharacteristic->notify();
            Serial.println("📤 [STREAM] Engine Temp: 50°C (response: 41 05 5A)");
            break;
        }
    }
    dataSequence++;
}

void sendExtendedOBDData() {
    if (!deviceConnected || !pCharacteristic) return;
    
    // Send supported PIDs periodically to remind XR-2 of capabilities (BINARY FORMAT)
    uint8_t supported_pids[] = {0x41, 0x00, 0xBE, 0x3E, 0xB8, 0x11};
    pCharacteristic->setValue(supported_pids, sizeof(supported_pids));
    pCharacteristic->notify();
    Serial.println("📤 [STREAM] Supported PIDs refresh (binary)");
}

// 🔄 Status data streaming function (firmware-matched binary format)
void sendPeriodicStatus() {
    if (!deviceConnected || !pCharacteristic) return;
    
    static int statusSequence = 0;
    
    // Send status data based on firmware patterns (binary format)
    switch(statusSequence % 4) {
        case 0: {
            // Coolant temperature - BINARY FORMAT
            int temp = 80 + (millis() / 10000) % 20;  // 80-100°C
            uint8_t temp_frame[] = {0x41, 0x05, (uint8_t)(temp + 40)};  // +40 offset per OBD spec
            pCharacteristic->setValue(temp_frame, sizeof(temp_frame));
            pCharacteristic->notify();
            Serial.printf("📤 [STATUS] Coolant: %d°C (binary: %02X %02X %02X)\n", 
                         temp, temp_frame[0], temp_frame[1], temp_frame[2]);
            break;
        }
        case 1: {
            // Intake air temperature - BINARY FORMAT
            int airTemp = 25 + (millis() / 15000) % 15;  // 25-40°C
            uint8_t air_frame[] = {0x41, 0x0F, (uint8_t)(airTemp + 40)};  // +40 offset
            pCharacteristic->setValue(air_frame, sizeof(air_frame));
            pCharacteristic->notify();
            Serial.printf("📤 [STATUS] Air temp: %d°C (binary: %02X %02X %02X)\n", 
                         airTemp, air_frame[0], air_frame[1], air_frame[2]);
            break;
        }
        case 2: {
            // Throttle position - BINARY FORMAT
            int throttle = 15 + (millis() / 8000) % 25;  // 15-40%
            uint8_t throttle_frame[] = {0x41, 0x11, (uint8_t)((throttle * 255) / 100)};
            pCharacteristic->setValue(throttle_frame, sizeof(throttle_frame));
            pCharacteristic->notify();
            Serial.printf("📤 [STATUS] Throttle: %d%% (binary: %02X %02X %02X)\n", 
                         throttle, throttle_frame[0], throttle_frame[1], throttle_frame[2]);
            break;
        }
        case 3: {
            // Fuel level - BINARY FORMAT
            int fuel = 40 + (millis() / 30000) % 50;  // 40-90%
            uint8_t fuel_frame[] = {0x41, 0x2F, (uint8_t)((fuel * 255) / 100)};
            pCharacteristic->setValue(fuel_frame, sizeof(fuel_frame));
            pCharacteristic->notify();
            Serial.printf("📤 [STATUS] Fuel: %d%% (binary: %02X %02X %02X)\n", 
                         fuel, fuel_frame[0], fuel_frame[1], fuel_frame[2]);
            break;
        }
    }
    statusSequence++;
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
        // Core data every 150ms (RPM, Speed, Engine Load)
        if (currentTime - lastCoreData >= CORE_DATA_INTERVAL) {
            sendCoreOBDData();
            lastCoreData = currentTime;
        }
        
        // Extended data every 750ms (Supported PIDs, Status data)
        if (currentTime - lastDataUpdate >= EXTENDED_DATA_INTERVAL) {
            sendExtendedOBDData();
            sendPeriodicStatus();  // Add status data streaming
            lastDataUpdate = currentTime;
        }
    }
    
    delay(10);  // Small delay to prevent overwhelming
}