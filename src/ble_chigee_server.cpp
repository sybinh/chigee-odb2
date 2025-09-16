#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// OBD Service UUID (discovered from XR-2 scan!)
#define SERVICE_UUID        "30312d30-3030-302d-3261-616130303030"
// Try Nordic UART pattern for characteristics
#define CHAR_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Nordic UART RX
#define CHAR_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Nordic UART TX

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristicRX = nullptr;
BLECharacteristic* pCharacteristicTX = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("🎉 CLIENT CONNECTED to CGOBD-5F72!");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("❌ Client disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.println("📨 OBD COMMAND from XR-2:");
            Serial.print("   Raw: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
            Serial.print("   ASCII: ");
            for (int i = 0; i < rxValue.length(); i++) {
                char c = rxValue[i];
                Serial.print(c >= 32 && c <= 126 ? c : '.');
            }
            Serial.println();
            
            // Parse OBD command and respond
            String command = String(rxValue.c_str());
            command.trim();
            String response = processOBDCommand(command);
            
            // Send response via TX characteristic
            pCharacteristicTX->setValue(response.c_str());
            pCharacteristicTX->notify();
            Serial.println("📤 SENT OBD response: " + response);
            Serial.println();
        }
    }
    
    void onRead(BLECharacteristic *pCharacteristic) {
        Serial.println("📖 XR-2 READ request received");
    }
    
private:
    String processOBDCommand(String cmd) {
        cmd.toUpperCase();
        Serial.println("🔍 Processing: " + cmd);
        
        // ELM327 initialization sequence
        if (cmd.indexOf("ATZ") >= 0) return "ELM327 v2.1\r\n>";
        if (cmd.indexOf("ATE0") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATE1") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATL0") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATL1") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATS0") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATS1") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATH0") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATH1") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATSP") >= 0) return "OK\r\n>";
        if (cmd.indexOf("ATDP") >= 0) return "AUTO\r\n>";
        if (cmd.indexOf("ATRV") >= 0) return "12.8V\r\n>";
        
        // Supported PIDs
        if (cmd.indexOf("0100") >= 0) return "41 00 BE 3E B8 11\r\n>";
        if (cmd.indexOf("0120") >= 0) return "41 20 80 07 65 15\r\n>";
        if (cmd.indexOf("0140") >= 0) return "41 40 48 00 00 10\r\n>";
        
        // OBD PIDs for XR-2 dashboard  
        if (cmd.indexOf("010C") >= 0) return "41 0C 1A F8\r\n>";      // RPM: 1726
        if (cmd.indexOf("010D") >= 0) return "41 0D 3C\r\n>";         // Speed: 60 km/h
        if (cmd.indexOf("0105") >= 0) return "41 05 5A\r\n>";         // Engine Temp: 50°C
        if (cmd.indexOf("0142") >= 0) return "41 42 34 80\r\n>";      // Battery: 13.2V
        if (cmd.indexOf("010B") >= 0) return "41 0B 63\r\n>";         // MAP: 99 kPa
        if (cmd.indexOf("0114") >= 0) return "41 14 80\r\n>";         // Fuel Trim: 0%
        if (cmd.indexOf("0110") >= 0) return "41 10 45\r\n>";         // MAF: 6.9 g/s
        if (cmd.indexOf("010F") >= 0) return "41 0F 48\r\n>";         // IAT: 32°C
        if (cmd.indexOf("013C") >= 0) return "41 3C 04 B0\r\n>";      // Catalyst: 120°C
        if (cmd.indexOf("03") >= 0) return "43 00\r\n>";              // No DTCs
        
        // Default response
        return "NO DATA\r\n>";
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== BLE CHIGEE OBD Server ===");
    Serial.println("Device Name: CGOBD-5F72");
    Serial.println("Protocol: BLE (not Classic BT)");
    Serial.println("Service UUID: " + String(SERVICE_UUID));
    Serial.println("RX Characteristic UUID: " + String(CHAR_RX_UUID));
    Serial.println("TX Characteristic UUID: " + String(CHAR_TX_UUID));
    Serial.println("===============================");
    Serial.println();

    // Create the BLE Device
    BLEDevice::init("CGOBD-5F72");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create RX Characteristic (XR-2 writes to ESP32)
    pCharacteristicRX = pService->createCharacteristic(
                         CHAR_RX_UUID,
                         BLECharacteristic::PROPERTY_WRITE |
                         BLECharacteristic::PROPERTY_WRITE_NR
                       );
    pCharacteristicRX->setCallbacks(new MyCallbacks());

    // Create TX Characteristic (ESP32 sends to XR-2)  
    pCharacteristicTX = pService->createCharacteristic(
                         CHAR_TX_UUID,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_NOTIFY |
                         BLECharacteristic::PROPERTY_INDICATE
                       );

    // Add descriptor for TX notifications
    pCharacteristicTX->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    
    Serial.println("✅ BLE server started and advertising");
    Serial.println("📡 Waiting for XR-2 connections...");
    Serial.println();
    Serial.println("Instructions:");
    Serial.println("1. Go to XR-2 Bluetooth settings");
    Serial.println("2. Search for 'CGOBD-5F72' (BLE device)");
    Serial.println("3. Try to pair/connect");
    Serial.println("4. Watch this monitor for results");
    Serial.println();
}

void loop() {
    // Restart advertising if disconnected
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("🔄 Restarting BLE advertising...");
        oldDeviceConnected = deviceConnected;
    }
    
    // Track connection state
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Send test OBD data every 5 seconds if connected
    static unsigned long lastSend = 0;
    if (deviceConnected && millis() - lastSend > 5000) {
        // Simulate OBD data
        String obdData = "41 0C 1A F8\r\n>"; // Example: RPM = 1726
        pCharacteristicTX->setValue(obdData.c_str());
        pCharacteristicTX->notify();
        Serial.println("📤 Sent OBD data: " + obdData);
        lastSend = millis();
    }

    delay(100);
}