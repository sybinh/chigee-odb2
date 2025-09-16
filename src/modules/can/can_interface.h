#pragma once

/**
 * @file can_interface.h
 * @brief Professional CAN bus interface with ESP32 CAN controller
 * @author Chigee OBD2 Project Team
 * @date 2025-01-15
 * 
 * This module provides professional CAN bus communication capabilities
 * for interfacing with vehicle ECUs and processing OBD2 data.
 */

#include <vector>
#include <queue>
#include <functional>
#include "../../config/project_config.h"
#include "../../config/hardware_config.h"

// ESP32 CAN includes
#include "driver/twai.h"

/**
 * @brief CAN bus speed configurations
 */
enum class CANSpeed {
    CAN_125KBPS,    // 125 kbit/s
    CAN_250KBPS,    // 250 kbit/s  
    CAN_500KBPS,    // 500 kbit/s (Standard OBD2)
    CAN_1MBPS       // 1 Mbit/s
};

/**
 * @brief CAN message types
 */
enum class CANMessageType {
    STANDARD,       // Standard 11-bit identifier
    EXTENDED,       // Extended 29-bit identifier
    ERROR_FRAME,    // Error frame
    REMOTE_FRAME    // Remote transmission request
};

/**
 * @brief CAN operation modes
 */
enum class CANMode {
    NORMAL,         // Normal operation
    LISTEN_ONLY,    // Listen-only (no ACK, no error frames)
    SELF_TEST,      // Self-test mode (loopback)
    NO_ACK          // No acknowledgment mode
};

/**
 * @brief CAN filter types
 */
enum class CANFilterType {
    ACCEPT_ALL,     // Accept all messages
    WHITELIST,      // Accept only whitelisted IDs
    BLACKLIST,      // Reject blacklisted IDs
    RANGE,          // Accept ID range
    CUSTOM          // Custom filter function
};

/**
 * @brief CAN message structure
 */
struct CANMessage {
    uint32_t id;                    // CAN identifier
    CANMessageType type;            // Message type
    uint8_t dlc;                    // Data length code (0-8)
    uint8_t data[8];                // Data bytes
    bool rtr;                       // Remote transmission request
    bool extd;                      // Extended frame format
    unsigned long timestamp;        // Reception timestamp
    uint16_t errorFlags;            // Error flags if any
    
    // Constructor
    CANMessage() : id(0), type(CANMessageType::STANDARD), dlc(0), 
                   rtr(false), extd(false), timestamp(0), errorFlags(0) {
        memset(data, 0, sizeof(data));
    }
};

/**
 * @brief CAN statistics structure
 */
struct CANStatistics {
    uint32_t messagesReceived;      // Total messages received
    uint32_t messagesSent;          // Total messages sent
    uint32_t errorFrames;           // Error frames received
    uint32_t busOffEvents;          // Bus-off events
    uint32_t arbitrationLost;       // Arbitration lost count
    uint32_t receiveOverflow;       // Receive buffer overflow
    uint32_t transmitTimeout;       // Transmit timeout count
    uint32_t filterRejects;         // Messages rejected by filter
    float busUtilization;           // Bus utilization percentage
    unsigned long lastMessageTime;  // Last message timestamp
    unsigned long uptimeSeconds;    // Interface uptime
    
    // Constructor
    CANStatistics() : messagesReceived(0), messagesSent(0), errorFrames(0),
                     busOffEvents(0), arbitrationLost(0), receiveOverflow(0),
                     transmitTimeout(0), filterRejects(0), busUtilization(0.0),
                     lastMessageTime(0), uptimeSeconds(0) {}
};

/**
 * @brief CAN message filter
 */
struct CANFilter {
    CANFilterType type;             // Filter type
    uint32_t id;                    // Filter ID
    uint32_t mask;                  // Filter mask
    uint32_t rangeStart;            // Range start (for range filter)
    uint32_t rangeEnd;              // Range end (for range filter)
    std::vector<uint32_t> whitelist; // Whitelist IDs
    std::vector<uint32_t> blacklist; // Blacklist IDs
    std::function<bool(const CANMessage&)> customFilter; // Custom filter function
    bool enabled;                   // Filter enabled
    
