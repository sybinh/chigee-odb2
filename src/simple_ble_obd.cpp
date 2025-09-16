#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Try single characteristic approach with exact service UUID
#define SERVICE_UUID        "30312d30-3030-302d-3261-616130303030"
#define CHARACTERISTIC_UUID "30312d31-3030-302d-3261-616130303030"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("🎉 XR-2 CONNECTED!");
        
        // Send structured OBD frame instead of individual PIDs
        delay(500);
        
        // Try complete OBD response frame
        String obdFrame = "41 00 BE 3E B8 11 41 0C 1A F8 41 0D 3C 41 05 5A 41 42 34 80 41 0B 63";
        pCharacteristic->setValue(obdFrame.c_str());
        pCharacteristic->notify();
        Serial.println("📤 Sent complete OBD frame: " + obdFrame);
        
        // Also try just supported PIDs response
        delay(200);
        pCharacteristic->setValue("41 00 BE 3E B8 11");  // Supported PIDs 01-20
        pCharacteristic->notify();
        Serial.println("📤 Sent supported PIDs");
        
        // Then send live data frame
        delay(200);  
        String liveData = "41 0C 1A F8 41 0D 3C 41 05 5A";  // RPM + Speed + Temp
        pCharacteristic->setValue(liveData.c_str());
        pCharacteristic->notify();
        Serial.println("📤 Sent live data frame");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("❌ XR-2 disconnected");
        // Restart advertising
        BLEDevice::startAdvertising();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.println("\n📨 COMMAND from XR-2:");
            Serial.print("   Hex: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
            Serial.print("   Text: '");
            for (int i = 0; i < rxValue.length(); i++) {
                char c = rxValue[i];
                if (c >= 32 && c <= 126) Serial.print(c);
                else Serial.print(".");
            }
            Serial.println("'");
            
            // Process and respond
            String cmd = String(rxValue.c_str());
            cmd.trim();
            cmd.toUpperCase();
            String response = processOBD(cmd);
            
            // Send response
            pCharacteristic->setValue(response.c_str());
            pCharacteristic->notify();
            Serial.println("📤 Response: '" + response + "'");
            Serial.println();
        }
    }
    
private:
    String processOBD(String cmd) {
        // Remove any control characters
        cmd.replace("\r", "");
        cmd.replace("\n", "");
        
        Serial.println("🔍 Processing: '" + cmd + "'");
        
        // ELM327 Commands
        if (cmd == "ATZ") return "ELM327 v2.1\r\n>";
        if (cmd == "ATE0") return "OK\r\n>";
        if (cmd == "ATE1") return "OK\r\n>";
        if (cmd == "ATL0") return "OK\r\n>";
        if (cmd == "ATH0") return "OK\r\n>";
        if (cmd == "ATH1") return "OK\r\n>";
        if (cmd == "ATS0") return "OK\r\n>";
        if (cmd == "ATSP0") return "OK\r\n>";
        if (cmd == "ATDP") return "AUTO\r\n>";
        if (cmd == "ATRV") return "12.8V\r\n>";
        
        // OBD PIDs
        if (cmd == "0100") return "41 00 BE 3E B8 11\r\n>";  // Supported PIDs
        if (cmd == "010C") return "41 0C 1A F8\r\n>";        // RPM: 1726
        if (cmd == "010D") return "41 0D 3C\r\n>";           // Speed: 60 km/h
        if (cmd == "0105") return "41 05 5A\r\n>";           // Engine Temp: 50°C
        if (cmd == "0142") return "41 42 34 80\r\n>";        // Battery: 13.2V
        if (cmd == "010B") return "41 0B 63\r\n>";           // MAP: 99 kPa
        if (cmd == "0114") return "41 14 80\r\n>";           // Fuel Trim: 0%
        if (cmd == "0110") return "41 10 45\r\n>";           // MAF: 6.9 g/s
        if (cmd == "010F") return "41 0F 48\r\n>";           // IAT: 32°C
        if (cmd == "013C") return "41 3C 04 B0\r\n>";        // Catalyst: 120°C
        if (cmd == "03") return "43 00\r\n>";                // No DTCs
        
        return "NO DATA\r\n>";
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for serial monitor
    
    // Clear any boot messages
    Serial.println();
    Serial.println("================================");
    Serial.println("🚀 ESP32 BLE OBD-II SERVER 🚀");
    Serial.println("================================");
    Serial.println("Target Device: Chigee XR-2");
    Serial.println("ESP32 Device Name: CGOBD-5F72");
    Serial.println("BLE Service UUID: " + String(SERVICE_UUID));
    Serial.println("Characteristic UUID: " + String(CHARACTERISTIC_UUID));
    Serial.println("Protocol: ELM327 over BLE");
    Serial.println("Status: Initializing...");
    Serial.println("================================");
    Serial.println();

    // Initialize BLE
    BLEDevice::init("CGOBD-5F72");
    
    // Create server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create service  
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create single characteristic with all properties
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    // Start service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();

    Serial.println("✅ BLE Server started!");
    Serial.println("📡 Advertising as CGOBD-5F72...");
    Serial.println("⏳ Waiting for XR-2 connection...");
    Serial.println("📋 Supported OBD PIDs:");
    Serial.println("   • 010C: RPM");
    Serial.println("   • 010D: Vehicle Speed");
    Serial.println("   • 0105: Engine Temperature");
    Serial.println("   • 0142: Battery Voltage");
    Serial.println("   • 010B: MAP Pressure");
    Serial.println("   • 0114: Fuel Trim");
    Serial.println("   • 0110: MAF Rate");
    Serial.println("   • 010F: Intake Air Temp");
    Serial.println("   • 013C: Catalyst Temp");
    Serial.println("   • 03: Diagnostic Trouble Codes");
    Serial.println("================================");
    Serial.println();
}

void loop() {
    // Continuously update characteristic with fresh OBD data
    if (deviceConnected) {
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 2000) {  // Update every 2 seconds
            
            // Create comprehensive OBD data that XR-2 can read anytime
            String obdData = "41 0C 1A F8 41 0D 3C 41 05 5A 41 42 34 80 41 0B 63";
            
            // Set characteristic value (XR-2 will read this)
            pCharacteristic->setValue(obdData.c_str());
            
            // Also send notification for real-time updates
            pCharacteristic->notify();
            
            Serial.println("📤 Updated OBD data: " + obdData);
            Serial.println("   💡 XR-2 can now READ this data anytime!");
            
            lastUpdate = millis();
        }
        
        // Connection status
        static unsigned long lastHeartbeat = 0;
        if (millis() - lastHeartbeat > 15000) {
            Serial.println("💓 Connection alive - OBD data continuously available");
            lastHeartbeat = millis();
        }
    }
    
    delay(100);
}