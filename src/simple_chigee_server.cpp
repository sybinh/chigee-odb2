#include <Arduino.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;
bool deviceConnected = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Simple CHIGEE Bluetooth Server ===");
    Serial.println("Device Name: CGOBD-5F72");
    Serial.println("======================================");
    
    // Simple Bluetooth initialization
    if (!SerialBT.begin("CGOBD-5F72")) {
        Serial.println("❌ Bluetooth init failed!");
        return;
    }
    
    Serial.println("✅ Bluetooth server started");
    Serial.println("📡 Waiting for connections...");
    Serial.println();
    Serial.println("Instructions:");
    Serial.println("1. Go to XR-2 Bluetooth settings");
    Serial.println("2. Search for 'CGOBD-5F72'");
    Serial.println("3. Try to pair/connect");
    Serial.println("4. Watch this monitor for results");
    Serial.println();
}

void loop() {
    static unsigned long lastCheck = 0;
    static bool wasConnected = false;
    
    // Check connection status every second
    if (millis() - lastCheck > 1000) {
        bool connected = SerialBT.hasClient();
        
        if (connected != wasConnected) {
            if (connected) {
                Serial.println("🎉 CLIENT CONNECTED to CGOBD-5F72!");
                deviceConnected = true;
            } else {
                Serial.println("❌ Client disconnected");
                deviceConnected = false;
            }
            wasConnected = connected;
        }
        
        lastCheck = millis();
    }
    
    // Handle incoming data
    if (SerialBT.available()) {
        String received = SerialBT.readString();
        Serial.println("📩 Received: " + received);
        
        // Echo back
        SerialBT.print("Echo: " + received);
    }
    
    // Handle Serial commands
    if (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        
        if (cmd == "status") {
            Serial.println("=== STATUS ===");
            Serial.println("Device: CHIGEE-TEST");
            Serial.println("Connected: " + String(deviceConnected ? "YES" : "NO"));
            Serial.println("==============");
        } else if (cmd.startsWith("send ")) {
            String data = cmd.substring(5);
            if (deviceConnected) {
                Serial.println("📤 Sending: " + data);
                SerialBT.print(data);
            } else {
                Serial.println("❌ No device connected");
            }
        } else if (cmd == "test") {
            if (deviceConnected) {
                SerialBT.print("Test OBD data: 41 0C 1A F8");
                Serial.println("📤 Sent test OBD data");
            } else {
                Serial.println("❌ No device connected");
            }
        } else {
            Serial.println("Commands: status, send <data>, test");
        }
    }
    
    delay(100);
}