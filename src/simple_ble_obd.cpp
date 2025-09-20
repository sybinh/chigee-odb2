#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Service UUID discovered from XR-2 firmware analysis
#define SERVICE_UUID        "30312d30-3030-302d-3261-616130303030"
#define CHARACTERISTIC_UUID "30312d31-3030-302d-3261-616130303030"

// BLE Connection States (from firmware analysis)
enum BLE_CONN_STATE {
    BLE_STATE_NOT_CONN = 0,
    BLE_STATE_REG_SERVER,
    BLE_STATE_SEARCH_DEV, 
    BLE_STATE_CONN_DEV,
    BLE_STATE_RUN,
    BLE_STATE_DIS_CONN,
    BLE_STATE_DIS_CONN_STOP
};

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
BLE_CONN_STATE currentState = BLE_STATE_NOT_CONN;

// Enhanced callback class with firmware-based logging
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        currentState = BLE_STATE_CONN_DEV;
        
        // Log connection with firmware-style format
        Serial.println("🎉 BLE_STATE_CONN_DEV: XR-2 CONNECTED!");
        String clientAddress = pServer->getConnInfo().getAddress().toString().c_str();
        Serial.printf("📍 BLE_STATE_CONN_DEV: addr:[%s]\n", clientAddress.c_str());
        
        currentState = BLE_STATE_RUN;
        Serial.println("✅ BLE_STATE_RUN: Data connection established");
        
        // Send OBD data using discovered function patterns
        delay(500);
        sendOBDSupportedPIDs();
        delay(200);
        sendOBDLiveData();
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        currentState = BLE_STATE_DIS_CONN;
        
        // Log disconnection with firmware-style format
        String clientAddress = pServer->getConnInfo().getAddress().toString().c_str();
        Serial.printf("❌ BLE_STATE_DIS_CONN: addr:[%s]\n", clientAddress.c_str());
        
        currentState = BLE_STATE_DIS_CONN_STOP;
        Serial.println("� BLE_STATE_DIS_CONN_STOP: Restarting advertising");
        
        // Restart advertising for auto-reconnection
        BLEDevice::startAdvertising();
        currentState = BLE_STATE_SEARCH_DEV;
        Serial.println("🔍 BLE_STATE_SEARCH_DEV: Waiting for XR-2 connection");
    }

private:
    // Emulate GetOBDxxx() functions discovered in firmware
    void sendOBDSupportedPIDs() {
        String supportedPIDs = "41 00 BE 3E B8 11";  // PIDs 01-20 supported
        pCharacteristic->setValue(supportedPIDs.c_str());
        pCharacteristic->notify();
        Serial.printf("📤 GetOBDSupportedPIDs(): %s\n", supportedPIDs.c_str());
    }
    
    void sendOBDLiveData() {
        // Emulate multiple firmware OBD functions
        sendGetOBDRpm();
        delay(100);
        sendGetOBDSpeed(); 
        delay(100);
        sendGetOBDEngCoolDeg();
        delay(100);
        sendGetOBDThrottle();
    }
    
    void sendGetOBDRpm() {
        String rpm = "41 0C 1A F8";  // ~1720 RPM
        pCharacteristic->setValue(rpm.c_str());
        pCharacteristic->notify();
        Serial.printf("📤 GetOBDRpm(): %s\n", rpm.c_str());
    }
    
    void sendGetOBDSpeed() {
        String speed = "41 0D 3C";    // 60 km/h  
        pCharacteristic->setValue(speed.c_str());
        pCharacteristic->notify();
        Serial.printf("📤 GetOBDSpeed(): %s\n", speed.c_str());
    }
    
    void sendGetOBDEngCoolDeg() {
        String temp = "41 05 5A";     // 50°C coolant temp
        pCharacteristic->setValue(temp.c_str());
        pCharacteristic->notify();
        Serial.printf("📤 GetOBDEngCoolDeg(): %s\n", temp.c_str());
    }
    
    void sendGetOBDThrottle() {
        String throttle = "41 11 4D"; // ~30% throttle position
        pCharacteristic->setValue(throttle.c_str());
        pCharacteristic->notify();
        Serial.printf("📤 GetOBDThrottle(): %s\n", throttle.c_str());
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            // Enhanced logging with firmware-style format
            Serial.printf("\n📨 ble_cmd_parse(): Command from XR-2 (length: %d)\n", rxValue.length());
            Serial.print("   Raw Hex: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
            Serial.print("   ASCII: '");
            for (int i = 0; i < rxValue.length(); i++) {
                char c = rxValue[i];
                if (c >= 32 && c <= 126) Serial.print(c);
                else Serial.printf("\\x%02X", (uint8_t)c);
            }
            Serial.println("'");
            
            // Process OBD command using firmware-style validation
            String cmd = String(rxValue.c_str());
            cmd.trim();
            cmd.toUpperCase();
            
            // Validate command using IPC_OBDValidBit pattern
            if (isValidOBDCommand(cmd)) {
                String response = parseOBDData(cmd);
                
                // Send response with logging
                pCharacteristic->setValue(response.c_str());
                pCharacteristic->notify();
                Serial.printf("📤 obd_data_parse(): Response sent: '%s'\n", response.c_str());
            } else {
                Serial.printf("❌ IPC_OBDValidBit: Invalid command rejected: '%s'\n", cmd.c_str());
                pCharacteristic->setValue("NO DATA\r\n>");
                pCharacteristic->notify();
            }
            Serial.println();
        }
    }
    
