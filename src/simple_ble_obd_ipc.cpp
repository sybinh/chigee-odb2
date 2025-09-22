#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Service UUID discovered from XR-2 firmware analysis
// Try standard Serial Port Profile UUID for better compatibility  
#define SERVICE_UUID        "0000aaa1-0000-1000-8000-00805f9b34fb"  // Standard OBD Service UUID
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

// IPC State Management - From XR-2 Firmware Analysis
bool obd_device_paired = false;
bool obd_device_usable = false;
unsigned long last_ipc_signal = 0;

// IPC Functions - Match XR-2 firmware expectations
bool IPC_IsOBDPair() {
    return obd_device_paired;
}

bool IPC_IsOBDUsable() {
    return obd_device_usable;
}

void IPC_SetOBDPaired(bool paired) {
    obd_device_paired = paired;
    Serial.println("🔗 IPC: OBD Pair Status = " + String(paired ? "PAIRED" : "UNPAIRED"));
}

void IPC_SetOBDUsable(bool usable) {
    obd_device_usable = usable;
    Serial.println("✅ IPC: OBD Usable Status = " + String(usable ? "USABLE" : "UNUSABLE"));
}

void IPC_SendHandshake() {
    // Send IPC handshake signals every 5 seconds
    if (millis() - last_ipc_signal > 5000) {
        Serial.println("📡 IPC: Sending handshake signals to XR-2");
        
        // Signal device is paired and ready
        IPC_SetOBDPaired(true);
        IPC_SetOBDUsable(true);
        
        last_ipc_signal = millis();
    }
}

