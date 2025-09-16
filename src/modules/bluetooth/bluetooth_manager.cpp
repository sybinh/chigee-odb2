/**
 * @file bluetooth_manager.cpp
 * @brief Implementation of professional Bluetooth communication manager
 * @author Chigee OBD2 Project Team
 * @date 2025-09-15
 */

#include "bluetooth_manager.h"
#include <esp_bt_main.h>
#include <esp_bt_device.h>

// ===== DEVICE PROFILE CONFIGURATIONS =====

// Genuine ELM327 v1.5 profile
static const SecurityFingerprint PROFILE_GENUINE_ELM327 = {
    .device_name = "OBDII",
    .manufacturer = "ELM Electronics",
    .mac_address = {0x00, 0x1B, 0xDC, 0x12, 0x34, 0x56},
    .service_uuid = "00001101-0000-1000-8000-00805F9B34FB",
    .class_of_device = 0x1F00,
    .authentication_required = false
};

// OBDLink MX+ profile
static const SecurityFingerprint PROFILE_OBDLINK_MX = {
    .device_name = "OBDLink MX+",
    .manufacturer = "ScanTool.net",
    .mac_address = {0x00, 0x04, 0x3E, 0xAB, 0xCD, 0xEF},
    .service_uuid = "00001101-0000-1000-8000-00805F9B34FB",
    .class_of_device = 0x1F00,
    .authentication_required = false
};

// BAFX Products profile
static const SecurityFingerprint PROFILE_BAFX = {
    .device_name = "OBDII",
    .manufacturer = "BAFX Products",
    .mac_address = {0x00, 0x0D, 0x18, 0x11, 0x22, 0x33},
    .service_uuid = "00001101-0000-1000-8000-00805F9B34FB",
    .class_of_device = 0x1F00,
    .authentication_required = false
};

// ===== CONSTRUCTOR AND DESTRUCTOR =====

BluetoothManager::BluetoothManager() 
    : currentState(BluetoothState::DISABLED)
    , activeProfile(DeviceProfile::GENUINE_ELM327)
    , isClientConnected(false)
    , connectionStartTime(0)
    , lastHeartbeat(0)
    , spoofingEnabled(false)
    , macAddressSpoofed(false)
    , connectionCallback(nullptr)
    , securityCallback(nullptr)
{
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    
    // Store original MAC address
    esp_read_mac(originalMAC, ESP_MAC_BT);
    memcpy(spoofedMAC, originalMAC, 6);
    
    // Initialize fingerprint with default profile
    fingerprint = PROFILE_GENUINE_ELM327;
    
    DEBUG_PRINTLN("BluetoothManager: Constructor completed");
}

BluetoothManager::~BluetoothManager() {
    end();
    DEBUG_PRINTLN("BluetoothManager: Destructor completed");
}

// ===== INITIALIZATION AND SETUP =====

bool BluetoothManager::begin() {
    DEBUG_PRINTLN("BluetoothManager: Initializing...");
    
    updateState(BluetoothState::INITIALIZING);
    
    // Initialize profiles
    initializeProfiles();
    
    // Setup Bluetooth security if spoofing enabled
    if (spoofingEnabled) {
        if (!setupBluetoothSecurity()) {
            DEBUG_PRINTLN("BluetoothManager: Security setup failed");
            updateState(BluetoothState::ERROR_STATE);
            return false;
        }
    }
    
    // Initialize Bluetooth Serial
    if (!serialBT.begin(fingerprint.device_name)) {
        DEBUG_PRINTLN("BluetoothManager: Bluetooth initialization failed");
        updateState(BluetoothState::ERROR_STATE);
        return false;
    }
    
    // Set Class of Device if specified
    if (fingerprint.class_of_device != 0) {
        esp_bt_cod_t cod;
        cod.major = (fingerprint.class_of_device >> 8) & 0x1F;
        cod.minor = (fingerprint.class_of_device >> 2) & 0x3F;
        cod.service = (fingerprint.class_of_device >> 13) & 0x7FF;
        esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD);
    }
    
    updateState(BluetoothState::ADVERTISING);
    
    // Reset statistics
    stats.uptime_ms = millis();
    
    DEBUG_PRINTF("BluetoothManager: Initialized successfully as '%s'\n", 
                 fingerprint.device_name.c_str());
    
    logSecurityEvent("BLUETOOTH_INITIALIZED", 
                    "Profile: " + getDeviceProfileName(activeProfile));
    
    return true;
}

