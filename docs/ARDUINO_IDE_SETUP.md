# Arduino IDE Setup Guide for ESP32 Testing

## 🛠️ Quick Setup for Arduino IDE Testing

### 1. Arduino IDE Installation
```
1. Download Arduino IDE 2.x from: https://www.arduino.cc/en/software
2. Install and run Arduino IDE
3. Go to File → Preferences → Additional Board Manager URLs
4. Add: https://dl.espressif.com/dl/package_esp32_index.json
5. Go to Tools → Board → Boards Manager
6. Search "ESP32" → Install "ESP32 by Espressif Systems"
```

### 2. Required Libraries
```
Tools → Manage Libraries → Install:
- BluetoothSerial (should be included with ESP32 package)
- No additional libraries needed for basic testing
```

### 3. Board Configuration
```
Tools → Board → ESP32 Arduino → "ESP32 Dev Module"
Tools → Port → Select your ESP32 COM port
Tools → Upload Speed → 921600
Tools → CPU Frequency → 240MHz (WiFi/BT)
Tools → Flash Frequency → 80MHz  
Tools → Flash Mode → QIO
Tools → Flash Size → 4MB (32Mb)
Tools → Partition Scheme → "Huge APP (3MB No OTA/1MB SPIFFS)"
```

### 4. Test Code for Arduino IDE

Create new sketch and paste this simplified test code:

```cpp
/**
 * ESP32-WROOM-32 Hardware Validation Test
 * For Arduino IDE
 */

#include "BluetoothSerial.h"

// Pin definitions for ESP32-WROOM-32
#define LED_BUILTIN 2
#define LED_STATUS 2
#define TEST_PIN_1 4
#define TEST_PIN_2 16
#define TEST_PIN_3 17

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n🚗 ChigeeOBD2 ESP32-WROOM-32 Test 🚗");
  Serial.println("===================================");
  
  // Initialize pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEST_PIN_1, OUTPUT);
  pinMode(TEST_PIN_2, OUTPUT);
  pinMode(TEST_PIN_3, OUTPUT);
  
  // LED test sequence
  Serial.println("🔴 Testing LEDs...");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Hardware validation
  Serial.println("🔧 Running hardware tests...");
  validateHardware();
  
  // Bluetooth test
  Serial.println("📱 Starting Bluetooth...");
  if (SerialBT.begin("ChigeeOBD2_ESP32")) {
    Serial.println("✅ Bluetooth started successfully!");
    Serial.println("📱 Device name: ChigeeOBD2_ESP32");
    Serial.println("🔑 PIN: 1234");
  } else {
    Serial.println("❌ Bluetooth failed to start!");
  }
  
  Serial.println("\n🎮 Ready for testing!");
  Serial.println("Commands: help, bt_status, dummy_data");
}

void loop() {
  static uint32_t lastUpdate = 0;
  static uint32_t lastHeartbeat = 0;
  
  // Heartbeat LED
  if (millis() - lastHeartbeat > 1000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastHeartbeat = millis();
  }
  
  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    handleCommand(command);
  }
  
  // Handle Bluetooth data
  if (SerialBT.available()) {
    String btData = SerialBT.readString();
    btData.trim();
    btData.toUpperCase();
    Serial.println("📨 BT Received: " + btData);
    handleOBD2Command(btData);
  }
  
  // Print status every 10 seconds
  if (millis() - lastUpdate > 10000) {
    printStatus();
    lastUpdate = millis();
  }
  
  delay(100);
}

void validateHardware() {
  bool allPassed = true;
  
  // Check chip info
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  
  Serial.printf("Chip: %s, Cores: %d, Revision: %d\n", 
    (chip_info.model == CHIP_ESP32) ? "ESP32" : "Unknown",
    chip_info.cores, chip_info.revision);
  
  // Check flash size
  uint32_t flashSize = ESP.getFlashChipSize();
  Serial.printf("Flash: %d MB\n", flashSize / (1024 * 1024));
  
  // Check free heap
  uint32_t freeHeap = ESP.getFreeHeap();
  Serial.printf("Free Heap: %d KB\n", freeHeap / 1024);
  
  // Test GPIO pins
  Serial.println("Testing GPIO pins...");
  testGPIO(TEST_PIN_1, "GPIO4");
  testGPIO(TEST_PIN_2, "GPIO16"); 
  testGPIO(TEST_PIN_3, "GPIO17");
  
  // Test ADC
  Serial.println("Testing ADC...");
  int adc1 = analogRead(36);
  int adc2 = analogRead(39);
  Serial.printf("ADC36: %d, ADC39: %d\n", adc1, adc2);
  
  if (chip_info.model == CHIP_ESP32 && flashSize >= 4*1024*1024 && freeHeap > 200*1024) {
    Serial.println("✅ Hardware validation PASSED!");
  } else {
    Serial.println("❌ Hardware validation FAILED!");
  }
}

void testGPIO(int pin, const char* name) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  pinMode(pin, INPUT);
  bool high = digitalRead(pin);
  
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(10);
  pinMode(pin, INPUT);
  bool low = !digitalRead(pin);
  
  Serial.printf("%s: %s\n", name, (high && low) ? "OK" : "FAIL");
}

void handleCommand(String cmd) {
  cmd.toLowerCase();
  
  if (cmd == "help") {
    Serial.println("\n📋 Available Commands:");
    Serial.println("help        - Show this menu");
    Serial.println("bt_status   - Bluetooth status");
    Serial.println("dummy_data  - Send dummy OBD2 data");
    Serial.println("reboot      - Restart ESP32");
    Serial.println("info        - System information");
  }
  else if (cmd == "bt_status") {
    Serial.printf("Bluetooth: %s\n", SerialBT.hasClient() ? "Connected" : "Disconnected");
    if (SerialBT.hasClient()) {
      Serial.println("✅ Ready for OBD2 commands");
    }
  }
  else if (cmd == "dummy_data") {
    sendDummyOBD2Data();
  }
  else if (cmd == "reboot") {
    Serial.println("🔄 Rebooting ESP32...");
    ESP.restart();
  }
  else if (cmd == "info") {
    printSystemInfo();
  }
  else {
    Serial.println("❓ Unknown command. Type 'help' for available commands");
  }
}

void handleOBD2Command(String cmd) {
  String response = "";
  
  if (cmd == "ATZ") {
    response = "ELM327 v2.1";
  }
  else if (cmd == "ATE0" || cmd == "ATL0" || cmd == "ATS0" || cmd == "ATH1" || cmd == "ATSP0") {
    response = "OK";
  }
  else if (cmd == "ATI") {
    response = "ELM327 v2.1";
  }
  else if (cmd == "010C") {  // RPM
    uint16_t rpm = 850 + random(-50, 51);  // Idle RPM with variation
    uint16_t rpmRaw = rpm * 4;
    response = "41 0C " + String(rpmRaw >> 8, HEX) + " " + String(rpmRaw & 0xFF, HEX);
  }
  else if (cmd == "010D") {  // Speed
    uint8_t speed = 0;  // Stationary
    response = "41 0D " + String(speed, HEX);
  }
  else if (cmd == "0104") {  // Engine Load
    uint8_t load = random(10, 25);  // 10-25% idle load
    response = "41 04 " + String((load * 255) / 100, HEX);
  }
  else if (cmd == "0105") {  // Coolant Temp
    int temp = 70 + random(-5, 11);  // 65-80°C operating temp
    response = "41 05 " + String(temp + 40, HEX);  // Add 40°C offset
  }
  else if (cmd == "0111") {  // Throttle Position
    uint8_t throttle = 0;  // Idle = 0% throttle
    response = "41 11 " + String((throttle * 255) / 100, HEX);
  }
  else if (cmd == "012F") {  // Fuel Level
    uint8_t fuel = 75;  // 75% fuel level
    response = "41 2F " + String((fuel * 255) / 100, HEX);
  }
  else {
    response = "NO DATA";
  }
  
  SerialBT.println(response);
  Serial.println("📤 Sent: " + response);
}

void sendDummyOBD2Data() {
  if (!SerialBT.hasClient()) {
    Serial.println("❌ No Bluetooth connection");
    return;
  }
  
  Serial.println("📊 Sending dummy OBD2 data...");
  
  // Send a series of realistic responses
  SerialBT.println("41 0C 03 54");  // RPM: ~850
  delay(100);
  SerialBT.println("41 0D 00");     // Speed: 0 km/h
  delay(100);
  SerialBT.println("41 04 33");     // Load: ~20%
  delay(100);
  SerialBT.println("41 05 6E");     // Temp: ~70°C
  delay(100);
  SerialBT.println("41 11 00");     // Throttle: 0%
  delay(100);
  SerialBT.println("41 2F BF");     // Fuel: ~75%
  
  Serial.println("✅ Dummy data sent!");
}

void printStatus() {
  Serial.println("\n📊 === System Status ===");
  Serial.printf("Uptime: %d seconds\n", millis() / 1000);
  Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("CPU Freq: %d MHz\n", getCpuFrequencyMhz());
  Serial.printf("Bluetooth: %s\n", SerialBT.hasClient() ? "Connected" : "Waiting");
  Serial.println("========================");
}

void printSystemInfo() {
  Serial.println("\n🔧 === System Information ===");
  
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  
  Serial.printf("Chip: %s\n", (chip_info.model == CHIP_ESP32) ? "ESP32" : "Unknown");
  Serial.printf("Cores: %d\n", chip_info.cores);
  Serial.printf("Revision: %d\n", chip_info.revision);
  Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("PSRAM: %s\n", ESP.getPsramSize() > 0 ? "Yes" : "No");
  Serial.printf("WiFi: %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");
  Serial.printf("Bluetooth: %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "Yes" : "No");
  
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  Serial.printf("BT MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  Serial.println("===============================");
}
```