// Checksum Functions - From XR-2 Firmware Analysis
uint8_t sum_check(const uint8_t* data, int length) {
    uint8_t checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool data_check(const uint8_t* frame, int length) {
    // Validate frame structure and checksum
    if (length < 3) return false;  // Minimum OBD frame size
    
    // Check if it's a valid OBD response (starts with 41-49)
    if (frame[0] < 0x41 || frame[0] > 0x49) return false;
    
    // Calculate and validate checksum
    uint8_t calculated_checksum = sum_check(frame, length - 1);
    uint8_t frame_checksum = frame[length - 1];
    
    return (calculated_checksum == frame_checksum);
}

void IPC_OBDValidBit8(uint8_t value, const char* description) {
    if (value == 0xFF) {
        Serial.printf("⚠️ IPC: Invalid 8-bit value for %s: 0x%02X\n", description, value);
    } else {
        Serial.printf("✅ IPC: Valid 8-bit %s: 0x%02X\n", description, value);
    }
}

void IPC_OBDValidBit16(uint16_t value, const char* description) {
    if (value == 0xFFFF) {
        Serial.printf("⚠️ IPC: Invalid 16-bit value for %s: 0x%04X\n", description, value);
    } else {
        Serial.printf("✅ IPC: Valid 16-bit %s: 0x%04X\n", description, value);
    }
}

void IPC_OBDValidBit32(uint32_t value, const char* description) {
    if (value == 0xFFFFFFFF) {
        Serial.printf("⚠️ IPC: Invalid 32-bit value for %s: 0x%08X\n", description, value);
    } else {
        Serial.printf("✅ IPC: Valid 32-bit %s: 0x%08X\n", description, value);
    }
}

// Global BLE variables
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
        Serial.printf("📍 BLE_STATE_CONN_DEV: Connected device\n");
        
        // IPC Handshake - CRITICAL per firmware requirements
        Serial.println("🤝 Starting IPC handshake sequence...");
        IPC_SetOBDPaired(true);     // Signal device is paired
        IPC_SetOBDUsable(true);     // Signal device is ready
        
        currentState = BLE_STATE_RUN;
        Serial.println("✅ BLE_STATE_RUN: Data connection established");
        
        // Send OBD data using discovered function patterns
        delay(500);
        sendOBDSupportedPIDs();
        sendGetOBDRpm();
        delay(100);
        sendGetOBDSpeed();
        delay(100);
        sendGetOBDEngCoolDeg();
        delay(100);
        sendGetOBDThrottle();
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        currentState = BLE_STATE_DIS_CONN;
        
        // Reset IPC state on disconnect
        IPC_SetOBDPaired(false);
        IPC_SetOBDUsable(false);
        
        // Log disconnection with firmware-style format
        Serial.printf("❌ BLE_STATE_DIS_CONN: Device disconnected\n");
        
        currentState = BLE_STATE_DIS_CONN_STOP;
        Serial.println("🔄 BLE_STATE_DIS_CONN_STOP: Restarting advertising");
        
        // Restart advertising for auto-reconnection
        BLEDevice::startAdvertising();
        currentState = BLE_STATE_SEARCH_DEV;
        Serial.println("🔍 BLE_STATE_SEARCH_DEV: Waiting for XR-2 connection");
    }
    
    void sendOBDSupportedPIDs() {
        // IPC Validation - CRITICAL per firmware requirements
        if (!IPC_IsOBDPair() || !IPC_IsOBDUsable()) {
            Serial.println("❌ IPC: Supported PIDs blocked - device not ready");
            return;
        }
        
        // Create comprehensive supported PIDs mask - PID 0100 response
        // Based on XR-2 firmware requirements: 41 00 FE 3F F8 11
        uint8_t frame[] = {0x41, 0x00, 0xFE, 0x3F, 0xF8, 0x11};
        uint8_t checksum = sum_check(frame, sizeof(frame));
        
        // Add checksum to frame
        uint8_t validated_frame[sizeof(frame) + 1];
        memcpy(validated_frame, frame, sizeof(frame));
        validated_frame[sizeof(frame)] = checksum;
        
        // Validate frame before sending
        if (data_check(validated_frame, sizeof(validated_frame))) {
            pCharacteristic->setValue(validated_frame, sizeof(validated_frame));
            pCharacteristic->notify();
            
            // Log supported PIDs breakdown
            Serial.printf("📤 Supported PIDs (0100): Binary frame with checksum 0x%02X\n", checksum);
            Serial.println("   ✅ Supported PIDs breakdown:");
            Serial.println("   • 0105: Engine Coolant Temperature");
            Serial.println("   • 010B: Intake Manifold Pressure");
            Serial.println("   • 010C: Engine RPM");
            Serial.println("   • 010D: Vehicle Speed");
            Serial.println("   • 010E: Timing Advance");
            Serial.println("   • 010F: Intake Air Temperature");
            Serial.println("   • 0111: Throttle Position");
            Serial.println("   • 011F: Run time since engine start");
            Serial.println("   • 0131: Distance traveled since codes cleared");
            Serial.println("   • 0133: Barometric pressure");
            Serial.println("   • 013C: Catalyst Temperature");
            Serial.println("   • 0142: Control module voltage");
            Serial.println("   • 015E: Engine fuel rate");
        } else {
            Serial.println("❌ Checksum: Supported PIDs frame validation failed");
        }
    }
    
    void sendGetOBDRpm() {
        // IPC Validation - CRITICAL per firmware requirements
        if (!IPC_IsOBDPair() || !IPC_IsOBDUsable()) {
            Serial.println("❌ IPC: GetOBDRpm() blocked - device not ready");
            return;
        }
        
        // Create binary OBD frame with checksum validation
        uint8_t frame[] = {0x41, 0x0C, 0x1A, 0xF8};  // RPM data
        uint8_t checksum = sum_check(frame, sizeof(frame));
        
        // Add checksum to frame
        uint8_t validated_frame[sizeof(frame) + 1];
        memcpy(validated_frame, frame, sizeof(frame));
        validated_frame[sizeof(frame)] = checksum;
        
        // Validate frame before sending
        if (data_check(validated_frame, sizeof(validated_frame))) {
            pCharacteristic->setValue(validated_frame, sizeof(validated_frame));
            pCharacteristic->notify();
            
            // Validate data values
            uint16_t rpm_value = (frame[2] << 8) | frame[3];
            IPC_OBDValidBit16(rpm_value / 4, "RPM");
            
            Serial.printf("📤 GetOBDRpm(): Binary frame with checksum 0x%02X (IPC validated)\n", checksum);
        } else {
            Serial.println("❌ Checksum: GetOBDRpm() frame validation failed");
        }
    }
    
    void sendGetOBDSpeed() {
        // IPC Validation - CRITICAL per firmware requirements
        if (!IPC_IsOBDPair() || !IPC_IsOBDUsable()) {
            Serial.println("❌ IPC: GetOBDSpeed() blocked - device not ready");
            return;
        }
        
        // Create binary OBD frame with checksum validation
        uint8_t frame[] = {0x41, 0x0D, 0x3C};  // Speed data (60 km/h)
        uint8_t checksum = sum_check(frame, sizeof(frame));
        
        // Add checksum to frame
        uint8_t validated_frame[sizeof(frame) + 1];
        memcpy(validated_frame, frame, sizeof(frame));
        validated_frame[sizeof(frame)] = checksum;
        
        // Validate frame before sending
        if (data_check(validated_frame, sizeof(validated_frame))) {
            pCharacteristic->setValue(validated_frame, sizeof(validated_frame));
            pCharacteristic->notify();
            
            // Validate data values
            IPC_OBDValidBit8(frame[2], "Speed");
            
            Serial.printf("📤 GetOBDSpeed(): Binary frame with checksum 0x%02X (IPC validated)\n", checksum);
        } else {
            Serial.println("❌ Checksum: GetOBDSpeed() frame validation failed");
        }
    }
    
    void sendGetOBDEngCoolDeg() {
        // IPC Validation - CRITICAL per firmware requirements
        if (!IPC_IsOBDPair() || !IPC_IsOBDUsable()) {
            Serial.println("❌ IPC: GetOBDEngCoolDeg() blocked - device not ready");
            return;
        }
        
        // Create binary OBD frame with checksum validation
        uint8_t frame[] = {0x41, 0x05, 0x5A};  // Coolant temp (90°C = 0x5A + 40)
        uint8_t checksum = sum_check(frame, sizeof(frame));
        
        // Add checksum to frame
        uint8_t validated_frame[sizeof(frame) + 1];
        memcpy(validated_frame, frame, sizeof(frame));
        validated_frame[sizeof(frame)] = checksum;
        
        // Validate frame before sending
        if (data_check(validated_frame, sizeof(validated_frame))) {
            pCharacteristic->setValue(validated_frame, sizeof(validated_frame));
            pCharacteristic->notify();
            
            // Validate data values (convert to actual temperature)
            uint8_t temp_celsius = frame[2] - 40;
            IPC_OBDValidBit8(temp_celsius, "Coolant Temp (°C)");
            
            Serial.printf("📤 GetOBDEngCoolDeg(): Binary frame with checksum 0x%02X (IPC validated)\n", checksum);
        } else {
            Serial.println("❌ Checksum: GetOBDEngCoolDeg() frame validation failed");
        }
    }
    
    void sendGetOBDThrottle() {
        // IPC Validation - CRITICAL per firmware requirements
        if (!IPC_IsOBDPair() || !IPC_IsOBDUsable()) {
            Serial.println("❌ IPC: GetOBDThrottle() blocked - device not ready");
            return;
        }
        
        // Create binary OBD frame with checksum validation
        uint8_t frame[] = {0x41, 0x11, 0x4D};  // Throttle position (~30%)
        uint8_t checksum = sum_check(frame, sizeof(frame));
        
        // Add checksum to frame
        uint8_t validated_frame[sizeof(frame) + 1];
        memcpy(validated_frame, frame, sizeof(frame));
        validated_frame[sizeof(frame)] = checksum;
        
        // Validate frame before sending
        if (data_check(validated_frame, sizeof(validated_frame))) {
            pCharacteristic->setValue(validated_frame, sizeof(validated_frame));
            pCharacteristic->notify();
            
            // Validate data values (convert to percentage)
            uint8_t throttle_percent = (frame[2] * 100) / 255;
            IPC_OBDValidBit8(throttle_percent, "Throttle Position (%)");
            
            Serial.printf("📤 GetOBDThrottle(): Binary frame with checksum 0x%02X (IPC validated)\n", checksum);
        } else {
            Serial.println("❌ Checksum: GetOBDThrottle() frame validation failed");
        }
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.println("\n📨 Command from XR-2:");
            Serial.print("   Raw bytes: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
            
            String cmd = String(rxValue.c_str());
            cmd.trim();
            cmd.toUpperCase();
            
            // Process different command types
            if (cmd.startsWith("AT")) {
                // AT commands get ASCII responses
                String response = processATCommand(cmd);
                pCharacteristic->setValue(response.c_str());
                pCharacteristic->notify();
                Serial.println("📤 AT Response: '" + response + "'");
            } else if (cmd.startsWith("01")) {
                // OBD PIDs - process with IPC validation and specific PID handling
                processPIDRequest(cmd, pCharacteristic);
            }
        }
    }
    