void BluetoothManager::end() {
    if (currentState != BluetoothState::DISABLED) {
        DEBUG_PRINTLN("BluetoothManager: Shutting down...");
        
        disconnect();
        serialBT.end();
        
        updateState(BluetoothState::DISABLED);
        
        DEBUG_PRINTLN("BluetoothManager: Shutdown complete");
    }
}

bool BluetoothManager::setDeviceProfile(DeviceProfile profile) {
    DEBUG_PRINTF("BluetoothManager: Setting device profile to %s\n", 
                 getDeviceProfileName(profile).c_str());
    
    activeProfile = profile;
    applySpoofingProfile(profile);
    
    logSecurityEvent("PROFILE_CHANGED", getDeviceProfileName(profile));
    
    return true;
}

void BluetoothManager::setDeviceName(const String& name) {
    fingerprint.device_name = name;
    
    // Update Bluetooth device name if already initialized
    if (currentState != BluetoothState::DISABLED) {
        // Note: Name change requires restart of Bluetooth
        DEBUG_PRINTLN("BluetoothManager: Device name changed - restart required");
    }
    
    DEBUG_PRINTF("BluetoothManager: Device name set to '%s'\n", name.c_str());
}

bool BluetoothManager::enableMACAddressSpoofing(bool enable) {
    if (enable && !macAddressSpoofed) {
        DEBUG_PRINTLN("BluetoothManager: WARNING - Enabling MAC address spoofing!");
        DEBUG_PRINTLN("This will permanently change your ESP32 MAC address!");
        
        // Set spoofed MAC address
        if (esp_base_mac_addr_set(spoofedMAC) == ESP_OK) {
            macAddressSpoofed = true;
            spoofingEnabled = true;
            
            DEBUG_PRINT("BluetoothManager: MAC spoofed to ");
            DEBUG_PRINTLN(macArrayToString(spoofedMAC));
            
            logSecurityEvent("MAC_SPOOFING_ENABLED", 
                           "Spoofed to " + macArrayToString(spoofedMAC));
            
            return true;
        } else {
            DEBUG_PRINTLN("BluetoothManager: MAC spoofing failed!");
            return false;
        }
    } else if (!enable && macAddressSpoofed) {
        // Restore original MAC
        if (esp_base_mac_addr_set(originalMAC) == ESP_OK) {
            macAddressSpoofed = false;
            
            DEBUG_PRINT("BluetoothManager: MAC restored to ");
            DEBUG_PRINTLN(macArrayToString(originalMAC));
            
            logSecurityEvent("MAC_SPOOFING_DISABLED", 
                           "Restored to " + macArrayToString(originalMAC));
            
            return true;
        } else {
            DEBUG_PRINTLN("BluetoothManager: MAC restoration failed!");
            return false;
        }
    }
    
    return true; // No change needed
}

// ===== CONNECTION MANAGEMENT =====