private:
    // Emulate IPC_OBDValidBit validation from firmware
    bool isValidOBDCommand(String cmd) {
        if (cmd.length() == 0) return false;
        if (cmd.startsWith("AT")) return true;  // ELM327 commands
        if (cmd.length() == 4 && cmd.substring(0,2) == "01") return true;  // Mode 01 PIDs
        if (cmd == "03") return true;  // Mode 03 DTCs
        return false;
    }
    
    // Emulate parseOBDData function from firmware  
    String parseOBDData(String cmd) {
        Serial.printf("🔍 parseOBDData(): Processing '%s'\n", cmd.c_str());
        
        // ELM327 Commands (AT commands)
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
        
        // OBD Mode 01 PIDs (emulating GetOBDxxx functions)
        if (cmd == "0100") {
            Serial.println("📊 GetOBDSupportedPIDs() called");
            return "41 00 BE 3E B8 11\r\n>";  // Supported PIDs 01-20
        }
        if (cmd == "010C") {
            Serial.println("📊 GetOBDRpm() called");
            return "41 0C 1A F8\r\n>";        // RPM: 1726
        }
        if (cmd == "010D") {
            Serial.println("📊 GetOBDSpeed() called");
            return "41 0D 3C\r\n>";           // Speed: 60 km/h
        }
        if (cmd == "0105") {
            Serial.println("📊 GetOBDEngCoolDeg() called");
            return "41 05 5A\r\n>";           // Engine Temp: 50°C
        }
        if (cmd == "0111") {
            Serial.println("📊 GetOBDThrottle() called");
            return "41 11 4D\r\n>";           // Throttle: ~30%
        }
        if (cmd == "0142") return "41 42 34 80\r\n>";        // Battery: 13.2V
        if (cmd == "010B") return "41 0B 63\r\n>";           // MAP: 99 kPa
        if (cmd == "0114") return "41 14 80\r\n>";           // Fuel Trim: 0%
        if (cmd == "0110") return "41 10 45\r\n>";           // MAF: 6.9 g/s
        if (cmd == "010F") {
            Serial.println("📊 GetOBDIntakeTemp() called");
            return "41 0F 48\r\n>";           // IAT: 32°C
        }
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
    Serial.println("===================================================");
    Serial.println("🚀 ESP32 BLE OBD-II SERVER - FIRMWARE ENHANCED 🚀");
    Serial.println("===================================================");
    Serial.println("Target Device: Chigee XR-2");
    Serial.println("ESP32 Device Name: CGOBD-5F72");
    Serial.printf("BLE Service UUID: %s\n", SERVICE_UUID);
    Serial.printf("Characteristic UUID: %s\n", CHARACTERISTIC_UUID);
    Serial.println("Protocol: ELM327 over BLE");
    Serial.println("Firmware Analysis: ✅ Complete");
    Serial.println("===================================================");
    Serial.println();
    
    // Initialize BLE with firmware-style logging
    currentState = BLE_STATE_NOT_CONN;
    Serial.println("🔧 BLE_STATE_NOT_CONN: Initializing BLE device...");
    
    BLEDevice::init("CGOBD-5F72");  // Must match firmware expectation
    
    currentState = BLE_STATE_REG_SERVER;
    Serial.println("🔧 BLE_STATE_REG_SERVER: Creating BLE server...");
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create service with discovered UUID
    BLEService *pService = pServer->createService(SERVICE_UUID);
    Serial.printf("✅ Created BLE service with UUID: %s\n", SERVICE_UUID);
    
    // Create characteristic with enhanced configuration
    pCharacteristic = pService->createCharacteristic(
                         CHARACTERISTIC_UUID,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_WRITE |
                         BLECharacteristic::PROPERTY_NOTIFY
                       );
    
    // Add enhanced CCCD (Client Characteristic Configuration Descriptor)
    BLE2902* pBLE2902 = new BLE2902();
    pBLE2902->setNotifications(true);
    pCharacteristic->addDescriptor(pBLE2902);
    
    pCharacteristic->setCallbacks(new MyCallbacks());
    
    Serial.printf("✅ Created characteristic with UUID: %s\n", CHARACTERISTIC_UUID);
    Serial.println("✅ Enhanced CCCD configuration applied");
    
    // Start service
    pService->start();
    Serial.println("✅ BLE service started");
    
    // Configure advertising for XR-2 discovery
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);  // Set for fast connection
    
    BLEDevice::startAdvertising();
    
    currentState = BLE_STATE_SEARCH_DEV;
    Serial.println("🔍 BLE_STATE_SEARCH_DEV: Advertising started");
    Serial.println("📡 Waiting for XR-2 connection...");
    Serial.printf("🎯 serviceuuid: %s\n", SERVICE_UUID);
    Serial.printf("🎯 writeUuid: %s\n", CHARACTERISTIC_UUID);
    Serial.printf("🎯 readUuid: %s\n", CHARACTERISTIC_UUID);
    Serial.println("===================================================");
}