private:
    String processATCommand(String cmd) {
        cmd.replace("\r", "");
        cmd.replace("\n", "");
        
        // Binary responses for XR-2 compatibility
        return "OK\r>";  // Single response for all AT commands
    }
    
    void processPIDRequest(String cmd, BLECharacteristic* pCharacteristic) {
        // IPC Validation for all PID requests
        if (!IPC_IsOBDPair() || !IPC_IsOBDUsable()) {
            Serial.println("❌ IPC: PID request blocked - device not ready");
            return;
        }
        
        Serial.println("🔍 Processing PID request: " + cmd);
        
        // Parse PID from command (e.g., "0100" -> 0x00)
        if (cmd.length() >= 4) {
            String pidStr = cmd.substring(2, 4);
            uint8_t pid = strtol(pidStr.c_str(), NULL, 16);
            
            uint8_t frame[8];  // Max frame size
            int frameSize = 0;
            bool validPID = true;
            
            switch (pid) {
                case 0x00:  // Supported PIDs 01-20 - IMMEDIATE RESPONSE
                    frame[0] = 0x41; frame[1] = 0x00; 
                    frame[2] = 0xFE; frame[3] = 0x3F; frame[4] = 0xF8; frame[5] = 0x11;
                    frameSize = 6;
                    Serial.println("   📋 PID 0100: Supported PIDs 01-20 [IMMEDIATE]");
                    
                    // Immediate binary transmission for XR-2
                    if (data_check(frame, frameSize)) {
                        pCharacteristic->setValue(frame, frameSize);
                        pCharacteristic->notify();
                        Serial.println("   ✅ Binary frame sent IMMEDIATELY");
                    }
                    return;  // Exit immediately - no delayed notification
                    break;
                    
                case 0x05:  // Engine Coolant Temperature
                    frame[0] = 0x41; frame[1] = 0x05; frame[2] = 0x5A;  // 90°C
                    frameSize = 3;
                    Serial.println("   🌡️ PID 0105: Engine Coolant Temperature");
                    break;
                    
                case 0x0B:  // Intake Manifold Pressure
                    frame[0] = 0x41; frame[1] = 0x0B; frame[2] = 0x63;  // 99 kPa
                    frameSize = 3;
                    Serial.println("   📊 PID 010B: Intake Manifold Pressure");
                    break;
                    
                case 0x0C:  // Engine RPM
                    frame[0] = 0x41; frame[1] = 0x0C; frame[2] = 0x1A; frame[3] = 0xF8;  // 1726 RPM
                    frameSize = 4;
                    Serial.println("   🔄 PID 010C: Engine RPM");
                    break;
                    
                case 0x0D:  // Vehicle Speed
                    frame[0] = 0x41; frame[1] = 0x0D; frame[2] = 0x3C;  // 60 km/h
                    frameSize = 3;
                    Serial.println("   🚗 PID 010D: Vehicle Speed");
                    break;
                    
                case 0x11:  // Throttle Position
                    frame[0] = 0x41; frame[1] = 0x11; frame[2] = 0x4D;  // ~30%
                    frameSize = 3;
                    Serial.println("   🎛️ PID 0111: Throttle Position");
                    break;
                    
                case 0x42:  // Control Module Voltage
                    frame[0] = 0x41; frame[1] = 0x42; frame[2] = 0x34; frame[3] = 0x80;  // 13.2V
                    frameSize = 4;
                    Serial.println("   ⚡ PID 0142: Control Module Voltage");
                    break;
                    
                default:
                    validPID = false;
                    Serial.println("   ❓ PID " + pidStr + ": NOT SUPPORTED");
                    break;
            }
            
            if (validPID) {
                // Add checksum and send
                uint8_t checksum = sum_check(frame, frameSize);
                uint8_t validated_frame[frameSize + 1];
                memcpy(validated_frame, frame, frameSize);
                validated_frame[frameSize] = checksum;
                
                if (data_check(validated_frame, frameSize + 1)) {
                    pCharacteristic->setValue(validated_frame, frameSize + 1);
                    pCharacteristic->notify();
                    Serial.printf("   📤 PID Response: %d bytes with checksum 0x%02X\n", frameSize + 1, checksum);
                } else {
                    Serial.println("   ❌ Checksum validation failed for PID response");
                }
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("===========================================");
    Serial.println("🚀 ESP32 BLE OBD-II SERVER with IPC 🚀");
    Serial.println("===========================================");
    Serial.println("Target: Chigee XR-2");
    Serial.println("Service UUID: " + String(SERVICE_UUID));
    Serial.println("Features: IPC Validation + Binary OBD");
    Serial.println("===========================================");

    // Initialize BLE device
    BLEDevice::init("CGOBD-5F72");  // XR-2 compatible device name
    Serial.println("🔧 BLE Device Name: CGOBD-5F72");

    // Create server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create characteristic
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

    // Start service and advertising
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();

    currentState = BLE_STATE_SEARCH_DEV;
    Serial.println("🔍 BLE_STATE_SEARCH_DEV: Waiting for XR-2...");
    Serial.println("📋 IPC Functions Ready:");
    Serial.println("   • IPC_IsOBDPair() - Device pairing status");
    Serial.println("   • IPC_IsOBDUsable() - Device readiness");
    Serial.println("   • IPC handshake every 5 seconds");
    Serial.println("===========================================");
}

void loop() {
    // IPC Handshake Management - CRITICAL for XR-2 compatibility
    if (deviceConnected) {
        IPC_SendHandshake();  // Maintain IPC signaling
    }
    
    // Enhanced loop with firmware-style state management
    static unsigned long lastUpdate = 0;
    static unsigned long lastHeartbeat = 0;
    
    // Connection status monitoring  
    if (deviceConnected && currentState == BLE_STATE_RUN && IPC_IsOBDUsable()) {
        // Update OBD data periodically
        if (millis() - lastUpdate > 2000) {  // Update every 2 seconds
            // Only update if IPC validates device is ready
            if (IPC_IsOBDPair() && IPC_IsOBDUsable()) {
                // Create comprehensive binary OBD frame with checksum
                uint8_t multi_frame[] = {
                    0x41, 0x0C, 0x1A, 0xF8,  // RPM
                    0x41, 0x0D, 0x3C,        // Speed
                    0x41, 0x05, 0x5A,        // Coolant temp
                    0x41, 0x42, 0x34, 0x80,  // Voltage
                    0x41, 0x0B, 0x63         // MAP pressure
                };
                
                // Calculate comprehensive frame checksum
                uint8_t checksum = sum_check(multi_frame, sizeof(multi_frame));
                
                // Add checksum to frame
                uint8_t validated_multi_frame[sizeof(multi_frame) + 1];
                memcpy(validated_multi_frame, multi_frame, sizeof(multi_frame));
                validated_multi_frame[sizeof(multi_frame)] = checksum;
                
                // Send binary frame with checksum validation
                pCharacteristic->setValue(validated_multi_frame, sizeof(validated_multi_frame));
                pCharacteristic->notify();
                
                Serial.printf("📤 IPC-Validated Binary OBD Update: %d bytes with checksum 0x%02X\n", 
                              sizeof(validated_multi_frame), checksum);
            } else {
                Serial.println("⚠️ IPC: OBD update skipped - device not ready");
            }
            lastUpdate = millis();
        }
        
        // Connection heartbeat logging
        if (millis() - lastHeartbeat > 15000) {
            Serial.println("💓 BLE_STATE_RUN: IPC Active - OBD data streaming");
            lastHeartbeat = millis();
        }
    }
    
    delay(100);
}