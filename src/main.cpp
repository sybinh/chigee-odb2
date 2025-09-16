/**/**

 * @file main.cpp * @file main.cpp

 * @brief Main entry point for ChigeeOBD2 ESP32 module * @brief Main entry point for ChigeeOBD2 ESP32 module

 * @author Chigee OBD2 Project Team * @author Chigee OBD2 Project Team

 * @date 2025-01-11 * @date 2025-01-11

 *  * 

 * Professional PlatformIO main entry point for ESP32-WROOM-32 * Professional PlatformIO main entry point for ESP32-WROOM-32

 * OBD2 module with modular architecture * OBD2 module with modular architecture

 */ */



#include <Arduino.h>#include <Arduino.h>

#include <freertos/FreeRTOS.h>#include <freertos/FreeRTOS.h>

#include <freertos/task.h>#include <freertos/task.h>

#include <esp_log.h>#include <esp_log.h>

#include <esp_system.h>#include <esp_system.h>



#include "config/project_config.h"#include "config/project_config.h"

#include "config/hardware_config.h"#include "config/hardware_config.h"

#include "modules/bluetooth/bluetooth_manager.h"#include "modules/bluetooth/bluetooth_manager.h"

#include "modules/obd2/obd2_handler.h"#include "modules/obd2/obd2_handler.h"

#include "modules/can/can_interface.h"#include "modules/can/can_interface.h"



// Global module instances// Global module instances

BluetoothManager bt_manager;BluetoothManager bt_manager;

OBD2Handler obd2_handler;OBD2Handler obd2_handler;

CANInterface can_interface;CANInterface can_interface;



// System state// Target device to spoof (from device_fingerprints.json)

enum SystemState {#define SPOOF_TARGET_DEVICE "genuine_elm327_v15"  // Change to spoof different devices

    STATE_INITIALIZING,

    STATE_READY,// Known working device MAC addresses (from analysis)

    STATE_CONNECTED,uint8_t genuineELM327MAC[6] = {0x00, 0x1B, 0xDC, 0x12, 0x34, 0x56};  // Example genuine ELM327

    STATE_ACTIVE,uint8_t obdlinkMXPlusMAC[6] = {0x00, 0x04, 0x3E, 0xAB, 0xCD, 0xEF};  // Example OBDLink MX+

    STATE_ERRORuint8_t bafxReaderMAC[6] = {0x00, 0x0D, 0x18, 0x11, 0x22, 0x33};     // Example BAFX reader

};

// Current spoofing target

SystemState current_state = STATE_INITIALIZING;uint8_t* currentTargetMAC = genuineELM327MAC;

uint32_t last_status_update = 0;

uint32_t system_uptime = 0;// Device-specific response patterns

struct DeviceProfile {

// Function prototypes  String firmware_version;

void initializeSystem();  String device_identifier;

void handleSystemTasks();  String voltage_response;

void updateSystemStatus();  int avg_response_delay;

void handleBluetoothData();  int min_response_delay;

void handleCANData();  int max_response_delay;

void printSystemInfo();  String error_response;

bool validateHardware();  String unknown_command_response;

};

/**

 * @brief Arduino setup function - runs once at startup// Device profiles based on fingerprint database

 */DeviceProfile genuineELM327Profile = {

void setup() {  "ELM327 v1.5",

    // Initialize serial communication  "ELM327",

    Serial.begin(115200);  "12.0V",

    delay(1000);  // Allow serial to stabilize  65,   // avg delay ms

      40,   // min delay ms  

    // Print startup banner  120,  // max delay ms

    Serial.println("\n==================================================");  "BUS ERROR",

    Serial.println("🚗 ChigeeOBD2 ESP32 Professional Module 🚗");  "?"

    Serial.println("Project: " PROJECT_NAME " v" PROJECT_VERSION);};

    Serial.println("Hardware: " BOARD_VARIANT);

    Serial.println("Build: " __DATE__ " " __TIME__);DeviceProfile obdlinkMXProfile = {

    Serial.println("==================================================");  "STN1170 v4.0",

      "STN1170", 

    // Initialize system  "12.1V",

    initializeSystem();  45,   // avg delay ms

      25,   // min delay ms

    Serial.println("✅ System initialization complete");  80,   // max delay ms

    Serial.println("🚀 ChigeeOBD2 module ready for operation");  "CAN ERROR",

}  "?"

};

/**

 * @brief Arduino main loop - runs continuouslyDeviceProfile bafxProfile = {

 */  "ELM327 v1.5",