void BluetoothManager::handleConnections() {
    bool wasConnected = isClientConnected;
    bool nowConnected = serialBT.hasClient();
    
    // Update connection status
    if (nowConnected != wasConnected) {
        if (nowConnected) {
            // New connection established
            isClientConnected = true;
            connectionStartTime = millis();
            connectedClientMAC = "Unknown"; // BT Classic doesn't expose client MAC easily
            
            updateState(BluetoothState::CONNECTED);
            stats.connection_count++;
            
            DEBUG_PRINTLN("BluetoothManager: Client connected");
            
            if (connectionCallback) {
                connectionCallback(true, connectedClientMAC);
            }
            
            logSecurityEvent("CLIENT_CONNECTED", connectedClientMAC);
            
        } else {
            // Connection lost
            isClientConnected = false;
            connectedClientMAC = "";
            
            updateState(BluetoothState::ADVERTISING);
            
            DEBUG_PRINTLN("BluetoothManager: Client disconnected");
            
            if (connectionCallback) {
                connectionCallback(false, "");
            }
            
            logSecurityEvent("CLIENT_DISCONNECTED", "Connection duration: " + 
                           String(getConnectionDuration()) + "ms");
        }
    }
    
    // Process incoming data if connected
    if (isClientConnected) {
        processIncomingData();
        
        // Update heartbeat
        if (serialBT.available() || nowConnected != wasConnected) {
            lastHeartbeat = millis();
        }
        
        // Check for connection timeout
        if (millis() - lastHeartbeat > BLUETOOTH_TIMEOUT_MS) {
            DEBUG_PRINTLN("BluetoothManager: Connection timeout");
            disconnect();
        }
    }
    
    // Update statistics
    updateConnectionStats();
}

bool BluetoothManager::isConnected() const {
    return isClientConnected && serialBT.hasClient();
}

String BluetoothManager::getConnectedClientMAC() const {
    return connectedClientMAC;
}

unsigned long BluetoothManager::getConnectionDuration() const {
    if (isClientConnected) {
        return millis() - connectionStartTime;
    }
    return 0;
}

void BluetoothManager::disconnect() {
    if (isClientConnected) {
        DEBUG_PRINTLN("BluetoothManager: Forcing disconnect");
        
        serialBT.disconnect();
        isClientConnected = false;
        connectedClientMAC = "";
        
        updateState(BluetoothState::ADVERTISING);
        
        logSecurityEvent("FORCED_DISCONNECT", "Manual disconnect");
    }
}

// ===== COMMUNICATION =====

int BluetoothManager::available() {
    if (isClientConnected) {
        return serialBT.available();
    }
    return 0;
}

String BluetoothManager::readCommand() {
    if (!isClientConnected || !serialBT.available()) {
        return "";
    }
    
    // Read data into buffer
    while (serialBT.available()) {
        char c = serialBT.read();
        stats.bytes_received++;
        
        if (c == '\r' || c == '\n') {
            if (inputBuffer.length() > 0) {
                String command = inputBuffer;
                inputBuffer = "";
                stats.commands_processed++;
                
                DEBUG_PRINTF("BluetoothManager: Received command: '%s'\n", command.c_str());
                
                return command;
            }
        } else if (c >= 32 && c <= 126) { // Printable ASCII
            if (inputBuffer.length() < BUFFER_SIZE - 1) {
                inputBuffer += c;
            } else {
                DEBUG_PRINTLN("BluetoothManager: Command buffer overflow!");
                inputBuffer = "";
            }
        }
    }
    
    return ""; // No complete command yet
}

bool BluetoothManager::sendResponse(const String& response) {
    if (!isClientConnected) {
        return false;
    }
    
    size_t sent = serialBT.print(response);
    stats.bytes_sent += sent;
    
    DEBUG_PRINTF("BluetoothManager: Sent response: '%s'\n", response.c_str());
    
    return sent == response.length();
}

bool BluetoothManager::sendResponsef(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    return sendResponse(String(buffer));
}

size_t BluetoothManager::sendRawData(const uint8_t* data, size_t length) {
    if (!isClientConnected) {
        return 0;
    }
    
    size_t sent = serialBT.write(data, length);
    stats.bytes_sent += sent;
    
    return sent;
}

// ===== SECURITY AND SPOOFING =====