void loop() {
    // Enhanced loop with firmware-style state management
    static unsigned long lastUpdate = 0;
    static unsigned long lastHeartbeat = 0;
    
    // Connection status monitoring  
    if (deviceConnected && currentState == BLE_STATE_RUN) {
        // Update OBD data periodically (emulating continuous sensor reading)
        if (millis() - lastUpdate > 2000) {  // Update every 2 seconds
            updateOBDCharacteristic();
            lastUpdate = millis();
        }
        
        // Connection heartbeat logging
        if (millis() - lastHeartbeat > 15000) {
            Serial.println("� BLE_STATE_RUN: Connection stable - OBD data streaming");
            lastHeartbeat = millis();
        }
    }
    
    delay(100);
}

// Enhanced OBD data update function
void updateOBDCharacteristic() {
    // Emulate g_obd_para global parameter update
    static uint16_t rpm = 1720;
    static uint8_t speed = 60;
    static uint8_t temp = 90;
    
    // Simulate changing values
    rpm += random(-50, 50);
    if (rpm < 800) rpm = 800;
    if (rpm > 6000) rpm = 6000;
    
    speed += random(-2, 2);
    if (speed < 0) speed = 0;
    if (speed > 120) speed = 120;
    
    temp += random(-1, 1);
    if (temp < 80) temp = 80;
    if (temp > 110) temp = 110;
    
    // Create dynamic OBD response with updated values
    String obdData = String("41 0C ") + 
                    String((rpm * 4) >> 8, HEX) + " " + 
                    String((rpm * 4) & 0xFF, HEX) + " " +
                    String("41 0D ") + String(speed, HEX) + " " +
                    String("41 05 ") + String(temp + 40, HEX);  // Offset by 40°C
    
    // Update characteristic value
    pCharacteristic->setValue(obdData.c_str());
    pCharacteristic->notify();
    
    Serial.printf("📤 g_obd_para updated: RPM=%d, Speed=%d, Temp=%d°C\n", rpm, speed, temp-40);
    Serial.printf("� OBD Response: %s\n", obdData.c_str());
}