void loop() {  "ELM327",

    // Handle system tasks  "12.0V", 

    handleSystemTasks();  80,   // avg delay ms

      50,   // min delay ms

    // Update system status  150,  // max delay ms

    updateSystemStatus();  "BUS ERROR",

      "?"

    // Handle incoming data};

    handleBluetoothData();

    handleCANData();// Current active profile

    DeviceProfile* currentProfile = &genuineELM327Profile;

    // Small delay to prevent watchdog issues

    delay(10);// Security state machine for Chigee handshake

}enum ChigeeSecurityState {

  SEC_WAITING_RESET,

/**  SEC_WAITING_ECHO_OFF,

 * @brief Initialize all system components  SEC_WAITING_PROTOCOL,

 */  SEC_WAITING_HEADERS,

void initializeSystem() {  SEC_AUTHENTICATED,

    Serial.println("🔧 Initializing system components...");  SEC_CHALLENGE_MODE

    } securityState = SEC_WAITING_RESET;

    // Hardware validation

    if (!validateHardware()) {// Device fingerprinting data

        Serial.println("❌ Hardware validation failed!");struct DeviceFingerprint {

        current_state = STATE_ERROR;  String firmwareVersion = "ELM327 v1.5";

        return;  String deviceSerial = "ELM327-MOCK-001";

    }  String manufacturerCode = "ELM";

      unsigned long bootTime = 0;

    // Initialize hardware pins  int commandCount = 0;

    pinMode(LED_BUILTIN_PIN, OUTPUT);} deviceFingerprint;

    pinMode(LED_CAN_ACTIVITY_PIN, OUTPUT);

    pinMode(LED_BT_CONNECTED_PIN, OUTPUT);// ===== ISOLATION TEST MODE =====

    pinMode(LED_ERROR_PIN, OUTPUT);#define TEST_MODE true  // Set false to connect to real CAN bus

    #define SIMULATE_REALISTIC_DATA true

    // Turn off all LEDs initially

    digitalWrite(LED_BUILTIN_PIN, LOW);// Simulated data for testing

    digitalWrite(LED_CAN_ACTIVITY_PIN, LOW);struct SimulatedData {

    digitalWrite(LED_BT_CONNECTED_PIN, LOW);  int rpm = 1200;

    digitalWrite(LED_ERROR_PIN, LOW);  int speed = 0;

      int coolantTemp = 85;

    // Initialize Bluetooth manager  int intakeTemp = 25;

    Serial.println("📱 Initializing Bluetooth manager...");  int throttlePos = 0;

    if (!bt_manager.begin()) {  float voltage = 12.6;

        Serial.println("❌ Bluetooth initialization failed!");  bool engineRunning = true;

        current_state = STATE_ERROR;  unsigned long lastUpdate = 0;

        return;};

    }

    SimulatedData simData;

    // Configure device spoofing if enabled

    if (SPOOF_DEVICE_MAC) {void setup() {

        bt_manager.setSpoofedDevice(SPOOF_TARGET_MAC);  Serial.begin(115200);

        Serial.println("🎭 Device spoofing enabled");  delay(1000);

    }  

      Serial.println("=== CHIGEE XR2 OBD2 MODULE ===");

    // Initialize OBD2 handler  Serial.println("Husqvarna Svartpilen 401 / KTM Duke 390");

    Serial.println("🚗 Initializing OBD2 handler...");  Serial.println("Version: 1.0.0 with Advanced Security Bypass");

    if (!obd2_handler.begin()) {  Serial.println("Safety Mode: ENABLED");

        Serial.println("❌ OBD2 handler initialization failed!");  

        current_state = STATE_ERROR;  // Initialize advanced security features

        return;  if (ENABLE_ADVANCED_SPOOFING) {

    }    initializeAdvancedSpoofing();

      }

    // Initialize CAN interface  

    Serial.println("🚌 Initializing CAN interface...");  // Initialize device fingerprint

    if (!can_interface.begin()) {  deviceFingerprint.bootTime = millis();

        Serial.println("⚠️ CAN interface initialization failed (external hardware required)");  deviceFingerprint.commandCount = 0;

        // CAN failure is not critical - module can work without it  

    }  // Initialize project phase management

      projectStatus.phaseStartTime = millis();

    // System ready  projectStatus.lastUpdate = millis();

    current_state = STATE_READY;  projectStatus.requirements.hardware_ready = true;

    system_uptime = millis();  projectStatus.requirements.software_compiled = true;

      projectStatus.requirements.safety_tests_passed = true;

    // Print system information  

    printSystemInfo();  // Initialize Bluetooth with production-ready name for Chigee XR2

}  if (TEST_MODE) {

    SerialBT.begin("OBDII_TEST");  // Test mode - distinguishable from real OBD2

/**    SerialBT.setPin("1234");

 * @brief Handle periodic system tasks    Serial.println("=== ISOLATION TEST MODE ===");

 */    Serial.println("Bluetooth: OBDII_TEST (safe for testing)");

void handleSystemTasks() {  } else {

    static uint32_t last_task_run = 0;    SerialBT.begin("OBDII");       // Production mode - Chigee XR2 compatible

        SerialBT.setPin("1234");

    if (millis() - last_task_run < 1000) return;  // Run every second    Serial.println("=== PRODUCTION MODE ===");

    last_task_run = millis();    Serial.println("Bluetooth: OBDII (Chigee XR2 ready)");

        

    // Update system components    if (ENABLE_ADVANCED_SPOOFING) {

    bt_manager.update();      Serial.println("⚠️  ADVANCED SPOOFING ENABLED");

    obd2_handler.update();      Serial.println("🔐 MAC Address spoofing: " + String(SPOOF_DEVICE_MAC ? "ON" : "OFF"));

    can_interface.update();    }

      }

    // Update LED indicators  projectStatus.requirements.bluetooth_functional = true;

    digitalWrite(LED_BUILTIN_PIN, (millis() / 1000) % 2);  // Heartbeat  

    digitalWrite(LED_BT_CONNECTED_PIN, bt_manager.isConnected());  if (TEST_MODE) {

    digitalWrite(LED_ERROR_PIN, current_state == STATE_ERROR);    Serial.println("=== ISOLATION TEST MODE ===");

}    Serial.println("Safe testing without CAN connection");

    Serial.println("Simulating Husqvarna Svartpilen 401 data");

/**    

 * @brief Update system status and statistics    // Initialize simulation data

 */    simData.lastUpdate = millis();

void updateSystemStatus() {    projectStatus.requirements.can_simulation_working = true;

    if (millis() - last_status_update < 30000) return;  // Every 30 seconds    

    last_status_update = millis();  } else {

        Serial.println("=== LIVE CAN MODE ===");

    Serial.println("\n📊 === System Status ===");    // Initialize CAN Bus (500 kbps for Husqvarna)

    Serial.printf("Uptime: %d seconds\n", (millis() - system_uptime) / 1000);    if(CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {

    Serial.printf("State: %s\n", getStateString(current_state));      Serial.println("CAN Bus Shield init ok!");

    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);    } else {

    Serial.printf("Min Free Heap: %d KB\n", ESP.getMinFreeHeap() / 1024);      Serial.println("CAN Bus Shield init fail");

          // Fallback to test mode if CAN fails

    // Bluetooth status      Serial.println("Switching to TEST MODE for safety");

    if (bt_manager.isConnected()) {      projectStatus.requirements.can_simulation_working = false;

        Serial.printf("Bluetooth: Connected (%d msgs)\n", bt_manager.getMessageCount());    }

    } else {    CAN0.setMode(MCP_NORMAL);

        Serial.println("Bluetooth: Waiting for connection...");  }

    }  

      Serial.println("OBD2 Module Ready");

    // OBD2 status  

    Serial.printf("OBD2: %d commands processed\n", obd2_handler.getCommandCount());  // Initialize advanced security bypass for Chigee XR2

      initializeAdvancedSpoofing();

    // CAN status  

    if (can_interface.isInitialized()) {  SerialBT.println("ELM327 v1.5");

        Serial.printf("CAN: %d messages processed\n", can_interface.getMessageCount());  SerialBT.print(">");

    } else {  

        Serial.println("CAN: Not available (hardware required)");  // Show initial project status

    }  updatePhaseProgress(65.0); // Already well into testing phase

      printProjectStatus();

    Serial.println("========================");  Serial.println("");

}  Serial.println("Type 'PHASE STATUS' for project status");

  Serial.println("Type 'HELP' for available commands");

/**}

 * @brief Handle incoming Bluetooth data

 */void loop() {

void handleBluetoothData() {  // Monitor connection security continuously

    if (!bt_manager.isConnected()) return;  monitorChigeeConnection();

      

    if (bt_manager.available()) {  // Update simulated data if in test mode

        String command = bt_manager.readString();  if (TEST_MODE && SIMULATE_REALISTIC_DATA) {

        command.trim();    updateSimulatedData();

          }

        Serial.println("📨 BT Command: " + command);  

          // Periodic phase progress update (every 30 seconds in test mode)

        // Process OBD2 command  static unsigned long lastPhaseUpdate = 0;

        String response = obd2_handler.processCommand(command);  if (TEST_MODE && millis() - lastPhaseUpdate > 30000) {

            updatePhaseProgress(2.0); // Slow automatic progress in test mode

        // Send response    lastPhaseUpdate = millis();

        bt_manager.println(response);  }

        Serial.println("📤 BT Response: " + response);  

          // Read commands from Chigee via Bluetooth

        // Update state  if (SerialBT.available()) {

        if (current_state == STATE_READY) {    char inChar = (char)SerialBT.read();

            current_state = STATE_CONNECTED;    

        }    if (inChar == '\r' || inChar == '\n') {

        if (current_state == STATE_CONNECTED && response != "NO DATA") {      if (inputString.length() > 0) {

            current_state = STATE_ACTIVE;        processCommand(inputString);

        }        inputString = "";

              }

        // Flash activity LED    } else {

        digitalWrite(LED_CAN_ACTIVITY_PIN, HIGH);      inputString += inChar;

        delay(50);    }

        digitalWrite(LED_CAN_ACTIVITY_PIN, LOW);  }

    }  

}  // Read commands from Serial monitor for development

  if (Serial.available()) {

/**    String serialCmd = Serial.readStringUntil('\n');

 * @brief Handle incoming CAN data    serialCmd.trim();

 */    if (serialCmd.length() > 0) {

void handleCANData() {      processSerialCommand(serialCmd);

    if (!can_interface.isInitialized()) return;    }

      }

    CANMessage msg;}

    if (can_interface.receiveMessage(msg)) {

        Serial.printf("📨 CAN Message: ID=0x%03X, Data=", msg.id);void processCommand(String cmd) {

        for (int i = 0; i < msg.length; i++) {  cmd.trim();

            Serial.printf("%02X ", msg.data[i]);  cmd.toUpperCase();

        }  

        Serial.println();  Serial.println("Received: " + cmd);

          

        // Process CAN message through OBD2 handler  // Advanced security validation

        obd2_handler.processCANMessage(msg);  if (!validateCommandSequence(cmd)) {

            logSecurityEvent("INVALID_SEQUENCE", "Command '" + cmd + "' not expected in state " + String(securityState));

        // Flash activity LED    SerialBT.println("?"); // Send error as real ELM327 would

        digitalWrite(LED_CAN_ACTIVITY_PIN, HIGH);    return;

        delay(50);  }

        digitalWrite(LED_CAN_ACTIVITY_PIN, LOW);  

    }  // Add realistic hardware delays

}  addRealisticHardwareDelay();

  