    // Constructor
    CANFilter() : type(CANFilterType::ACCEPT_ALL), id(0), mask(0),
                  rangeStart(0), rangeEnd(0), enabled(true) {}
};

/**
 * @brief CAN message callback function type
 */
typedef std::function<void(const CANMessage&)> CANMessageCallback;

/**
 * @brief CAN error callback function type
 */
typedef std::function<void(uint16_t errorCode, const String& description)> CANErrorCallback;

/**
 * @class CANInterface
 * @brief Professional CAN bus interface with advanced features
 */
class CANInterface {
private:
    // Hardware configuration
    CANSpeed currentSpeed;
    CANMode currentMode;
    bool interfaceEnabled;
    bool busOff;
    
    // Message handling
    std::queue<CANMessage> receiveQueue;
    std::queue<CANMessage> transmitQueue;
    const size_t maxQueueSize = 100;
    
    // Filtering
    CANFilter messageFilter;
    
    // Statistics
    CANStatistics statistics;
    unsigned long interfaceStartTime;
    
    // Callbacks
    CANMessageCallback messageCallback;
    CANErrorCallback errorCallback;
    
    // Internal methods
    bool configureCANController();
    void processReceivedMessage(const twai_message_t& message);
    void handleCANError(uint16_t errorCode);
    bool applyMessageFilter(const CANMessage& message);
    void updateStatistics();
    String getErrorDescription(uint16_t errorCode);
    
public:
    /**
     * @brief Constructor
     */
    CANInterface();
    
    /**
     * @brief Destructor
     */
    ~CANInterface();
    
    // ===== INITIALIZATION =====
    
    /**
     * @brief Initialize CAN interface
     * @param speed CAN bus speed
     * @param mode Operation mode
     * @return true if initialization successful
     */
    bool initialize(CANSpeed speed = CANSpeed::CAN_500KBPS, 
                   CANMode mode = CANMode::NORMAL);
    
    /**
     * @brief Start CAN interface
     * @return true if started successfully
     */
    bool start();
    
    /**
     * @brief Stop CAN interface
     */
    void stop();
    
    /**
     * @brief Reset CAN interface
     * @return true if reset successful
     */
    bool reset();
    
    /**
     * @brief Check if interface is initialized
     * @return true if initialized
     */
    bool isInitialized() const;
    
    // ===== CONFIGURATION =====
    
    /**
     * @brief Set CAN bus speed
     * @param speed New CAN speed
     * @return true if speed set successfully
     */
    bool setSpeed(CANSpeed speed);
    
    /**
     * @brief Get current CAN speed
     * @return Current CAN speed
     */
    CANSpeed getSpeed() const;
    
    /**
     * @brief Set operation mode
     * @param mode New operation mode
     * @return true if mode set successfully
     */
    bool setMode(CANMode mode);
    
    /**
     * @brief Get current operation mode
     * @return Current operation mode
     */
    CANMode getMode() const;
    
    /**
     * @brief Get CAN speed in bps
     * @param speed CAN speed enum
     * @return Speed in bits per second
     */
    static uint32_t getSpeedBPS(CANSpeed speed);
    
    // ===== MESSAGE TRANSMISSION =====
    
    /**
     * @brief Send CAN message
     * @param message Message to send
     * @param timeout Timeout in milliseconds
     * @return true if message sent successfully
     */
    bool sendMessage(const CANMessage& message, uint32_t timeout = 1000);
    
    /**
     * @brief Send CAN message with data
     * @param id CAN identifier
     * @param data Data bytes
     * @param length Data length (0-8)
     * @param extended Extended frame format
     * @param timeout Timeout in milliseconds
     * @return true if message sent successfully
     */
    bool sendMessage(uint32_t id, const uint8_t* data, uint8_t length, 
                    bool extended = false, uint32_t timeout = 1000);
    
    /**
     * @brief Send OBD2 request
     * @param pid Parameter ID
     * @param mode OBD2 mode (default 0x01)
     * @return true if request sent successfully
     */
    bool sendOBD2Request(uint16_t pid, uint8_t mode = 0x01);
    
