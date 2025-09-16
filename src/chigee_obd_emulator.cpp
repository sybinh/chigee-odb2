#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_bt_device.h>

BluetoothSerial SerialBT;

bool clientConnected = false;
unsigned long lastStatusPrint = 0;
unsigned long lastDataSend = 0;

// Generate CHIGEE-like device name
String generateChigeeDeviceName() {
    // Use last 4 digits of MAC for unique ID
    const uint8_t* mac = esp_bt_dev_get_address();
    uint16_t id = (mac[4] << 8) | mac[5];
    return "CHIGEE-" + String(id, HEX);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    String deviceName = generateChigeeDeviceName();
    
    Serial.println("=== ESP32 Chigee OBD Module Emulator ===");
    Serial.println("Device Name: " + deviceName);
    Serial.println("Waiting for XR-2 to connect...");
    Serial.println("=========================================");
    
    // Initialize Bluetooth with CHIGEE-like name
    if (!SerialBT.begin(deviceName)) {
        Serial.println("ERROR: Bluetooth initialization failed!");
        return;
    }
    
    Serial.println("✅ Bluetooth Server started as: " + deviceName);
    Serial.println("📡 Advertising and waiting for XR-2 connection...");
    
    // Set up callback for connection events
    SerialBT.register_callback([](esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
        switch(event) {
            case ESP_SPP_INIT_EVT:
                Serial.println("📡 SPP initialized");
                break;
            case ESP_SPP_START_EVT:
                Serial.println("📡 SPP server started - ready for connections");
                break;
            case ESP_SPP_SRV_OPEN_EVT:
                Serial.println("🎉 CLIENT CONNECTED!");
                if (param && param->srv_open.rem_bda) {
                    Serial.print("Remote address: ");
                    for(int i = 0; i < 6; i++) {
                        Serial.printf("%02X", param->srv_open.rem_bda[i]);
                        if(i < 5) Serial.print(":");
                    }
                    Serial.println();
                }
                clientConnected = true;
                break;
            case ESP_SPP_CLOSE_EVT:
                Serial.println("❌ Client disconnected");
                clientConnected = false;
                break;
            case ESP_SPP_DATA_IND_EVT:
                if (param && param->data_ind.data && param->data_ind.len > 0) {
                    Serial.println("📩 Data received from XR-2:");
                    Serial.print("Length: ");
                    Serial.println(param->data_ind.len);
                    Serial.print("HEX: ");
                    for(int i = 0; i < param->data_ind.len; i++) {
                        Serial.printf("%02X ", param->data_ind.data[i]);
                    }
                    Serial.println();
                    Serial.print("ASCII: ");
                    for(int i = 0; i < param->data_ind.len; i++) {
                        char c = param->data_ind.data[i];
                        Serial.print(isprint(c) ? c : '.');
                    }
                    Serial.println();
                    Serial.println("================================");
                    
                    // Echo response for testing
                    SerialBT.write(param->data_ind.data, param->data_ind.len);
                }
                break;
            default:
                Serial.printf("📡 SPP Event: %d\n", event);
                break;
        }
    });
    
    Serial.println();
    Serial.println("🔍 INSTRUCTIONS:");
    Serial.println("1. Go to XR-2 Bluetooth settings");
    Serial.println("2. Search for: " + deviceName);
    Serial.println("3. Pair and connect");
    Serial.println("4. Watch for communication analysis");
    Serial.println();
}

void sendTestOBDData() {
    if (!clientConnected) return;
    
    static int testPhase = 0;
    String testData;
    
    switch(testPhase % 6) {
        case 0:
            testData = "41 0C 1A F8\r";  // RPM response (6904 RPM)
            Serial.println("📤 Sending RPM data: " + testData);
            break;
        case 1:
            testData = "41 0D 2D\r";     // Speed response (45 km/h) 
            Serial.println("📤 Sending Speed data: " + testData);
            break;
        case 2:
            testData = "41 05 5A\r";     // Engine coolant temp (50°C)
            Serial.println("📤 Sending Temp data: " + testData);
            break;
        case 3:
            testData = "41 0F 3C\r";     // Intake air temp (20°C)
            Serial.println("📤 Sending Air Temp data: " + testData);
            break;
        case 4:
            testData = "41 04 80\r";     // Engine load (50%)
            Serial.println("📤 Sending Load data: " + testData);
            break;
        case 5:
            testData = "41 11 4F\r";     // Throttle position (31%)
            Serial.println("📤 Sending Throttle data: " + testData);
            break;
    }
    
    SerialBT.print(testData);
    testPhase++;
}

void printStatus() {
    const uint8_t* mac = esp_bt_dev_get_address();
    
    Serial.println();
    Serial.println("=== STATUS REPORT ===");
    Serial.printf("ESP32 MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println("Device Name: " + generateChigeeDeviceName());
    Serial.println("Client Connected: " + String(clientConnected ? "YES" : "NO"));
    
    if (clientConnected) {
        Serial.println("📡 XR-2 is connected! Sending test OBD data...");
    } else {
        Serial.println("⏳ Waiting for XR-2 to connect...");
        Serial.println("   Make sure XR-2 is in pairing mode!");
    }
    Serial.println("=====================");
    Serial.println();
}

void loop() {
    // Print status every 15 seconds
    if (millis() - lastStatusPrint > 15000) {
        printStatus();
        lastStatusPrint = millis();
    }
    
    // Send test OBD data every 3 seconds when connected
    if (clientConnected && millis() - lastDataSend > 3000) {
        sendTestOBDData();
        lastDataSend = millis();
    }
    
    // Handle manual commands from Serial Monitor
    if (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "test") {
            if (clientConnected) {
                sendTestOBDData();
            } else {
                Serial.println("❌ No client connected");
            }
        } else if (cmd.startsWith("send ")) {
            String data = cmd.substring(5);
            if (clientConnected) {
                Serial.println("📤 Sending: " + data);
                SerialBT.print(data);
            } else {
                Serial.println("❌ No client connected");
            }
        } else {
            Serial.println("Commands: status, test, send <data>");
        }
    }
    
    delay(100);
}