DeviceProfile BluetoothManager::getCurrentProfile() const {
    return activeProfile;
}

SecurityFingerprint BluetoothManager::getFingerprint() const {
    return fingerprint;
}

bool BluetoothManager::isMACAddressSpoofed() const {
    return macAddressSpoofed;
}

void BluetoothManager::getOriginalMAC(uint8_t* mac) const {
    memcpy(mac, originalMAC, 6);
}

void BluetoothManager::getCurrentMAC(uint8_t* mac) const {
    if (macAddressSpoofed) {
        memcpy(mac, spoofedMAC, 6);
    } else {
        esp_read_mac(mac, ESP_MAC_BT);
    }
}

// ===== STATISTICS AND MONITORING =====

BluetoothState BluetoothManager::getState() const {
    return currentState;
}

BluetoothStats BluetoothManager::getStatistics() const {
    return stats;
}

void BluetoothManager::resetStatistics() {
    memset(&stats, 0, sizeof(stats));
    stats.uptime_ms = millis();
    
    DEBUG_PRINTLN("BluetoothManager: Statistics reset");
}

String BluetoothManager::getStateString() const {
    switch (currentState) {
        case BluetoothState::DISABLED: return "DISABLED";
        case BluetoothState::INITIALIZING: return "INITIALIZING";
        case BluetoothState::ADVERTISING: return "ADVERTISING";
        case BluetoothState::CONNECTED: return "CONNECTED";
        case BluetoothState::AUTHENTICATED: return "AUTHENTICATED";
        case BluetoothState::ERROR_STATE: return "ERROR";
        default: return "UNKNOWN";
    }
}

void BluetoothManager::printDiagnostics() const {
    DEBUG_PRINTLN("=== BluetoothManager Diagnostics ===");
    DEBUG_PRINTF("State: %s\n", getStateString().c_str());
    DEBUG_PRINTF("Profile: %s\n", getDeviceProfileName(activeProfile).c_str());
    DEBUG_PRINTF("Device Name: %s\n", fingerprint.device_name.c_str());
    DEBUG_PRINTF("Connected: %s\n", isClientConnected ? "YES" : "NO");
    
    if (isClientConnected) {
        DEBUG_PRINTF("Client MAC: %s\n", connectedClientMAC.c_str());
        DEBUG_PRINTF("Connection Duration: %lu ms\n", getConnectionDuration());
    }
    
    DEBUG_PRINTF("MAC Spoofed: %s\n", macAddressSpoofed ? "YES" : "NO");
    
    uint8_t currentMAC[6];
    getCurrentMAC(currentMAC);
    DEBUG_PRINTF("Current MAC: %s\n", macArrayToString(currentMAC).c_str());
    
    DEBUG_PRINTF("Connections: %u\n", stats.connection_count);
    DEBUG_PRINTF("Bytes Sent: %u\n", stats.bytes_sent);
    DEBUG_PRINTF("Bytes Received: %u\n", stats.bytes_received);
    DEBUG_PRINTF("Commands Processed: %u\n", stats.commands_processed);
    DEBUG_PRINTF("Security Events: %u\n", stats.security_events);
    DEBUG_PRINTF("Uptime: %lu ms\n", millis() - stats.uptime_ms);
    DEBUG_PRINTLN("=====================================");
}

// ===== CALLBACKS =====

void BluetoothManager::setConnectionCallback(ConnectionCallback callback) {
    connectionCallback = callback;
}

void BluetoothManager::setSecurityCallback(SecurityCallback callback) {
    securityCallback = callback;
}

// ===== PRIVATE METHODS =====

void BluetoothManager::initializeProfiles() {
    // Set default profile based on configuration
    applySpoofingProfile(activeProfile);
}

