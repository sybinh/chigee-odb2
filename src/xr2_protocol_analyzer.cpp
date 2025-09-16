#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_main.h>
#include <esp_bt.h>

BluetoothSerial SerialBT;

// Target XR-2 device info
const String TARGET_NAME = "CHIGEE-6697";
const String TARGET_MAC = "34:76:5a:02:c0:33";

bool connected = false;
bool connectAttempted = false;
unsigned long lastConnectionAttempt = 0;
unsigned long lastDataSend = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== XR-2 Bluetooth Protocol Analyzer ===");
    Serial.println("Target: " + TARGET_NAME + " (" + TARGET_MAC + ")");
    Serial.println("==========================================");
    
    if (!SerialBT.begin("ESP32-OBD-Analyzer")) {
        Serial.println("ERROR: Bluetooth initialization failed!");
        return;
    }
    
    Serial.println("✅ Bluetooth initialized");
    Serial.println("🔍 Searching for XR-2...");
    
    // Set up callback for connection events
    SerialBT.register_callback([](esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
        switch(event) {
            case ESP_SPP_INIT_EVT:
                Serial.println("📡 SPP initialized");
                break;
            case ESP_SPP_START_EVT:
                Serial.println("📡 SPP started");
                break;
            case ESP_SPP_OPEN_EVT:
                Serial.println("🔗 Connection OPENED to XR-2!");
                connected = true;
                break;
            case ESP_SPP_CLOSE_EVT:
                Serial.println("❌ Connection CLOSED");
                connected = false;
                connectAttempted = false;
                break;
            case ESP_SPP_DATA_IND_EVT:
                Serial.println("📩 Data received from XR-2:");
                // Print received data in hex and ASCII
                for(int i = 0; i < param->data_ind.len; i++) {
                    Serial.printf("%02X ", param->data_ind.data[i]);
                }
                Serial.println();
                for(int i = 0; i < param->data_ind.len; i++) {
                    char c = param->data_ind.data[i];
                    Serial.print(isprint(c) ? c : '.');
                }
                Serial.println();
                break;
            default:
                Serial.printf("📡 SPP Event: %d\n", event);
                break;
        }
    });
}

void attemptConnection() {
    if (connectAttempted || connected) return;
    
    Serial.println("🔄 Attempting to connect to " + TARGET_NAME + "...");
    
    // Try to connect by name first
    bool result = SerialBT.connect(TARGET_NAME);
    
    if (!result) {
        Serial.println("❌ Connection by name failed");
        Serial.println("🔄 Trying connection by MAC address...");
        
        // Convert MAC string to uint8_t array
        uint8_t mac[6];
        sscanf(TARGET_MAC.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        
        result = SerialBT.connect(mac);
    }
    
    connectAttempted = true;
    lastConnectionAttempt = millis();
    
    if (result) {
        Serial.println("✅ Connection attempt initiated...");
        Serial.println("⏳ Waiting for connection confirmation...");
    } else {
        Serial.println("❌ Connection attempt failed!");
    }
}

void sendTestData() {
    if (!connected) return;
    
    // Send various test commands to see how XR-2 responds
    static int testPhase = 0;
    
    switch(testPhase) {
        case 0:
            Serial.println("📤 Sending: Hello XR-2");
            SerialBT.print("Hello XR-2");
            break;
        case 1:
            Serial.println("📤 Sending: OBD test command");
            SerialBT.print("AT+TEST");
            break;
        case 2:
            Serial.println("📤 Sending: ELM327 style command");
            SerialBT.print("ATZ\r");
            break;
        case 3:
            Serial.println("📤 Sending: OBD PID request");
            SerialBT.print("010C\r");
            break;
        case 4:
            Serial.println("📤 Sending: JSON test");
            SerialBT.print("{\"test\":\"data\"}");
            break;
        default:
            testPhase = 0;
            return;
    }
    
    testPhase++;
    lastDataSend = millis();
}

void discoverDevices() {
    Serial.println("🔍 Scanning for Bluetooth devices...");
    
    SerialBT.discoverAsync([](BTAdvertisedDevice* pDevice) {
        String name = pDevice->getName().c_str();
        String address = pDevice->getAddress().toString().c_str();
        int rssi = pDevice->getRSSI();
        
        if (name.length() > 0) {
            Serial.printf("📱 Found: %s (%s) RSSI: %d\n", 
                         name.c_str(), address.c_str(), rssi);
            
            if (name == TARGET_NAME || address == TARGET_MAC) {
                Serial.println("🎯 Target XR-2 found!");
                SerialBT.discoverAsyncStop();
                delay(1000);
                attemptConnection();
                return false; // Stop discovery
            }
        }
        return true; // Continue discovery
    });
}

void printStatus() {
    Serial.println("\n=== STATUS REPORT ===");
    Serial.println("Target: " + TARGET_NAME + " (" + TARGET_MAC + ")");
    Serial.println("Connected: " + String(connected ? "YES" : "NO"));
    Serial.println("Connection attempted: " + String(connectAttempted ? "YES" : "NO"));
    
    if (connected) {
        Serial.println("📡 Ready to analyze XR-2 communication!");
    } else if (connectAttempted) {
        Serial.println("⏳ Waiting for connection to establish...");
    } else {
        Serial.println("🔍 Ready to search for XR-2...");
    }
    Serial.println("=====================\n");
}

void loop() {
    static unsigned long lastDiscovery = 0;
    static unsigned long lastStatus = 0;
    
    // Print status every 10 seconds
    if (millis() - lastStatus > 10000) {
        printStatus();
        lastStatus = millis();
    }
    
    // If not connected and haven't attempted recently, start discovery
    if (!connected && !connectAttempted && millis() - lastConnectionAttempt > 30000) {
        if (millis() - lastDiscovery > 20000) {
            discoverDevices();
            lastDiscovery = millis();
        }
    }
    
    // Reset connection attempt after timeout
    if (connectAttempted && !connected && millis() - lastConnectionAttempt > 15000) {
        Serial.println("⏰ Connection timeout, will retry...");
        connectAttempted = false;
    }
    
    // Send test data periodically when connected
    if (connected && millis() - lastDataSend > 5000) {
        sendTestData();
    }
    
    // Handle incoming serial commands for manual testing
    if (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        
        if (cmd == "scan") {
            discoverDevices();
        } else if (cmd == "connect") {
            attemptConnection();
        } else if (cmd == "status") {
            printStatus();
        } else if (cmd.startsWith("send ")) {
            String data = cmd.substring(5);
            if (connected) {
                Serial.println("📤 Sending: " + data);
                SerialBT.print(data);
            } else {
                Serial.println("❌ Not connected to send data");
            }
        } else {
            Serial.println("Commands: scan, connect, status, send <data>");
        }
    }
    
    delay(100);
}