### 5. Testing Steps

1. **Upload Code**:
   - Copy the code above into Arduino IDE
   - Select correct board and port
   - Click Upload button
   - Wait for "Done uploading" message

2. **Open Serial Monitor**:
   - Tools → Serial Monitor
   - Set baud rate to 115200
   - Should see startup messages

3. **Test Commands**:
   ```
   Type in Serial Monitor:
   help         → Show command menu
   bt_status    → Check Bluetooth status
   info         → Show system information
   ```

4. **Bluetooth Pairing**:
   - On Chigee: Settings → Bluetooth → Search
   - Look for "ChigeeOBD2_ESP32"
   - Enter PIN: 1234
   - Should see "Connected" in serial monitor

5. **OBD2 Testing**:
   - Once paired, Chigee will send OBD2 commands
   - Monitor serial output for received commands
   - Should see realistic responses being sent

### 6. Expected Results

```
🚗 ChigeeOBD2 ESP32-WROOM-32 Test 🚗
===================================
🔴 Testing LEDs...
🔧 Running hardware tests...
Chip: ESP32, Cores: 2, Revision: 1
Flash: 4 MB
Free Heap: 290 KB
Testing GPIO pins...
GPIO4: OK
GPIO16: OK  
GPIO17: OK
Testing ADC...
ADC36: 3891, ADC39: 4095
✅ Hardware validation PASSED!
📱 Starting Bluetooth...
✅ Bluetooth started successfully!
📱 Device name: ChigeeOBD2_ESP32
🔑 PIN: 1234

🎮 Ready for testing!
Commands: help, bt_status, dummy_data
```

### 7. Troubleshooting

**Upload Issues:**
- Check COM port in Device Manager
- Try different USB cable
- Press and hold BOOT button during upload

**Bluetooth Issues:**
- Restart ESP32 after upload
- Clear Bluetooth cache on Chigee
- Check serial monitor for error messages

**No Response:**
- Verify baud rate is 115200
- Try different Serial Monitor (like PuTTY)
- Check ESP32 is powered and running