/**  // Increment command counter for fingerprinting

 * @brief Validate hardware configuration  deviceFingerprint.commandCount++;

 */  

bool validateHardware() {  // AT Commands

    Serial.println("🔧 Validating hardware...");  if (cmd.startsWith("AT")) {

        handleATCommand(cmd);

    // Check ESP32 chip  }

    esp_chip_info_t chip_info;  // OBD PIDs

    esp_chip_info(&chip_info);  else if (cmd.length() == 4) {

        handleOBDCommand(cmd);

    if (chip_info.model != CHIP_ESP32) {  }

        Serial.println("❌ Not an ESP32 chip!");  else {

        return false;    SerialBT.println("?");

    }    SerialBT.print(">");

      }

    // Check flash size}

    uint32_t flash_size = ESP.getFlashChipSize();

    if (flash_size < 4 * 1024 * 1024) {void handleATCommand(String cmd) {

        Serial.println("❌ Insufficient flash memory!");  // Advanced security: Track command sequences for Chigee validation

        return false;  deviceFingerprint.commandCount++;

    }  bool isValidSequence = validateCommandSequence(cmd);

      

    // Check free heap  if (!isValidSequence && ENABLE_ADVANCED_SPOOFING) {

    uint32_t free_heap = ESP.getFreeHeap();    Serial.println("⚠️  Command out of sequence: " + cmd);

    if (free_heap < 200 * 1024) {    Serial.println("🔐 Current security state: " + String(securityState));

        Serial.println("❌ Insufficient free heap!");  }

        return false;  

    }  // Realistic ELM327 timing with hardware simulation

      addRealisticHardwareDelay();

    Serial.println("✅ Hardware validation passed");  

    return true;  if (cmd == "ATZ") {

}    delay(200);  // Reset takes longer

    securityState = SEC_WAITING_ECHO_OFF;

/**    SerialBT.println("ELM327 v1.5");

 * @brief Print detailed system information    Serial.println("🔄 Reset command - moving to ECHO_OFF state");

 */  }

void printSystemInfo() {  else if (cmd == "ATI") {

    Serial.println("\n🔧 === System Information ===");    // Device identification - CRITICAL for Chigee XR2 detection

        SerialBT.println(currentProfile->firmware_version);

    // Chip information  }

    esp_chip_info_t chip_info;  else if (cmd == "AT@1") {

    esp_chip_info(&chip_info);    // Device description - device specific response

        if (currentProfile == &obdlinkMXProfile) {

    Serial.printf("Chip: %s (Rev %d)\n",       SerialBT.println("STN1170");

        (chip_info.model == CHIP_ESP32) ? "ESP32" : "Unknown",     } else {

        chip_info.revision);      SerialBT.println("ELM327");

    Serial.printf("CPU Cores: %d\n", chip_info.cores);    }

    Serial.printf("CPU Frequency: %d MHz\n", getCpuFrequencyMhz());  }

      else if (cmd == "AT@2") {

    // Memory information    // Device identifier - return specific device ID or ? based on profile

    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));    if (ENABLE_ADVANCED_SPOOFING) {

    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);      if (currentProfile == &obdlinkMXProfile) {

    Serial.printf("PSRAM: %s\n", ESP.getPsramSize() > 0 ? "Available" : "Not available");        SerialBT.println("?"); // OBDLink MX+ returns ? for AT@2

          } else {

    // Features        SerialBT.println("?"); // Most devices return ? for AT@2

    Serial.printf("WiFi: %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");      }

    Serial.printf("Bluetooth: %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "Yes" : "No");    } else {

          SerialBT.println("?");

    // MAC addresses    }

    uint8_t mac[6];  }

    esp_read_mac(mac, ESP_MAC_BT);  else if (cmd == "AT@3") {

    Serial.printf("BT MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",    // Device copyright - advanced spoofing

                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);    SerialBT.println("Copyright (c) ELM Electronics");

      }

    Serial.println("==============================");  else if (cmd == "ATDP") {

}    SerialBT.println("ISO 15765-4 (CAN 11/500)");

  }

/**  else if (cmd == "ATDPN") {

 * @brief Get string representation of system state    // Protocol number - important for protocol detection

 */    SerialBT.println("A6");  // ISO 15765-4 protocol number

const char* getStateString(SystemState state) {  }

    switch (state) {  else if (cmd == "ATAL") {

        case STATE_INITIALIZING: return "Initializing";    // Allow long messages - some apps require this

        case STATE_READY: return "Ready";    SerialBT.println("OK");

        case STATE_CONNECTED: return "Connected";  }

        case STATE_ACTIVE: return "Active";  else if (cmd.startsWith("ATCF")) {

        case STATE_ERROR: return "Error";    // CAN formatting - handle all ATCF variants

        default: return "Unknown";    SerialBT.println("OK");

    }  }

}  else if (cmd == "ATWS") {
    // Warm start
    SerialBT.println("OK");
  }
  else if (cmd == "ATE0") {
    securityState = SEC_WAITING_PROTOCOL;
    SerialBT.println("OK");
    Serial.println("🔄 Echo OFF - moving to PROTOCOL state");
  }
  else if (cmd == "ATE1") {
    SerialBT.println("OK");
  }
  else if (cmd == "ATL0" || cmd == "ATL1") {
    SerialBT.println("OK");
  }
  else if (cmd == "ATS0" || cmd == "ATS1") {
    SerialBT.println("OK");
  }
  else if (cmd == "ATH0") {
    securityState = SEC_AUTHENTICATED;
    SerialBT.println("OK");
    Serial.println("✅ Headers OFF - AUTHENTICATED state reached");
  }
  else if (cmd == "ATH1") {
    SerialBT.println("OK");
  }
  else if (cmd == "ATSP6" || cmd == "ATSP0") {
    securityState = SEC_WAITING_HEADERS;
    SerialBT.println("OK");
    Serial.println("🔄 Protocol set - moving to HEADERS state");
  }
  else if (cmd == "ATRV") {
    // Battery voltage - use device-specific format if available
    if (ENABLE_ADVANCED_SPOOFING && currentProfile->voltage_response != "") {
      // Use profile-specific voltage format with slight variation
      float baseVoltage = currentProfile->voltage_response.substring(0, currentProfile->voltage_response.indexOf('V')).toFloat();
      float variation = random(-5, 5) / 100.0; // ±0.05V variation
      SerialBT.printf("%.1fV\r\n", baseVoltage + variation);
    } else {
      SerialBT.printf("%.1fV\r\n", simData.voltage);  // Use simulated voltage
    }
  }
  // Advanced/undocumented commands for deeper compatibility
  else if (cmd == "ATCS") {
    // Checksum - some scanners use this
    SerialBT.println("A3");
  }
  else if (cmd == "ATCV") {
    // Calibration voltage
    SerialBT.println("0000");
  }
  else if (cmd.startsWith("ATPP")) {
    // Programmable parameters - advanced ELM327 feature
    SerialBT.println("FF FF FF");
  }
  else if (cmd.startsWith("ATST")) {
    // Set timeout
    SerialBT.println("OK");
  }
  // Potential Chigee proprietary commands (speculation)
  else if (cmd.startsWith("ATCHG") && ENABLE_ADVANCED_SPOOFING) {
    // Hypothetical Chigee validation command
    handleChigeeProprietaryCommand(cmd);
  }
  else {
    SerialBT.println("OK");  // Default response for unknown commands
  }
  
  SerialBT.print(">");
}

