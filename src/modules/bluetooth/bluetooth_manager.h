#pragma once

/**
 * @file bluetooth_manager.h
 * @brief Professional Bluetooth communication manager for Chigee XR2 OBD2 Module
 * @author Chigee OBD2 Project Team
 * @date 2025-09-15
 * 
 * This module handles all Bluetooth Serial Profile (SPP) communications
 * with advanced security features for Chigee XR2 compatibility.
 */

#include <BluetoothSerial.h>
#include <esp_bt_device.h>
#include "../../config/project_config.h"
#include "../../config/hardware_config.h"

/**
 * @brief Connection states for Bluetooth management
 */
enum class BluetoothState {
    DISABLED,           // Bluetooth radio disabled
    INITIALIZING,       // Starting up
    ADVERTISING,        // Discoverable, waiting for connection
    CONNECTED,          // Client connected
    AUTHENTICATED,      // Passed security validation
    ERROR_STATE         // Error condition
};

/**
 * @brief Device security profiles for spoofing
 */
enum class DeviceProfile {
    GENUINE_ELM327,     // Genuine ELM327 v1.5
    OBDLINK_MX_PLUS,    // OBDLink MX+ professional
    BAFX_PRODUCTS,      // BAFX Products reader
    CUSTOM              // Custom profile
};

/**
 * @brief Bluetooth connection statistics
 */
struct BluetoothStats {
    uint32_t connection_count;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t commands_processed;
    uint32_t security_events;
    unsigned long uptime_ms;
    unsigned long last_activity_ms;
};

/**
 * @brief Security fingerprint for device spoofing
 */
struct SecurityFingerprint {
    String device_name;
    String manufacturer;
    uint8_t mac_address[6];
    String service_uuid;
    uint16_t class_of_device;
    bool authentication_required;
};

/**
 * @class BluetoothManager
 * @brief Professional Bluetooth communication handler with security features
 */
class BluetoothManager {
private:
    BluetoothSerial serialBT;
    BluetoothState currentState;
    DeviceProfile activeProfile;
    SecurityFingerprint fingerprint;
    BluetoothStats stats;
    
    // Connection management
    bool isClientConnected;
    String connectedClientMAC;
    unsigned long connectionStartTime;
    unsigned long lastHeartbeat;
    
    // Security features
    bool spoofingEnabled;
    bool macAddressSpoofed;
    uint8_t originalMAC[6];
    uint8_t spoofedMAC[6];
    
    // Command buffering
    String inputBuffer;
    static const size_t BUFFER_SIZE = BLUETOOTH_BUFFER_SIZE;
    
    // Internal methods
    void initializeProfiles();
    void applySpoofingProfile(DeviceProfile profile);
    bool setupBluetoothSecurity();
    void updateConnectionStats();
    void logSecurityEvent(const String& event, const String& details = "");
    
public:
    /**
     * @brief Constructor
     */
    BluetoothManager();
    
    /**
     * @brief Destructor
     */
    ~BluetoothManager();
    
    // ===== INITIALIZATION AND SETUP =====
    
    /**
     * @brief Initialize Bluetooth manager
     * @return true if initialization successful
     */
    bool begin();
    
    /**
     * @brief Shutdown Bluetooth manager
     */
    void end();
    
    /**
     * @brief Configure device profile for spoofing
     * @param profile Device profile to emulate
     * @return true if profile applied successfully
     */
    bool setDeviceProfile(DeviceProfile profile);
    
    /**
     * @brief Set custom device name
     * @param name Device name for Bluetooth advertising
     */
    void setDeviceName(const String& name);
    
    /**
     * @brief Enable/disable MAC address spoofing
     * @param enable Enable spoofing (WARNING: Changes MAC permanently!)
     * @return true if successful
     */
    bool enableMACAddressSpoofing(bool enable);
    
    // ===== CONNECTION MANAGEMENT =====
    
    /**
     * @brief Handle incoming connections and maintain existing ones
     * Call this regularly in main loop
     */
    void handleConnections();
    
    /**
     * @brief Check if client is currently connected
     * @return true if client connected
     */
    bool isConnected() const;
    
    /**
     * @brief Get connected client MAC address
     * @return MAC address string or empty if not connected
     */
    String getConnectedClientMAC() const;
    
