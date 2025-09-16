/*
 * Compact WiFi + Bluetooth OBD Server for ESP32
 * Minimal version for maximum compatibility
 */

#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>

BluetoothSerial BT;
WebServer server(80);

// Simple OBD data
float rpm = 1500, speed = 60, temp = 85, fuel = 75;
unsigned long lastUpdate = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Compact OBD Server");
    
    // Init Bluetooth
    BT.begin("ESP32_OBD");
    Serial.println("BT: ESP32_OBD");
    
    // Init WiFi AP
    WiFi.softAP("ESP32_OBD_WiFi", "12345678");
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.softAPIP());
    
    // Web routes
    server.on("/", []() {
        server.send(200, "text/html", 
            F("<!DOCTYPE html><html><head><title>ESP32 OBD</title></head>"
              "<body><h1>ESP32 OBD Server</h1>"
              "<p>RPM: <span id='r'>0</span></p>"
              "<p>Speed: <span id='s'>0</span> km/h</p>"
              "<p>Temp: <span id='t'>0</span>C</p>"
              "<p>Fuel: <span id='f'>0</span>%</p>"
              "<script>"
              "setInterval(function(){"
              "fetch('/api').then(r=>r.json()).then(d=>{"
              "document.getElementById('r').innerHTML=d.rpm;"
              "document.getElementById('s').innerHTML=d.speed;"
              "document.getElementById('t').innerHTML=d.temp;"
              "document.getElementById('f').innerHTML=d.fuel;"
              "})},1000);"
              "</script></body></html>"));
    });
    
    server.on("/api", []() {
        server.send(200, "application/json", 
            "{\"rpm\":" + String(rpm) + 
            ",\"speed\":" + String(speed) + 
            ",\"temp\":" + String(temp) + 
            ",\"fuel\":" + String(fuel) + "}");
    });
    
    server.begin();
    Serial.println("Server started");
}

void loop() {
    server.handleClient();
    
    // Handle BT OBD
    if (BT.available()) {
        String cmd = BT.readStringUntil('\r');
        cmd.trim();
        cmd.toUpperCase();
        
        if (cmd.startsWith("AT")) {
            BT.println("OK");
        } else if (cmd == "010C") { // RPM
            int r = rpm * 4;
            BT.printf("41 0C %02X %02X\r>", r >> 8, r & 0xFF);
        } else if (cmd == "010D") { // Speed
            BT.printf("41 0D %02X\r>", (int)speed);
        } else if (cmd == "0105") { // Temp
            BT.printf("41 05 %02X\r>", (int)temp + 40);
        } else if (cmd == "012F") { // Fuel
            BT.printf("41 2F %02X\r>", (int)(fuel * 2.55));
        } else {
            BT.println("NO DATA\r>");
        }
    }
    
    // Update data
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        rpm = 1500 + sin(millis() / 1000.0) * 500;
        speed = 60 + sin(millis() / 2000.0) * 20;
        temp = 85 + sin(millis() / 5000.0) * 10;
        fuel = 75 - (millis() / 60000.0) * 0.1;
    }
    
    delay(50);
}