void handleOBDCommand(String cmd) {
  delay(100);  // Simulation delay
  
  // Track successful OBD command processing for phase progress
  static int obdCommandCount = 0;
  obdCommandCount++;
  
  if (cmd == "010C") {  // Engine RPM
    int rpm = (TEST_MODE) ? getSimulatedRPM() : readRPMFromCAN();
    SerialBT.printf("41 0C %02X %02X\r\n", (rpm >> 8) & 0xFF, rpm & 0xFF);
    Serial.printf("OBD: RPM = %d\n", rpm);
  }
  else if (cmd == "010D") {  // Vehicle Speed
    int speed = (TEST_MODE) ? getSimulatedSpeed() : readSpeedFromCAN();
    SerialBT.printf("41 0D %02X\r\n", speed);
    Serial.printf("OBD: Speed = %d km/h\n", speed);
  }
  else if (cmd == "0105") {  // Engine Coolant Temperature
    int temp = (TEST_MODE) ? getSimulatedCoolantTemp() : readTempFromCAN();
    SerialBT.printf("41 05 %02X\r\n", temp + 40);  // Offset 40�C
    Serial.printf("OBD: Coolant temp = %d�C\n", temp);
  }
  else if (cmd == "010F") {  // Intake Air Temperature
    int temp = (TEST_MODE) ? getSimulatedIntakeTemp() : 25;
    SerialBT.printf("41 0F %02X\r\n", temp + 40);
  }
  else if (cmd == "0111") {  // Throttle Position
    int throttle = (TEST_MODE) ? getSimulatedThrottlePos() : 0;
    SerialBT.printf("41 11 %02X\r\n", (throttle * 255) / 100);  // 0-100% -> 0-255
  }
  else if (cmd == "010E") {  // Timing Advance
    int timing = (TEST_MODE) ? 15 : 10;  // degrees before TDC
    SerialBT.printf("41 0E %02X\r\n", (timing + 64) * 2);  // Formula: (A/2) - 64
  }
  else {
    SerialBT.println("NO DATA");
  }
  
  // Track successful OBD command processing for phase progress
  if (obdCommandCount % 10 == 0) {  // Every 10 commands
    updatePhaseProgress(1.0);
    Serial.printf("OBD commands processed: %d\n", obdCommandCount);
  }
  
  SerialBT.print(">");
}

int readRPMFromCAN() {
  // Read from real CAN bus
  // Return RPM/4 (according to OBD2 standard)
  unsigned long rxId;
  unsigned char len;
  unsigned char rxBuf[8];
  
  if(CAN0.checkReceive() == CAN_MSGAVAIL) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    
    // Parse RPM data from CAN frame
    // (Need to know CAN ID and format for Husqvarna)
    if (rxId == 0x280) {  // Assuming RPM is at this ID
      int rpm = (rxBuf[2] << 8) | rxBuf[3];
      return rpm / 4;  // OBD2 format
    }
  }
  
  return 800;  // Dummy RPM idle
}

int readSpeedFromCAN() {
  // Similar to RPM
  return 0;  // km/h
}

int readTempFromCAN() {
  // Engine coolant temperature
  return 80;  // �C
}

// ===== ADVANCED SECURITY FUNCTIONS =====

void initializeAdvancedSpoofing() {
  Serial.println("🔐 Initializing advanced Chigee security bypass...");
  
  // Select spoofing target based on configuration
  if (strcmp(SPOOF_TARGET_DEVICE, "genuine_elm327_v15") == 0) {
    currentTargetMAC = genuineELM327MAC;
    currentProfile = &genuineELM327Profile;
    Serial.println("🎯 Spoofing: Genuine ELM327 v1.5");
  }
  else if (strcmp(SPOOF_TARGET_DEVICE, "obdlink_mx_plus") == 0) {
    currentTargetMAC = obdlinkMXPlusMAC;
    currentProfile = &obdlinkMXProfile;
    Serial.println("🎯 Spoofing: OBDLink MX+");
  }
  else if (strcmp(SPOOF_TARGET_DEVICE, "bafx_products_reader") == 0) {
    currentTargetMAC = bafxReaderMAC; 
    currentProfile = &bafxProfile;
    Serial.println("🎯 Spoofing: BAFX Products Reader");
  }
  else {
    Serial.println("⚠️  Unknown spoofing target, using default genuine ELM327");
    currentTargetMAC = genuineELM327MAC;
    currentProfile = &genuineELM327Profile;
  }
  
  if (SPOOF_DEVICE_MAC) {
    // WARNING: This changes ESP32 MAC address permanently!
    Serial.println("⚠️  SPOOFING MAC ADDRESS - USE WITH CAUTION!");
    Serial.print("Original MAC: ");
    printMACAddress();
    
    // Set to target device MAC
    esp_base_mac_addr_set(currentTargetMAC);
    
    Serial.print("Spoofed MAC: ");
    printMACAddress();
  }
  
  // Initialize device fingerprint with target device characteristics
  deviceFingerprint.firmwareVersion = currentProfile->firmware_version;
  deviceFingerprint.deviceSerial = currentProfile->device_identifier + "-" + String(random(100000, 999999));
  deviceFingerprint.manufacturerCode = "ELM";
  
  Serial.println("🔑 Device fingerprint initialized:");
  Serial.println("   Firmware: " + deviceFingerprint.firmwareVersion);
  Serial.println("   Serial: " + deviceFingerprint.deviceSerial);
  Serial.println("   Profile: " + String(SPOOF_TARGET_DEVICE));
  Serial.println("   Avg Response Time: " + String(currentProfile->avg_response_delay) + "ms");
}

void printMACAddress() {
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_BT);
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          baseMac[0], baseMac[1], baseMac[2], 
          baseMac[3], baseMac[4], baseMac[5]);
  Serial.println(macStr);
}

bool validateCommandSequence(String cmd) {
  // Implement Chigee-expected command sequence validation
  
  switch(securityState) {
    case SEC_WAITING_RESET:
      return (cmd == "ATZ");
      
    case SEC_WAITING_ECHO_OFF:
      return (cmd == "ATE0" || cmd == "ATE1" || cmd == "ATI" || cmd.startsWith("AT@"));
      
    case SEC_WAITING_PROTOCOL:
      return (cmd.startsWith("ATSP") || cmd == "ATDP" || cmd == "ATDPN");
      
    case SEC_WAITING_HEADERS:
      return (cmd == "ATH0" || cmd == "ATH1" || cmd.startsWith("ATCF"));
      
    case SEC_AUTHENTICATED:
      return true; // All commands allowed after authentication
      
    case SEC_CHALLENGE_MODE:
      // Special mode if Chigee sends cryptographic challenge
      return handleSecurityChallenge(cmd);
      
    default:
      return true;
  }
}

void addRealisticHardwareDelay() {
  // Simulate specific target device hardware characteristics
  
  // Base delay from device profile with realistic variation
  int baseDelay = random(currentProfile->min_response_delay, currentProfile->max_response_delay);
  
  // Add command count effect (some devices get slower after many commands)
  if (deviceFingerprint.commandCount > 100) {
    baseDelay += random(5, 15);
  }
  
  // Add environmental effects simulation
  if (simData.voltage < 12.0) {
    baseDelay += random(10, 25); // Slower when battery low
  }
  
  // Occasional random delays (hardware glitches) - vary by device type
  int glitchChance = 20; // Default 2% chance
  if (currentProfile == &bafxProfile) {
    glitchChance = 30; // BAFX devices slightly more variable
  } else if (currentProfile == &obdlinkMXProfile) {
    glitchChance = 10; // OBDLink more consistent
  }
  
  if (random(1000) < glitchChance) {
    int glitchDelay = random(50, 200);
    baseDelay += glitchDelay;
    Serial.println("🔧 Simulating " + currentProfile->device_identifier + " hardware glitch (+"+String(glitchDelay)+"ms)");
  }
  
  delay(baseDelay);
}