    /**
     * @brief Get connection duration in milliseconds
     * @return Connection duration or 0 if not connected
     */
    unsigned long getConnectionDuration() const;
    
    /**
     * @brief Disconnect current client
     */
    void disconnect();
    
    // ===== COMMUNICATION =====
    
    /**
     * @brief Check if data is available to read
     * @return Number of bytes available
     */
    int available();
    
    /**
     * @brief Read a command from Bluetooth
     * @return Complete command string or empty if none available
     */
    String readCommand();
    
    /**
     * @brief Send response to connected client
     * @param response Response string to send
     * @return true if sent successfully
     */
    bool sendResponse(const String& response);
    
    /**
     * @brief Send formatted response
     * @param format Printf-style format string
     * @param ... Arguments for format string
     * @return true if sent successfully
     */
    bool sendResponsef(const char* format, ...);
    
    /**
     * @brief Send raw data
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     * @return Number of bytes actually sent
     */
    size_t sendRawData(const uint8_t* data, size_t length);
    
    // ===== SECURITY AND SPOOFING =====
    
    /**
     * @brief Get current device profile
     * @return Active device profile
     */
    DeviceProfile getCurrentProfile() const;
    
    /**
     * @brief Get security fingerprint
     * @return Current security fingerprint
     */
    SecurityFingerprint getFingerprint() const;
    
    /**
     * @brief Check if MAC spoofing is active
     * @return true if MAC address is spoofed
     */
    bool isMACAddressSpoofed() const;
    
    /**
     * @brief Get original MAC address (before spoofing)
     * @param mac Buffer to store MAC address (6 bytes)
     */
    void getOriginalMAC(uint8_t* mac) const;
    
    /**
     * @brief Get current MAC address (spoofed or original)
     * @param mac Buffer to store MAC address (6 bytes)
     */
    void getCurrentMAC(uint8_t* mac) const;
    
    // ===== STATISTICS AND MONITORING =====
    
    /**
     * @brief Get current state
     * @return Current Bluetooth state
     */
    BluetoothState getState() const;
    
    /**
     * @brief Get connection statistics
     * @return Statistics structure
     */
    BluetoothStats getStatistics() const;
    
    /**
     * @brief Reset statistics counters
     */
    void resetStatistics();
    
    /**
     * @brief Get state as human-readable string
     * @return State description
     */
    String getStateString() const;
    
    /**
     * @brief Print diagnostic information
     */
    void printDiagnostics() const;
    
    // ===== CALLBACKS =====
    
    /**
     * @brief Callback function type for connection events
     */
    typedef void (*ConnectionCallback)(bool connected, const String& clientMAC);
    
    /**
     * @brief Callback function type for security events
     */
    typedef void (*SecurityCallback)(const String& event, const String& details);
    
    /**
     * @brief Set connection event callback
     * @param callback Function to call on connection changes
     */
    void setConnectionCallback(ConnectionCallback callback);
    
    /**
     * @brief Set security event callback
     * @param callback Function to call on security events
     */
    void setSecurityCallback(SecurityCallback callback);
    
private:
    // Callback function pointers
    ConnectionCallback connectionCallback;
    SecurityCallback securityCallback;
    
    // Internal state management
    void updateState(BluetoothState newState);
    void processIncomingData();
    bool validateClient(const String& clientMAC);
    void handleSecurityChallenge(const String& challenge);
};

// ===== UTILITY FUNCTIONS =====

/**
 * @brief Convert MAC address array to string
 * @param mac MAC address array (6 bytes)
 * @return MAC address string (XX:XX:XX:XX:XX:XX)
 */
String macArrayToString(const uint8_t* mac);

/**
 * @brief Convert MAC address string to array
 * @param macString MAC address string
 * @param mac Output array (6 bytes)
 * @return true if conversion successful
 */
bool macStringToArray(const String& macString, uint8_t* mac);

/**
 * @brief Get device profile name as string
 * @param profile Device profile enum
 * @return Profile name string
 */
String getDeviceProfileName(DeviceProfile profile);

/**
 * @brief Parse device profile from string
 * @param profileName Profile name string
 * @return Device profile enum
 */
DeviceProfile parseDeviceProfile(const String& profileName);

#endif // BLUETOOTH_MANAGER_H