    /**
     * @brief Queue message for transmission
     * @param message Message to queue
     * @return true if message queued successfully
     */
    bool queueMessage(const CANMessage& message);
    
    /**
     * @brief Process transmit queue
     * @return Number of messages sent
     */
    int processTransmitQueue();
    
    // ===== MESSAGE RECEPTION =====
    
    /**
     * @brief Receive CAN message
     * @param message Reference to store received message
     * @param timeout Timeout in milliseconds
     * @return true if message received
     */
    bool receiveMessage(CANMessage& message, uint32_t timeout = 0);
    
    /**
     * @brief Check if messages are available
     * @return Number of messages in receive queue
     */
    int availableMessages() const;
    
    /**
     * @brief Process received messages
     * @return Number of messages processed
     */
    int processReceiveQueue();
    
    /**
     * @brief Flush receive queue
     */
    void flushReceiveQueue();
    
    /**
     * @brief Set message received callback
     * @param callback Callback function
     */
    void setMessageCallback(CANMessageCallback callback);
    
    // ===== FILTERING =====
    
    /**
     * @brief Set message filter
     * @param filter Filter configuration
     */
    void setMessageFilter(const CANFilter& filter);
    
    /**
     * @brief Set accept all filter
     */
    void setAcceptAllFilter();
    
    /**
     * @brief Set whitelist filter
     * @param ids Vector of accepted IDs
     */
    void setWhitelistFilter(const std::vector<uint32_t>& ids);
    
    /**
     * @brief Set blacklist filter
     * @param ids Vector of rejected IDs
     */
    void setBlacklistFilter(const std::vector<uint32_t>& ids);
    
    /**
     * @brief Set range filter
     * @param startId Start of ID range
     * @param endId End of ID range
     */
    void setRangeFilter(uint32_t startId, uint32_t endId);
    
    /**
     * @brief Set custom filter function
     * @param filterFunc Custom filter function
     */
    void setCustomFilter(std::function<bool(const CANMessage&)> filterFunc);
    
    /**
     * @brief Enable/disable filter
     * @param enabled Filter enabled state
     */
    void setFilterEnabled(bool enabled);
    
    // ===== STATUS & DIAGNOSTICS =====
    
    /**
     * @brief Get CAN bus status
     * @return Status string
     */
    String getStatus() const;
    
    /**
     * @brief Check if bus is off
     * @return true if bus is in bus-off state
     */
    bool isBusOff() const;
    
    /**
     * @brief Get error state
     * @return Error state code
     */
    uint16_t getErrorState() const;
    
    /**
     * @brief Get statistics
     * @return CAN statistics structure
     */
    CANStatistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Print diagnostics
     */
    void printDiagnostics() const;
    
    /**
     * @brief Set error callback
     * @param callback Error callback function
     */
    void setErrorCallback(CANErrorCallback callback);
    
    // ===== OBD2 SPECIFIC FUNCTIONS =====
    
    /**
     * @brief Initialize for OBD2 communication
     * @return true if OBD2 initialization successful
     */
    bool initializeOBD2();
    
    /**
     * @brief Send OBD2 functional request
     * @param data Request data
     * @param length Data length
     * @return true if request sent successfully
     */
    bool sendOBD2Functional(const uint8_t* data, uint8_t length);
    
    /**
     * @brief Send OBD2 physical request to specific ECU
     * @param ecuId ECU identifier
     * @param data Request data
     * @param length Data length
     * @return true if request sent successfully
     */
    bool sendOBD2Physical(uint32_t ecuId, const uint8_t* data, uint8_t length);
    
    /**
     * @brief Wait for OBD2 response
     * @param response Reference to store response
     * @param timeout Timeout in milliseconds
     * @return true if response received
     */
    bool waitOBD2Response(CANMessage& response, uint32_t timeout = 5000);
    
    /**
     * @brief Check if message is OBD2 response
     * @param message Message to check
     * @return true if message is OBD2 response
     */
    static bool isOBD2Response(const CANMessage& message);
    