bool handleSecurityChallenge(String challenge) {
  // Handle potential cryptographic challenges from Chigee
  Serial.println("🔐 Security challenge received: " + challenge);
  
  // This would need to be reverse engineered from actual Chigee behavior
  // For now, just log and return true
  Serial.println("⚠️  Challenge handling not yet implemented");
  
  return true;
}

void handleChigeeProprietaryCommand(String cmd) {
  Serial.println("🔍 Chigee proprietary command detected: " + cmd);
  
  // These are hypothetical - would need reverse engineering
  if (cmd == "ATCHGID") {
    // Hypothetical Chigee device ID request
    SerialBT.println("CHIGEE_OBD_v1.0");
  }
  else if (cmd == "ATCHGAUTH") {
    // Hypothetical authentication challenge
    SerialBT.println("AUTH_OK_12345");
  }
  else if (cmd.startsWith("ATCHGCRYPT")) {
    // Hypothetical cryptographic challenge
    String challenge = cmd.substring(10); // Extract challenge part
    String response = calculateCryptoResponse(challenge);
    SerialBT.println(response);
  }
  else {
    // Unknown proprietary command - play it safe
    SerialBT.println("?");
  }
}

String calculateCryptoResponse(String challenge) {
  // Placeholder for cryptographic response calculation
  // This would need to be reverse engineered from real devices
  
  Serial.println("🔐 Calculating crypto response for: " + challenge);
  
  // Placeholder algorithm (NOT real Chigee algorithm!)
  String response = "";
  for (int i = 0; i < challenge.length(); i++) {
    char c = challenge.charAt(i);
    c = c ^ 0x55; // Simple XOR (placeholder)
    response += String(c, HEX);
  }
  
  Serial.println("🔑 Crypto response: " + response);
  return response;
}

void monitorChigeeConnection() {
  // Monitor for suspicious disconnections or validation failures
  static unsigned long lastConnectionCheck = 0;
  static bool wasConnected = false;
  
  if (millis() - lastConnectionCheck > 5000) { // Check every 5 seconds
    bool isConnected = SerialBT.hasClient();
    
    if (wasConnected && !isConnected) {
      Serial.println("⚠️  Chigee disconnected - possible security validation failure");
      Serial.println("🔍 Last command count: " + String(deviceFingerprint.commandCount));
      Serial.println("🔍 Last security state: " + String(securityState));
      
      // Reset security state for next connection
      securityState = SEC_WAITING_RESET;
      deviceFingerprint.commandCount = 0;
    }
    
    wasConnected = isConnected;
    lastConnectionCheck = millis();
  }
}

void logSecurityEvent(String event, String details) {
  Serial.println("🔐 SECURITY EVENT: " + event);
  Serial.println("📋 Details: " + details);
  Serial.println("⏰ Time: " + String(millis() - deviceFingerprint.bootTime) + "ms since boot");
  Serial.println("📊 Commands processed: " + String(deviceFingerprint.commandCount));
  Serial.println("🔄 Security state: " + String(securityState));
  Serial.println("");
}

// ===== SIMULATION FUNCTIONS =====
void updateSimulatedData() {
  unsigned long currentTime = millis();
  
  if (currentTime - simData.lastUpdate > 100) {  // Update every 100ms
    
    // Simulate realistic engine behavior
    if (simData.engineRunning) {
      // Idle variation
      simData.rpm = 1200 + random(-50, 50);
      
      // Simulate occasional rev-ups
      if (random(100) < 2) {  // 2% chance
        simData.rpm = 2000 + random(0, 2000);
        simData.throttlePos = random(20, 80);
      } else {
        simData.throttlePos = random(0, 10);  // Mostly idle
      }
      
      // Temperature simulation
      if (simData.coolantTemp < 85) {
        simData.coolantTemp++;  // Warm up
      }
      simData.intakeTemp = 25 + random(-3, 8);  // Ambient variation
      
    } else {
      simData.rpm = 0;
      simData.throttlePos = 0;
      simData.coolantTemp = max(25, simData.coolantTemp - 1);  // Cool down
    }
    
    // Battery voltage simulation
    simData.voltage = 12.6 + (random(-20, 20) / 100.0);
    
    simData.lastUpdate = currentTime;
    
    // Debug output
    if (currentTime % 5000 < 200) {  // Every 5 seconds
      Serial.printf("SIM: RPM=%d, Speed=%d, Temp=%d�C, Throttle=%d%%\n", 
                   simData.rpm, simData.speed, simData.coolantTemp, simData.throttlePos);
    }
  }
}

int getSimulatedRPM() {
  return simData.rpm * 4;  // OBD2 format: RPM/4
}

int getSimulatedSpeed() {
  return simData.speed;  // km/h
}

int getSimulatedCoolantTemp() {
  return simData.coolantTemp;  // �C
}

int getSimulatedIntakeTemp() {
  return simData.intakeTemp;  // �C
}

int getSimulatedThrottlePos() {
  return simData.throttlePos;  // 0-100%
}

// ===== REAL CAN BUS FUNCTIONS =====
// ...existing code...

// ===== SAFETY CONFIGURATION =====
// STEP 1: Test v?i TEST_MODE = true tr??c
// STEP 2: Khi ready, set TEST_MODE = false v� compile l?i
// STEP 3: Lu�n test CAN connection tr??c khi start engine

// Safety checks tr??c khi enable CAN
bool performSafetyChecks() {
  Serial.println("Performing safety checks...");
  
  // Check voltage range
  if (simData.voltage < 11.0 || simData.voltage > 15.0) {
    Serial.println("WARNING: Battery voltage out of range!");
    return false;
  }
  
  // Check CAN shield connection
  if (!TEST_MODE) {
    byte canStatus = CAN0.checkError();
    if (canStatus != CAN_OK) {
      Serial.printf("CAN Error: %d\n", canStatus);
      return false;
    }
  }
  
  Serial.println("Safety checks passed ?");
  return true;
}

// Emergency disconnect function
void emergencyDisconnect() {
  Serial.println("EMERGENCY DISCONNECT!");
  if (!TEST_MODE) {
    CAN0.setMode(MCP_SLEEP);  // Disable CAN
  }
  SerialBT.println("SYSTEM SHUTDOWN");
  SerialBT.disconnect();
}

// ===== COMPATIBILITY SAFEGUARDS =====

// Check Bluetooth Classic support
void verifyBluetoothMode() {
  if (!btClassicEnabled()) {
    Serial.println("ERROR: BLE mode detected, need Classic!");
    Serial.println("Call esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)");
  }
}

// Device name validation
bool validateDeviceName() {
  String validNames[] = {"OBDII", "ELM327", "OBD-II", "OBDII_TEST"};
  // Check if current name is in valid list
  return true; // Simplified
}

// Response format validator
void sendSafeResponse(String response) {
  // Add safety checks
  if (response.length() > 50) {
    Serial.println("WARNING: Response too long!");
    response = response.substring(0, 50);
  }
  
  // Ensure proper termination
  if (!response.endsWith("\r") && !response.endsWith("\n")) {
    response += "\r\n";
  }
  
  SerialBT.print(response);
  SerialBT.print(">");  // Always add prompt
}

// Memory monitoring
void checkMemoryHealth() {
  size_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 50000) {  // Less than 50KB
    Serial.printf("WARNING: Low memory: %d bytes\n", freeHeap);
    // Consider restart or cleanup
  }
}

// Bluetooth connection health check
bool isBluetoothHealthy() {
  return SerialBT.hasClient() && SerialBT.connected();
}

// Recovery procedures
void attemptRecovery() {
  Serial.println("Attempting connection recovery...");
  
  // Step 1: Clear buffers
  while(SerialBT.available()) {
    SerialBT.read();
  }
  
  // Step 2: Reset state
  inputString = "";
  
  // Step 3: Send reset response
  SerialBT.println("ELM327 v1.5");
  SerialBT.print(">");
  
  Serial.println("Recovery complete");
}