void BluetoothManager::applySpoofingProfile(DeviceProfile profile) {
    switch (profile) {
        case DeviceProfile::GENUINE_ELM327:
            fingerprint = PROFILE_GENUINE_ELM327;
            memcpy(spoofedMAC, PROFILE_GENUINE_ELM327.mac_address, 6);
            break;
            
        case DeviceProfile::OBDLINK_MX_PLUS:
            fingerprint = PROFILE_OBDLINK_MX;
            memcpy(spoofedMAC, PROFILE_OBDLINK_MX.mac_address, 6);
            break;
            
        case DeviceProfile::BAFX_PRODUCTS:
            fingerprint = PROFILE_BAFX;
            memcpy(spoofedMAC, PROFILE_BAFX.mac_address, 6);
            break;
            
        case DeviceProfile::CUSTOM:
            // Keep current fingerprint
            break;
    }
}

bool BluetoothManager::setupBluetoothSecurity() {
    DEBUG_PRINTLN("BluetoothManager: Setting up security features");
    
    // Initialize Bluetooth if not already done
    if (!btStarted()) {
        if (!btStart()) {
            DEBUG_PRINTLN("BluetoothManager: Bluetooth start failed");
            return false;
        }
    }
    
    return true;
}

void BluetoothManager::updateConnectionStats() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    if (now - lastUpdate >= 1000) { // Update every second
        stats.last_activity_ms = lastHeartbeat;
        lastUpdate = now;
    }
}

void BluetoothManager::logSecurityEvent(const String& event, const String& details) {
    stats.security_events++;
    
    DEBUG_PRINTF("BluetoothManager: SECURITY EVENT - %s", event.c_str());
    if (details.length() > 0) {
        DEBUG_PRINTF(" (%s)", details.c_str());
    }
    DEBUG_PRINTLN();
    
    if (securityCallback) {
        securityCallback(event, details);
    }
}

void BluetoothManager::updateState(BluetoothState newState) {
    if (currentState != newState) {
        BluetoothState oldState = currentState;
        currentState = newState;
        
        DEBUG_PRINTF("BluetoothManager: State changed from %s to %s\n",
                     getStateString().c_str(), getStateString().c_str());
    }
}

void BluetoothManager::processIncomingData() {
    // This method is called regularly when connected
    // Additional processing can be added here if needed
}

bool BluetoothManager::validateClient(const String& clientMAC) {
    // Additional client validation can be implemented here
    // For now, accept all clients
    return true;
}

void BluetoothManager::handleSecurityChallenge(const String& challenge) {
    // Handle potential security challenges from Chigee XR2
    DEBUG_PRINTF("BluetoothManager: Security challenge received: %s\n", challenge.c_str());
    
    logSecurityEvent("SECURITY_CHALLENGE", challenge);
    
    // Challenge handling would be implemented based on reverse engineering results
}

// ===== UTILITY FUNCTIONS =====

String macArrayToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

bool macStringToArray(const String& macString, uint8_t* mac) {
    if (macString.length() != 17) return false;
    
    for (int i = 0; i < 6; i++) {
        String byteString = macString.substring(i * 3, i * 3 + 2);
        mac[i] = strtol(byteString.c_str(), NULL, 16);
    }
    
    return true;
}

String getDeviceProfileName(DeviceProfile profile) {
    switch (profile) {
        case DeviceProfile::GENUINE_ELM327: return "Genuine ELM327 v1.5";
        case DeviceProfile::OBDLINK_MX_PLUS: return "OBDLink MX+";
        case DeviceProfile::BAFX_PRODUCTS: return "BAFX Products";
        case DeviceProfile::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

DeviceProfile parseDeviceProfile(const String& profileName) {
    if (profileName.indexOf("ELM327") >= 0) return DeviceProfile::GENUINE_ELM327;
    if (profileName.indexOf("OBDLink") >= 0) return DeviceProfile::OBDLINK_MX_PLUS;
    if (profileName.indexOf("BAFX") >= 0) return DeviceProfile::BAFX_PRODUCTS;
    return DeviceProfile::CUSTOM;
}