    /**
     * @brief Parse OBD2 response
     * @param message CAN message containing OBD2 data
     * @param pid Reference to store parsed PID
     * @param data Reference to store parsed data
     * @param length Reference to store data length
     * @return true if parsing successful
     */
    static bool parseOBD2Response(const CANMessage& message, uint16_t& pid, 
                                 uint8_t* data, uint8_t& length);
    
    // ===== UTILITY FUNCTIONS =====
    
    /**
     * @brief Convert CAN message to string
     * @param message CAN message
     * @return String representation
     */
    static String messageToString(const CANMessage& message);
    
    /**
     * @brief Parse CAN message from string
     * @param str String representation
     * @param message Reference to store parsed message
     * @return true if parsing successful
     */
    static bool stringToMessage(const String& str, CANMessage& message);
    
    /**
     * @brief Calculate CAN message checksum
     * @param message CAN message
     * @return Checksum value
     */
    static uint8_t calculateChecksum(const CANMessage& message);
    
    /**
     * @brief Validate CAN message
     * @param message Message to validate
     * @return true if message is valid
     */
    static bool validateMessage(const CANMessage& message);
    
    /**
     * @brief Get timing configuration for speed
     * @param speed CAN speed
     * @param timing Reference to store timing config
     * @return true if timing configuration available
     */
    static bool getTimingConfig(CANSpeed speed, twai_timing_config_t& timing);
    
    /**
     * @brief Convert TWAI message to CANMessage
     * @param twaiMsg TWAI message structure
     * @param canMsg Reference to store converted message
     */
    static void convertFromTWAI(const twai_message_t& twaiMsg, CANMessage& canMsg);
    
    /**
     * @brief Convert CANMessage to TWAI message
     * @param canMsg CAN message structure
     * @param twaiMsg Reference to store converted message
     */
    static void convertToTWAI(const CANMessage& canMsg, twai_message_t& twaiMsg);
};

// ===== OBD2 CAN DEFINITIONS =====
namespace OBD2CAN {
    // Standard OBD2 CAN IDs
    constexpr uint32_t FUNCTIONAL_REQUEST_ID    = 0x7DF;    // Functional diagnostic request
    constexpr uint32_t RESPONSE_ID_BASE         = 0x7E8;    // Response ID base (0x7E8-0x7EF)
    constexpr uint32_t PHYSICAL_REQUEST_BASE    = 0x7E0;    // Physical request base (0x7E0-0x7E7)
    
    // Extended OBD2 CAN IDs (29-bit)
    constexpr uint32_t EXT_FUNCTIONAL_REQUEST   = 0x18DB33F1; // Extended functional request
    constexpr uint32_t EXT_RESPONSE_BASE        = 0x18DAF100; // Extended response base
    constexpr uint32_t EXT_PHYSICAL_REQUEST_BASE = 0x18DA00F1; // Extended physical request base
    
    // OBD2 frame types
    constexpr uint8_t FRAME_TYPE_SINGLE         = 0x00;    // Single frame
    constexpr uint8_t FRAME_TYPE_FIRST          = 0x10;    // First frame (multi-frame)
    constexpr uint8_t FRAME_TYPE_CONSECUTIVE    = 0x20;    // Consecutive frame
    constexpr uint8_t FRAME_TYPE_FLOW_CONTROL   = 0x30;    // Flow control frame
    
    // Flow control flags
    constexpr uint8_t FC_FLAG_CONTINUE_TO_SEND  = 0x00;    // Continue to send
    constexpr uint8_t FC_FLAG_WAIT              = 0x01;    // Wait
    constexpr uint8_t FC_FLAG_OVERFLOW          = 0x02;    // Buffer overflow
    
    // Timing parameters (ISO 14229)
    constexpr uint32_t P2_CLIENT_MAX            = 50;      // P2*client max (ms)
    constexpr uint32_t P2_STAR_CLIENT_MAX       = 5000;    // P2*client max (ms)
    constexpr uint8_t  ST_MIN_DEFAULT           = 0;       // STmin default (ms)
    constexpr uint8_t  BLOCK_SIZE_DEFAULT       = 0;       // Block size default (unlimited)
}

#endif // CAN_INTERFACE_H