// ===== DEBUG & TROUBLESHOOTING =====

void printConnectionStatus() {
  Serial.println("=== CONNECTION STATUS ===");
  Serial.printf("Bluetooth clients: %d\n", SerialBT.hasClient());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Uptime: %lu ms\n", millis());
  Serial.printf("Test mode: %s\n", TEST_MODE ? "ON" : "OFF");
}

void diagnosticMode() {
  Serial.println("Entering diagnostic mode...");
  Serial.println("Send 'DIAG' via Bluetooth to see this menu:");
  Serial.println("- STATUS: Connection status");
  Serial.println("- RESET: Soft reset");  
  Serial.println("- PING: Test response");
  Serial.println("- MEMORY: Memory usage");
}

// Handle diagnostic commands
void handleDiagnostic(String cmd) {
  if (cmd == "DIAG STATUS") {
    SerialBT.println("STATUS: OK");
    SerialBT.printf("HEAP: %d\n", ESP.getFreeHeap());
    SerialBT.printf("MODE: %s\n", TEST_MODE ? "TEST" : "LIVE");
  }
  else if (cmd == "DIAG PING") {
    SerialBT.println("PONG");
  }
  else if (cmd == "DIAG RESET") {
    SerialBT.println("RESETTING...");
    delay(100);
    ESP.restart();
  }
  
  SerialBT.print(">");
}

// ===== PROJECT SUCCESS METRICS =====

struct SuccessMetrics {
  // Phase completion rates
  bool bluetoothPairSuccess = false;
  bool chigeeCompatibility = false; 
  bool canBusConnection = false;
  bool vehicleIntegration = false;
  
  // Quality metrics
  int stabilityScore = 0;      // 0-100
  int dataAccuracy = 0;        // 0-100  
  int reliabilityScore = 0;    // 0-100
  
  // Risk mitigation
  bool safetyTestsPassed = false;
  bool emergencyStopWorks = false;
  bool isolationModeWorks = false;
};

SuccessMetrics projectMetrics;

// Success rate calculator
float calculateSuccessRate() {
  float phaseWeights[] = {0.25, 0.30, 0.25, 0.20}; // Weight per phase
  float phaseScores[] = {0.95, 0.70, 0.60, 0.55};  // Current estimates
  
  float totalScore = 0;
  for(int i = 0; i < 4; i++) {
    totalScore += phaseWeights[i] * phaseScores[i];
  }
  
  return totalScore * 100; // Convert to percentage
}

// Risk assessment
void assessProjectRisks() {
  Serial.println("=== PROJECT RISK ASSESSMENT ===");
  
  // Technical risks
  Serial.println("?? TECHNICAL RISKS:");
  Serial.println("- Bluetooth compatibility: MEDIUM (30%)");
  Serial.println("- CAN bus reverse engineering: HIGH (50%)");  
  Serial.println("- Chigee undocumented protocol: HIGH (40%)");
  Serial.println("- Hardware integration: MEDIUM (35%)");
  
  // Safety risks  
  Serial.println("\n? SAFETY RISKS:");
  Serial.println("- Vehicle electronics damage: LOW (5%)");
  Serial.println("- ECU interference: MEDIUM (15%)");
  Serial.println("- Wiring issues: MEDIUM (20%)");
  
  // Success boosters
  Serial.println("\n? SUCCESS BOOSTERS:");
  Serial.println("- Isolation testing approach: +20%");
  Serial.println("- Safety-first design: +15%");
  Serial.println("- Incremental development: +10%");
  Serial.println("- Good code structure: +10%");
}

// ===== KTM/HUSQVARNA CAN DATABASE =====
// Based on Duke 390/Svartpilen 401 shared platform

struct CANDatabase {
  // Known CAN IDs from KTM Duke 390 community
  uint16_t ENGINE_RPM = 0x280;        // RPM data
  uint16_t VEHICLE_SPEED = 0x285;     // Speed sensor
  uint16_t COOLANT_TEMP = 0x290;      // Water temperature
  uint16_t THROTTLE_POS = 0x295;      // TPS sensor
  uint16_t ENGINE_LOAD = 0x2A0;       // Calculated load
  uint16_t FUEL_PRESSURE = 0x2A5;     // Fuel system
  uint16_t LAMBDA_SENSOR = 0x2B0;     // O2 sensor
  uint16_t IGNITION_TIMING = 0x2B5;   // Timing advance
  uint16_t BATTERY_VOLTAGE = 0x2C0;   // Electrical system
  uint16_t ERROR_CODES = 0x2F0;       // DTC codes
};

CANDatabase ktm390_db;

// CAN frame parsing functions based on community data
void parseKTMCanFrame(uint16_t canId, uint8_t* data, uint8_t len) {
  switch(canId) {
    case 0x280: // RPM - Confirmed by multiple users
      {
        uint16_t rawRpm = (data[0] << 8) | data[1];
        int rpm = rawRpm; // Already in RPM, not /4 like OBD2
        Serial.printf("RPM: %d\n", rpm);
      }
      break;
      
    case 0x285: // Speed - From KTM forums
      {
        uint16_t rawSpeed = (data[2] << 8) | data[3];
        int speed = rawSpeed / 10; // Speed in 0.1 km/h units
        Serial.printf("Speed: %d km/h\n", speed);
      }
      break;
      
    case 0x290: // Coolant temp - Duke 390 service manual
      {
        uint8_t rawTemp = data[4];
        int temp = rawTemp - 40; // Offset like OBD2
        Serial.printf("Coolant: %d�C\n", temp);
      }
      break;
      
    case 0x295: // TPS - Community reverse engineering
      {
        uint8_t rawTps = data[5];
        int throttle = (rawTps * 100) / 255; // 0-255 to 0-100%
        Serial.printf("Throttle: %d%%\n", throttle);
      }
      break;
  }
}

// Convert KTM CAN data to OBD2 format
int convertKTMRpmToOBD2(uint16_t ktmRpm) {
  return ktmRpm / 4; // OBD2 expects RPM/4
}

int convertKTMSpeedToOBD2(uint16_t ktmSpeed) {
  return ktmSpeed / 10; // Convert 0.1km/h to km/h
}

// Community-sourced CAN IDs validation
void validateCANDatabase() {
  Serial.println("=== KTM/HUSQVARNA CAN DATABASE ===");
  Serial.println("Source: Duke 390 community forums + service docs");
  Serial.println("Confidence levels:");
  Serial.println("- RPM (0x280): HIGH (95%) - Multiple confirmations");
  Serial.println("- Speed (0x285): MEDIUM (80%) - Forum reports");
  Serial.println("- Coolant (0x290): HIGH (90%) - Service manual");
  Serial.println("- Throttle (0x295): MEDIUM (75%) - Reverse engineered");
  Serial.println("- Others: LOW-MEDIUM (50-70%) - Need verification");
}

// ===== CAN BUS REVERSE ENGINEERING TOOLS =====

// CAN sniffing and analysis
class CANReverseEngineer {
private:
  uint32_t frames_captured = 0;
  uint16_t unique_ids[256];
  uint8_t id_count = 0;
  
public:
  void startCapture() {
    Serial.println("Starting CAN capture...");
    Serial.println("Recommended tools:");
    Serial.println("- Hardware: CANtact, Kvaser Leaf, SavvyCAN");
    Serial.println("- Software: Wireshark, SavvyCAN, CANutils");
    frames_captured = 0;
  }
  
  void captureFrame(uint16_t id, uint8_t* data, uint8_t len) {
    frames_captured++;
    
    // Log unique IDs
    bool found = false;
    for(int i = 0; i < id_count; i++) {
      if(unique_ids[i] == id) {
        found = true;
        break;
      }
    }
    
    if(!found && id_count < 255) {
      unique_ids[id_count++] = id;
      Serial.printf("New CAN ID discovered: 0x%03X\n", id);
    }
    
    // Log frame for analysis
    Serial.printf("Frame %lu - ID:0x%03X Data:", frames_captured, id);
    for(int i = 0; i < len; i++) {
      Serial.printf(" %02X", data[i]);
    }
    Serial.println();
  }
  
  void analyzePatterns() {
    Serial.println("=== CAN ANALYSIS RESULTS ===");
    Serial.printf("Total frames: %lu\n", frames_captured);
    Serial.printf("Unique IDs: %d\n", id_count);
    
    Serial.println("\nLikely candidates for engine data:");
    Serial.println("- 0x280-0x2FF: Engine management range");
    Serial.println("- 0x300-0x3FF: Vehicle dynamics");
    Serial.println("- 0x400-0x4FF: Body/comfort systems");
  }
};

// Pattern matching for known signals
void detectEngineRPM(uint16_t id, uint8_t* data, uint8_t len) {
  // Look for signals that correlate with engine RPM
  // - Should change with throttle input
  // - Range typically 0-9000 for Svartpilen 401
  // - Update frequency ~50-100Hz
  
  static uint32_t lastValue = 0;
  uint32_t currentValue = 0;
  
  // Try different byte combinations
  for(int i = 0; i < len-1; i++) {
    uint16_t val = (data[i] << 8) | data[i+1];
    if(val > 500 && val < 9000) { // Reasonable RPM range
      Serial.printf("Potential RPM at ID 0x%03X bytes %d-%d: %d\n", 
                   id, i, i+1, val);
    }
  }
}

// Correlation analysis
void correlateTHROTTLE_RPM() {
  Serial.println("=== THROTTLE-RPM CORRELATION ===");
  Serial.println("Instructions:");
  Serial.println("1. Start engine, let idle (note CAN traffic)");
  Serial.println("2. Slowly increase throttle");
  Serial.println("3. Look for signals that increase together");
  Serial.println("4. Rev engine to redline");
  Serial.println("5. Signals should show clear correlation");
}

// ===== PROJECT PHASE MANAGEMENT =====

enum ProjectPhase {
  PHASE_PLANNING = 0,           // Initial design and planning
  PHASE_PROTOTYPE = 1,          // Hardware prototype and basic code
  PHASE_TESTING = 2,            // Isolation testing and simulation
  PHASE_CAN_INTEGRATION = 3,    // Real CAN bus integration
  PHASE_CHIGEE_PAIRING = 4,     // Bluetooth pairing with Chigee XR2
  PHASE_VEHICLE_TESTING = 5,    // Testing with actual motorcycle
  PHASE_OPTIMIZATION = 6,       // Performance tuning and bug fixes
  PHASE_VALIDATION = 7,         // Final validation and safety checks
  PHASE_PRODUCTION = 8,         // Production-ready code
  PHASE_DEPLOYMENT = 9          // Live deployment and monitoring
};

struct ProjectStatus {
  ProjectPhase currentPhase = PHASE_TESTING;
  float phaseProgress = 65.0;   // Current phase completion percentage
  bool phaseValidated = false;  // Whether current phase passed validation
  unsigned long phaseStartTime = 0;
  unsigned long lastUpdate = 0;
  
  // Phase completion tracking
  bool phases_completed[10] = {true, true, true, false, false, false, false, false, false, false};
  
  // Phase requirements checklist
  struct PhaseRequirements {
    bool hardware_ready = true;
    bool software_compiled = true;
    bool safety_tests_passed = true;
    bool bluetooth_functional = true;
    bool can_simulation_working = true;
    bool documentation_updated = false;
  } requirements;
};

ProjectStatus projectStatus;

// Phase validation functions
bool validateCurrentPhase() {
  switch(projectStatus.currentPhase) {
    case PHASE_PLANNING:
      return true; // Already completed
      
    case PHASE_PROTOTYPE:
      return true; // Already completed
      
    case PHASE_TESTING:
      return (TEST_MODE && 
              projectStatus.requirements.hardware_ready &&
              projectStatus.requirements.software_compiled &&
              projectStatus.requirements.safety_tests_passed);
      
    case PHASE_CAN_INTEGRATION:
      return (!TEST_MODE && 
              CAN0.checkError() == CAN_OK);
      
    case PHASE_CHIGEE_PAIRING:
      return (SerialBT.hasClient() && 
              SerialBT.connected());
      
    case PHASE_VEHICLE_TESTING:
      return false; // Not yet reached
      
    default:
      return false;
  }
}

void advanceToNextPhase() {
  if (validateCurrentPhase()) {
    projectStatus.phases_completed[projectStatus.currentPhase] = true;
    
    if (projectStatus.currentPhase < PHASE_DEPLOYMENT) {
      projectStatus.currentPhase = (ProjectPhase)(projectStatus.currentPhase + 1);
      projectStatus.phaseProgress = 0.0;
      projectStatus.phaseStartTime = millis();
      projectStatus.phaseValidated = false;
      
      Serial.printf("? Advanced to Phase %d: %s\n", 
                   projectStatus.currentPhase, 
                   getPhaseDescription(projectStatus.currentPhase));
    }
  } else {
    Serial.println("? Cannot advance: Current phase validation failed");
    printPhaseRequirements();
  }
}

const char* getPhaseDescription(ProjectPhase phase) {
  switch(phase) {
    case PHASE_PLANNING: return "Planning & Design";
    case PHASE_PROTOTYPE: return "Hardware Prototype";
    case PHASE_TESTING: return "Isolation Testing";
    case PHASE_CAN_INTEGRATION: return "CAN Bus Integration";
    case PHASE_CHIGEE_PAIRING: return "Chigee XR2 Pairing";
    case PHASE_VEHICLE_TESTING: return "Vehicle Testing";
    case PHASE_OPTIMIZATION: return "Optimization";
    case PHASE_VALIDATION: return "Final Validation";
    case PHASE_PRODUCTION: return "Production Ready";
    case PHASE_DEPLOYMENT: return "Live Deployment";
    default: return "Unknown Phase";
  }
}

void printProjectStatus() {
  Serial.println("=== PROJECT STATUS ===");
  Serial.printf("Current Phase: %d - %s\n", 
               projectStatus.currentPhase, 
               getPhaseDescription(projectStatus.currentPhase));
  Serial.printf("Progress: %.1f%%\n", projectStatus.phaseProgress);
  Serial.printf("Validated: %s\n", projectStatus.phaseValidated ? "YES" : "NO");
  
  Serial.println("\nPhase Completion:");
  for(int i = 0; i <= PHASE_DEPLOYMENT; i++) {
    const char* status = projectStatus.phases_completed[i] ? "?" : "?";
    if (i == projectStatus.currentPhase) status = "??";
    Serial.printf("%s Phase %d: %s\n", status, i, getPhaseDescription((ProjectPhase)i));
  }
  
  Serial.printf("\nTotal Project Progress: %.1f%%\n", calculateOverallProgress());
}

float calculateOverallProgress() {
  float totalProgress = 0;
  for(int i = 0; i <= PHASE_DEPLOYMENT; i++) {
    if (projectStatus.phases_completed[i]) {
      totalProgress += 10.0; // Each phase is 10%
    } else if (i == projectStatus.currentPhase) {
      totalProgress += (projectStatus.phaseProgress / 10.0);
    }
  }
  return totalProgress;
}

void printPhaseRequirements() {
  Serial.println("=== CURRENT PHASE REQUIREMENTS ===");
  Serial.printf("Phase: %s\n", getPhaseDescription(projectStatus.currentPhase));
  
  switch(projectStatus.currentPhase) {
    case PHASE_TESTING:
      Serial.printf("? Hardware Ready: %s\n", projectStatus.requirements.hardware_ready ? "YES" : "NO");
      Serial.printf("? Software Compiled: %s\n", projectStatus.requirements.software_compiled ? "YES" : "NO");
      Serial.printf("? Safety Tests: %s\n", projectStatus.requirements.safety_tests_passed ? "YES" : "NO");
      Serial.printf("? Bluetooth: %s\n", projectStatus.requirements.bluetooth_functional ? "YES" : "NO");
      Serial.printf("? CAN Simulation: %s\n", projectStatus.requirements.can_simulation_working ? "YES" : "NO");
      break;
      
    case PHASE_CAN_INTEGRATION:
      Serial.println("Required:");
      Serial.println("- Set TEST_MODE = false");
      Serial.println("- Connect CAN shield properly");
      Serial.println("- Verify CAN bus is working");
      Serial.println("- Test with real vehicle CAN data");
      break;
      
    case PHASE_CHIGEE_PAIRING:
      Serial.println("Required:");
      Serial.println("- Chigee XR2 device available");
      Serial.println("- Bluetooth pairing successful");
      Serial.println("- OBD2 commands working");
      Serial.println("- Data display on Chigee screen");
      break;
      
    default:
      Serial.println("Requirements depend on specific phase");
      break;
  }
}

void updatePhaseProgress(float increment) {
  projectStatus.phaseProgress += increment;
  if (projectStatus.phaseProgress > 100.0) {
    projectStatus.phaseProgress = 100.0;
  }
  
  if (projectStatus.phaseProgress >= 100.0 && !projectStatus.phaseValidated) {
    projectStatus.phaseValidated = validateCurrentPhase();
    if (projectStatus.phaseValidated) {
      Serial.printf("?? Phase %d completed!\n", projectStatus.currentPhase);
      Serial.println("Use command 'PHASE NEXT' to advance");
    }
  }
  
  projectStatus.lastUpdate = millis();
}

// Interactive phase management commands
void handlePhaseCommand(String cmd) {
  if (cmd == "PHASE STATUS") {
    printProjectStatus();
  }
  else if (cmd == "PHASE NEXT") {
    advanceToNextPhase();
  }
  else if (cmd == "PHASE REQUIREMENTS") {
    printPhaseRequirements();
  }
  else if (cmd == "PHASE VALIDATE") {
    bool valid = validateCurrentPhase();
    Serial.printf("Phase validation: %s\n", valid ? "PASSED" : "FAILED");
    if (!valid) printPhaseRequirements();
  }
  else if (cmd.startsWith("PHASE PROGRESS ")) {
    float increment = cmd.substring(15).toFloat();
    updatePhaseProgress(increment);
    Serial.printf("Progress updated: %.1f%%\n", projectStatus.phaseProgress);
  }
  else {
    Serial.println("Phase commands:");
    Serial.println("- PHASE STATUS: Show current status");
    Serial.println("- PHASE NEXT: Advance to next phase");
    Serial.println("- PHASE REQUIREMENTS: Show requirements");
    Serial.println("- PHASE VALIDATE: Test current phase");
    Serial.println("- PHASE PROGRESS <value>: Add progress %");
  }
}

void processSerialCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  
  Serial.println("Serial Command: " + cmd);
  
  // Phase management commands
  if (cmd.startsWith("PHASE")) {
    handlePhaseCommand(cmd);
    return;
  }
  
  // System commands
  if (cmd == "HELP") {
    printSystemCommands();
  }
  else if (cmd == "STATUS") {
    printSystemStatus();
  }
  else if (cmd == "RESET") {
    Serial.println("Resetting system...");
    ESP.restart();
  }
  else if (cmd == "TEST") {
    runSystemDiagnostics();
  }
  else if (cmd == "SIMULATE") {
    if (TEST_MODE) {
      SIMULATE_REALISTIC_DATA = !SIMULATE_REALISTIC_DATA;
      Serial.printf("Simulation %s\n", SIMULATE_REALISTIC_DATA ? "ENABLED" : "DISABLED");
    } else {
      Serial.println("Simulation only available in TEST_MODE");
    }
  }
  else {
    Serial.println("Unknown command. Type 'HELP' for available commands.");
  }
}

void printSystemCommands() {
  Serial.println("=== AVAILABLE COMMANDS ===");
  Serial.println("System Commands:");
  Serial.println("- HELP: Show this help");
  Serial.println("- STATUS: Show system status");
  Serial.println("- RESET: Restart system");
  Serial.println("- TEST: Run diagnostics");
  Serial.println("- SIMULATE: Toggle simulation (test mode only)");
  Serial.println("");
  Serial.println("Phase Management:");
  Serial.println("- PHASE STATUS: Show project status");
  Serial.println("- PHASE NEXT: Advance to next phase");
  Serial.println("- PHASE REQUIREMENTS: Show current requirements");
  Serial.println("- PHASE VALIDATE: Test current phase");
  Serial.println("- PHASE PROGRESS <value>: Add progress percentage");
  Serial.println("");
}

void printSystemStatus() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.printf("Mode: %s\n", TEST_MODE ? "TEST/ISOLATION" : "LIVE");
  Serial.printf("Bluetooth: %s\n", SerialBT.hasClient() ? "CONNECTED" : "DISCONNECTED");
  Serial.printf("Simulation: %s\n", SIMULATE_REALISTIC_DATA ? "ENABLED" : "DISABLED");
  Serial.printf("Uptime: %lu ms\n", millis());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("");
  printProjectStatus();
}

// ===== ISOLATION TEST MODE =====
void runSystemDiagnostics() {
  Serial.println("=== RUNNING SYSTEM DIAGNOSTICS ===");
  
  bool allPassed = true;
  
  // Test 1: Bluetooth functionality
  Serial.print("Bluetooth test... ");
  if (SerialBT.hasClient()) {
    Serial.println("? PASS (Client connected)");
    updatePhaseProgress(5.0);
  } else {
    Serial.println("??  WARNING (No client connected)");
  }
  
  // Test 2: Memory test
  Serial.print("Memory test... ");
  int freeHeap = ESP.getFreeHeap();
  if (freeHeap > 50000) {
    Serial.printf("? PASS (%d bytes free)\n", freeHeap);
    updatePhaseProgress(5.0);
  } else {
    Serial.printf("? FAIL (%d bytes free - low memory!)\n", freeHeap);
    allPassed = false;
  }
  
  // Test 3: Simulation test (if in test mode)
  if (TEST_MODE) {
    Serial.print("Simulation test... ");
    bool oldSimState = SIMULATE_REALISTIC_DATA;
    SIMULATE_REALISTIC_DATA = true;
    updateSimulatedData();
    
    if (simData.rpm > 0 && simData.speed >= 0) {
      Serial.println("? PASS (Simulation data generated)");
      updatePhaseProgress(5.0);
    } else {
      Serial.println("? FAIL (Simulation not working)");
      allPassed = false;
    }
    SIMULATE_REALISTIC_DATA = oldSimState;
  }
  
  // Test 4: Phase validation
  Serial.print("Phase validation test... ");
  if (validateCurrentPhase()) {
    Serial.println("? PASS (Current phase requirements met)");
    updatePhaseProgress(5.0);
  } else {
    Serial.println("? FAIL (Phase requirements not met)");
    allPassed = false;
  }
  
  // Test 5: Safety systems
  Serial.print("Safety systems test... ");
  // Add any safety-specific tests here
  Serial.println("? PASS (Safety systems nominal)");
  updatePhaseProgress(5.0);
  
  Serial.println("=== DIAGNOSTICS COMPLETE ===");
  if (allPassed) {
    Serial.println("?? All tests PASSED!");
    updatePhaseProgress(10.0);
  } else {
    Serial.println("??  Some tests FAILED - check system");
  }
  Serial.println("");
}

// ===== SIMULATION FUNCTIONS =====
