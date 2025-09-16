/**
 * @file can_interface.cpp
 * @brief Professional CAN bus interface implementation
 * @author Chigee OBD2 Project Team
 * @date 2025-01-15
 */

#include "can_interface.h"
#include <Arduino.h>

// ===== CONSTRUCTOR & DESTRUCTOR =====

CANInterface::CANInterface() :
    currentSpeed(CANSpeed::CAN_500KBPS),
    currentMode(CANMode::NORMAL),
    interfaceEnabled(false),
    busOff(false),
    interfaceStartTime(0),
    messageCallback(nullptr),
    errorCallback(nullptr)
{
    // Initialize statistics
    statistics = CANStatistics();
}

CANInterface::~CANInterface() {
    stop();
}

// ===== INITIALIZATION =====

bool CANInterface::initialize(CANSpeed speed, CANMode mode) {
    Serial.println(F("[CAN] Initializing CAN interface..."));
    
    currentSpeed = speed;
    currentMode = mode;
    
    // Get timing configuration for the specified speed
    twai_timing_config_t timing_config;
    if (!getTimingConfig(speed, timing_config)) {
        Serial.println(F("[CAN] ERROR: Invalid CAN speed configuration"));
        return false;
    }
    
    // Configure CAN controller
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
        static_cast<gpio_num_t>(CAN_TX_PIN), 
        static_cast<gpio_num_t>(CAN_RX_PIN), 
        TWAI_MODE_NORMAL
    );
    
    // Set mode based on configuration
    switch (mode) {
        case CANMode::LISTEN_ONLY:
            general_config.mode = TWAI_MODE_LISTEN_ONLY;
            break;
        case CANMode::SELF_TEST:
            general_config.mode = TWAI_MODE_SELF_TEST;
            break;
        case CANMode::NO_ACK:
            general_config.mode = TWAI_MODE_NO_ACK;
            break;
        default:
            general_config.mode = TWAI_MODE_NORMAL;
            break;
    }
    
    // Configure filter to accept all messages initially
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install TWAI driver
    esp_err_t result = twai_driver_install(&general_config, &timing_config, &filter_config);
    if (result != ESP_OK) {
        Serial.printf("[CAN] ERROR: Failed to install TWAI driver: %s\n", esp_err_to_name(result));
        return false;
    }
    
    // Initialize queues
    while (!receiveQueue.empty()) receiveQueue.pop();
    while (!transmitQueue.empty()) transmitQueue.pop();
    
    // Reset statistics
    resetStatistics();
    
    interfaceEnabled = true;
    interfaceStartTime = millis();
    
    Serial.printf("[CAN] Interface initialized at %d bps, mode: ", getSpeedBPS(speed));
    switch (mode) {
        case CANMode::NORMAL:
            Serial.println("NORMAL");
            break;
        case CANMode::LISTEN_ONLY:
            Serial.println("LISTEN_ONLY");
            break;
        case CANMode::SELF_TEST:
            Serial.println("SELF_TEST");
            break;
        case CANMode::NO_ACK:
            Serial.println("NO_ACK");
            break;
    }
    
    return true;
}

bool CANInterface::start() {
    if (!interfaceEnabled) {
        Serial.println(F("[CAN] ERROR: Interface not initialized"));
        return false;
    }
    
    // Start TWAI driver
    esp_err_t result = twai_start();
    if (result != ESP_OK) {
        Serial.printf("[CAN] ERROR: Failed to start TWAI driver: %s\n", esp_err_to_name(result));
        return false;
    }
    
    busOff = false;
    Serial.println(F("[CAN] Interface started successfully"));
    return true;
}

void CANInterface::stop() {
    if (!interfaceEnabled) {
        return;
    }
    
    // Stop TWAI driver
    twai_stop();
    
    // Uninstall driver
    twai_driver_uninstall();
    
    interfaceEnabled = false;
    busOff = false;
    
    Serial.println(F("[CAN] Interface stopped"));
}

bool CANInterface::reset() {
    Serial.println(F("[CAN] Resetting interface..."));
    
    bool wasEnabled = interfaceEnabled;
    CANSpeed oldSpeed = currentSpeed;
    CANMode oldMode = currentMode;
    
    stop();
    
    if (wasEnabled) {
        return initialize(oldSpeed, oldMode) && start();
    }
    
    return true;
}

bool CANInterface::isInitialized() const {
    return interfaceEnabled;
}

// ===== CONFIGURATION =====

bool CANInterface::setSpeed(CANSpeed speed) {
    if (interfaceEnabled) {
        Serial.println(F("[CAN] Cannot change speed while interface is active"));
        return false;
    }
    
    currentSpeed = speed;
    Serial.printf("[CAN] Speed set to %d bps\n", getSpeedBPS(speed));
    return true;
}

CANSpeed CANInterface::getSpeed() const {
    return currentSpeed;
}

bool CANInterface::setMode(CANMode mode) {
    if (interfaceEnabled) {
        Serial.println(F("[CAN] Cannot change mode while interface is active"));
        return false;
    }
    
    currentMode = mode;
    Serial.print(F("[CAN] Mode set to: "));
    switch (mode) {
        case CANMode::NORMAL:
            Serial.println("NORMAL");
            break;
        case CANMode::LISTEN_ONLY:
            Serial.println("LISTEN_ONLY");
            break;
        case CANMode::SELF_TEST:
            Serial.println("SELF_TEST");
            break;
        case CANMode::NO_ACK:
            Serial.println("NO_ACK");
            break;
    }
    return true;
}

CANMode CANInterface::getMode() const {
    return currentMode;
}

uint32_t CANInterface::getSpeedBPS(CANSpeed speed) {
    switch (speed) {
        case CANSpeed::CAN_125KBPS:
            return 125000;
        case CANSpeed::CAN_250KBPS:
            return 250000;
        case CANSpeed::CAN_500KBPS:
            return 500000;
        case CANSpeed::CAN_1MBPS:
            return 1000000;
        default:
            return 500000;
    }
}

// ===== MESSAGE TRANSMISSION =====

bool CANInterface::sendMessage(const CANMessage& message, uint32_t timeout) {
    if (!interfaceEnabled || busOff) {
        return false;
    }
    
    // Convert to TWAI message
    twai_message_t twai_msg;
    convertToTWAI(message, twai_msg);
    
    // Send message
    esp_err_t result = twai_transmit(&twai_msg, pdMS_TO_TICKS(timeout));
    
    if (result == ESP_OK) {
        statistics.messagesSent++;
        updateStatistics();
        return true;
    } else {
        if (result == ESP_ERR_TIMEOUT) {
            statistics.transmitTimeout++;
        }
        handleCANError(result);
        return false;
    }
}

bool CANInterface::sendMessage(uint32_t id, const uint8_t* data, uint8_t length, 
                              bool extended, uint32_t timeout) {
    if (length > 8) {
        return false;
    }
    
    CANMessage message;
    message.id = id;
    message.dlc = length;
    message.extd = extended;
    message.type = extended ? CANMessageType::EXTENDED : CANMessageType::STANDARD;
    memcpy(message.data, data, length);
    
    return sendMessage(message, timeout);
}

bool CANInterface::sendOBD2Request(uint16_t pid, uint8_t mode) {
    uint8_t data[8] = {0x02, mode, static_cast<uint8_t>(pid & 0xFF), 0x55, 0x55, 0x55, 0x55, 0x55};
    
    return sendMessage(OBD2CAN::FUNCTIONAL_REQUEST_ID, data, 8, false, 1000);
}

bool CANInterface::queueMessage(const CANMessage& message) {
    if (transmitQueue.size() >= maxQueueSize) {
        return false; // Queue full
    }
    
    transmitQueue.push(message);
    return true;
}

int CANInterface::processTransmitQueue() {
    int messagesSent = 0;
    
    while (!transmitQueue.empty() && messagesSent < 10) { // Limit to prevent blocking
        CANMessage message = transmitQueue.front();
        
        if (sendMessage(message, 10)) { // Short timeout for queued messages
            transmitQueue.pop();
            messagesSent++;
        } else {
            break; // Stop if transmission fails
        }
    }
    
    return messagesSent;
}

// ===== MESSAGE RECEPTION =====

bool CANInterface::receiveMessage(CANMessage& message, uint32_t timeout) {
    if (!interfaceEnabled) {
        return false;
    }
    
    // Check local queue first
    if (!receiveQueue.empty()) {
        message = receiveQueue.front();
        receiveQueue.pop();
        return true;
    }
    
    // Try to receive from TWAI driver
    twai_message_t twai_msg;
    esp_err_t result = twai_receive(&twai_msg, pdMS_TO_TICKS(timeout));
    
    if (result == ESP_OK) {
        convertFromTWAI(twai_msg, message);
        message.timestamp = millis();
        
        // Apply filter
        if (applyMessageFilter(message)) {
            statistics.messagesReceived++;
            statistics.lastMessageTime = message.timestamp;
            
            // Call callback if set
            if (messageCallback) {
                messageCallback(message);
            }
            
            updateStatistics();
            return true;
        } else {
            statistics.filterRejects++;
            return false;
        }
    }
    
    return false;
}

int CANInterface::availableMessages() const {
    return receiveQueue.size();
}

int CANInterface::processReceiveQueue() {
    int messagesProcessed = 0;
    CANMessage message;
    
    // Process up to 20 messages to prevent blocking
    while (messagesProcessed < 20 && receiveMessage(message, 0)) {
        if (receiveQueue.size() < maxQueueSize) {
            receiveQueue.push(message);
            messagesProcessed++;
        } else {
            statistics.receiveOverflow++;
            break;
        }
    }
    
    return messagesProcessed;
}

void CANInterface::flushReceiveQueue() {
    while (!receiveQueue.empty()) {
        receiveQueue.pop();
    }
    
    // Also flush TWAI driver queue
    twai_message_t dummy_msg;
    while (twai_receive(&dummy_msg, 0) == ESP_OK) {
        // Discard messages
    }
}

void CANInterface::setMessageCallback(CANMessageCallback callback) {
    messageCallback = callback;
}

// ===== FILTERING =====

void CANInterface::setMessageFilter(const CANFilter& filter) {
    messageFilter = filter;
    Serial.print(F("[CAN] Message filter set to: "));
    
    switch (filter.type) {
        case CANFilterType::ACCEPT_ALL:
            Serial.println("ACCEPT_ALL");
            break;
        case CANFilterType::WHITELIST:
            Serial.printf("WHITELIST (%d IDs)\n", filter.whitelist.size());
            break;
        case CANFilterType::BLACKLIST:
            Serial.printf("BLACKLIST (%d IDs)\n", filter.blacklist.size());
            break;
        case CANFilterType::RANGE:
            Serial.printf("RANGE (0x%X - 0x%X)\n", filter.rangeStart, filter.rangeEnd);
            break;
        case CANFilterType::CUSTOM:
            Serial.println("CUSTOM");
            break;
    }
}

void CANInterface::setAcceptAllFilter() {
    CANFilter filter;
    filter.type = CANFilterType::ACCEPT_ALL;
    setMessageFilter(filter);
}

void CANInterface::setWhitelistFilter(const std::vector<uint32_t>& ids) {
    CANFilter filter;
    filter.type = CANFilterType::WHITELIST;
    filter.whitelist = ids;
    setMessageFilter(filter);
}

void CANInterface::setBlacklistFilter(const std::vector<uint32_t>& ids) {
    CANFilter filter;
    filter.type = CANFilterType::BLACKLIST;
    filter.blacklist = ids;
    setMessageFilter(filter);
}

void CANInterface::setRangeFilter(uint32_t startId, uint32_t endId) {
    CANFilter filter;
    filter.type = CANFilterType::RANGE;
    filter.rangeStart = startId;
    filter.rangeEnd = endId;
    setMessageFilter(filter);
}

void CANInterface::setCustomFilter(std::function<bool(const CANMessage&)> filterFunc) {
    CANFilter filter;
    filter.type = CANFilterType::CUSTOM;
    filter.customFilter = filterFunc;
    setMessageFilter(filter);
}

void CANInterface::setFilterEnabled(bool enabled) {
    messageFilter.enabled = enabled;
    Serial.printf("[CAN] Message filter %s\n", enabled ? "ENABLED" : "DISABLED");
}

bool CANInterface::applyMessageFilter(const CANMessage& message) {
    if (!messageFilter.enabled) {
        return true;
    }
    
    switch (messageFilter.type) {
        case CANFilterType::ACCEPT_ALL:
            return true;
            
        case CANFilterType::WHITELIST:
            for (uint32_t id : messageFilter.whitelist) {
                if (message.id == id) {
                    return true;
                }
            }
            return false;
            
        case CANFilterType::BLACKLIST:
            for (uint32_t id : messageFilter.blacklist) {
                if (message.id == id) {
                    return false;
                }
            }
            return true;
            
        case CANFilterType::RANGE:
            return (message.id >= messageFilter.rangeStart && 
                   message.id <= messageFilter.rangeEnd);
            
        case CANFilterType::CUSTOM:
            if (messageFilter.customFilter) {
                return messageFilter.customFilter(message);
            }
            return true;
            
        default:
            return true;
    }
}

// ===== STATUS & DIAGNOSTICS =====

String CANInterface::getStatus() const {
    if (!interfaceEnabled) {
        return "DISABLED";
    }
    
    if (busOff) {
        return "BUS_OFF";
    }
    
    twai_status_info_t status_info;
    esp_err_t result = twai_get_status_info(&status_info);
    
    if (result == ESP_OK) {
        switch (status_info.state) {
            case TWAI_STATE_STOPPED:
                return "STOPPED";
            case TWAI_STATE_RUNNING:
                return "RUNNING";
            case TWAI_STATE_BUS_OFF:
                return "BUS_OFF";
            case TWAI_STATE_RECOVERING:
                return "RECOVERING";
            default:
                return "UNKNOWN";
        }
    }
    
    return "ERROR";
}

bool CANInterface::isBusOff() const {
    return busOff;
}

uint16_t CANInterface::getErrorState() const {
    if (!interfaceEnabled) {
        return 0xFFFF;
    }
    
    twai_status_info_t status_info;
    esp_err_t result = twai_get_status_info(&status_info);
    
    if (result == ESP_OK) {
        return (status_info.tx_error_counter << 8) | status_info.rx_error_counter;
    }
    
    return 0;
}

CANStatistics CANInterface::getStatistics() const {
    CANStatistics stats = statistics;
    stats.uptimeSeconds = (millis() - interfaceStartTime) / 1000;
    return stats;
}

void CANInterface::resetStatistics() {
    statistics = CANStatistics();
    interfaceStartTime = millis();
}

void CANInterface::printDiagnostics() const {
    Serial.println(F("=== CAN Interface Diagnostics ==="));
    Serial.printf("Status: %s\n", getStatus().c_str());
    Serial.printf("Speed: %d bps\n", getSpeedBPS(currentSpeed));
    Serial.print("Mode: ");
    switch (currentMode) {
        case CANMode::NORMAL:
            Serial.println("NORMAL");
            break;
        case CANMode::LISTEN_ONLY:
            Serial.println("LISTEN_ONLY");
            break;
        case CANMode::SELF_TEST:
            Serial.println("SELF_TEST");
            break;
        case CANMode::NO_ACK:
            Serial.println("NO_ACK");
            break;
    }
    
    CANStatistics stats = getStatistics();
    Serial.printf("Messages RX: %d\n", stats.messagesReceived);
    Serial.printf("Messages TX: %d\n", stats.messagesSent);
    Serial.printf("Error frames: %d\n", stats.errorFrames);
    Serial.printf("Bus-off events: %d\n", stats.busOffEvents);
    Serial.printf("RX queue size: %d\n", receiveQueue.size());
    Serial.printf("TX queue size: %d\n", transmitQueue.size());
    Serial.printf("Filter rejects: %d\n", stats.filterRejects);
    Serial.printf("Uptime: %d seconds\n", stats.uptimeSeconds);
    
    if (interfaceEnabled) {
        twai_status_info_t status_info;
        if (twai_get_status_info(&status_info) == ESP_OK) {
            Serial.printf("TX error count: %d\n", status_info.tx_error_counter);
            Serial.printf("RX error count: %d\n", status_info.rx_error_counter);
            Serial.printf("TX failed count: %d\n", status_info.tx_failed_count);
            Serial.printf("RX missed count: %d\n", status_info.rx_missed_count);
            Serial.printf("RX overrun count: %d\n", status_info.rx_overrun_count);
            Serial.printf("Arbitration lost: %d\n", status_info.arb_lost_count);
            Serial.printf("Bus error count: %d\n", status_info.bus_error_count);
        }
    }
    
    Serial.println(F("================================"));
}

void CANInterface::setErrorCallback(CANErrorCallback callback) {
    errorCallback = callback;
}

// ===== OBD2 SPECIFIC FUNCTIONS =====

bool CANInterface::initializeOBD2() {
    Serial.println(F("[CAN] Initializing for OBD2 communication..."));
    
    // Set up OBD2-specific filter
    std::vector<uint32_t> obd2_ids = {
        OBD2CAN::FUNCTIONAL_REQUEST_ID,
        OBD2CAN::RESPONSE_ID_BASE,
        OBD2CAN::RESPONSE_ID_BASE + 1,
        OBD2CAN::RESPONSE_ID_BASE + 2,
        OBD2CAN::RESPONSE_ID_BASE + 3,
        OBD2CAN::RESPONSE_ID_BASE + 4,
        OBD2CAN::RESPONSE_ID_BASE + 5,
        OBD2CAN::RESPONSE_ID_BASE + 6,
        OBD2CAN::RESPONSE_ID_BASE + 7
    };
    
    setWhitelistFilter(obd2_ids);
    
    Serial.println(F("[CAN] OBD2 initialization complete"));
    return true;
}

bool CANInterface::sendOBD2Functional(const uint8_t* data, uint8_t length) {
    return sendMessage(OBD2CAN::FUNCTIONAL_REQUEST_ID, data, length, false, 1000);
}

bool CANInterface::sendOBD2Physical(uint32_t ecuId, const uint8_t* data, uint8_t length) {
    uint32_t requestId = OBD2CAN::PHYSICAL_REQUEST_BASE + (ecuId & 0x07);
    return sendMessage(requestId, data, length, false, 1000);
}

bool CANInterface::waitOBD2Response(CANMessage& response, uint32_t timeout) {
    unsigned long startTime = millis();
    
    while ((millis() - startTime) < timeout) {
        if (receiveMessage(response, 10)) {
            if (isOBD2Response(response)) {
                return true;
            }
        }
    }
    
    return false;
}

bool CANInterface::isOBD2Response(const CANMessage& message) {
    return (message.id >= OBD2CAN::RESPONSE_ID_BASE && 
            message.id <= (OBD2CAN::RESPONSE_ID_BASE + 7));
}

bool CANInterface::parseOBD2Response(const CANMessage& message, uint16_t& pid, 
                                    uint8_t* data, uint8_t& length) {
    if (!isOBD2Response(message) || message.dlc < 3) {
        return false;
    }
    
    // Check for single frame response (PCI = 0x0X)
    if ((message.data[0] & 0xF0) == 0x00) {
        uint8_t dataLength = message.data[0] & 0x0F;
        if (dataLength < 2 || dataLength > 7) {
            return false;
        }
        
        // Extract mode and PID
        uint8_t mode = message.data[1];
        uint8_t pidByte = message.data[2];
        pid = (mode << 8) | pidByte;
        
        // Extract data
        length = dataLength - 2; // Subtract mode and PID bytes
        if (length > 0) {
            memcpy(data, &message.data[3], length);
        }
        
        return true;
    }
    
    // Multi-frame responses not implemented yet
    return false;
}

// ===== UTILITY FUNCTIONS =====

String CANInterface::messageToString(const CANMessage& message) {
    String str = "";
    
    // Add timestamp
    str += String(message.timestamp) + ":";
    
    // Add ID
    if (message.extd) {
        str += String(message.id, HEX) + "#";
    } else {
        str += String(message.id, HEX) + "#";
    }
    
    // Add data
    for (int i = 0; i < message.dlc; i++) {
        if (message.data[i] < 0x10) str += "0";
        str += String(message.data[i], HEX);
    }
    
    // Add flags
    if (message.rtr) str += "#RTR";
    if (message.extd) str += "#EXT";
    
    str.toUpperCase();
    return str;
}

bool CANInterface::stringToMessage(const String& str, CANMessage& message) {
    // Simple parser for basic CAN message format: ID#DATA
    int hashPos = str.indexOf('#');
    if (hashPos == -1) {
        return false;
    }
    
    // Parse ID
    String idStr = str.substring(0, hashPos);
    message.id = strtoul(idStr.c_str(), NULL, 16);
    
    // Parse data
    String dataStr = str.substring(hashPos + 1);
    message.dlc = dataStr.length() / 2;
    
    if (message.dlc > 8) {
        return false;
    }
    
    for (int i = 0; i < message.dlc; i++) {
        String byteStr = dataStr.substring(i * 2, (i * 2) + 2);
        message.data[i] = strtoul(byteStr.c_str(), NULL, 16);
    }
    
    message.type = CANMessageType::STANDARD;
    message.extd = false;
    message.rtr = false;
    message.timestamp = millis();
    
    return true;
}

uint8_t CANInterface::calculateChecksum(const CANMessage& message) {
    uint8_t checksum = 0;
    checksum += (message.id >> 24) & 0xFF;
    checksum += (message.id >> 16) & 0xFF;
    checksum += (message.id >> 8) & 0xFF;
    checksum += message.id & 0xFF;
    checksum += message.dlc;
    
    for (int i = 0; i < message.dlc; i++) {
        checksum += message.data[i];
    }
    
    return checksum;
}

bool CANInterface::validateMessage(const CANMessage& message) {
    // Check DLC
    if (message.dlc > 8) {
        return false;
    }
    
    // Check ID ranges
    if (message.extd && message.id > 0x1FFFFFFF) {
        return false;
    }
    
    if (!message.extd && message.id > 0x7FF) {
        return false;
    }
    
    return true;
}

bool CANInterface::getTimingConfig(CANSpeed speed, twai_timing_config_t& timing) {
    switch (speed) {
        case CANSpeed::CAN_125KBPS:
            timing = TWAI_TIMING_CONFIG_125KBITS();
            return true;
        case CANSpeed::CAN_250KBPS:
            timing = TWAI_TIMING_CONFIG_250KBITS();
            return true;
        case CANSpeed::CAN_500KBPS:
            timing = TWAI_TIMING_CONFIG_500KBITS();
            return true;
        case CANSpeed::CAN_1MBPS:
            timing = TWAI_TIMING_CONFIG_1MBITS();
            return true;
        default:
            return false;
    }
}

void CANInterface::convertFromTWAI(const twai_message_t& twaiMsg, CANMessage& canMsg) {
    canMsg.id = twaiMsg.identifier;
    canMsg.dlc = twaiMsg.data_length_code;
    canMsg.extd = twaiMsg.extd;
    canMsg.rtr = twaiMsg.rtr;
    canMsg.type = twaiMsg.extd ? CANMessageType::EXTENDED : CANMessageType::STANDARD;
    canMsg.timestamp = millis();
    canMsg.errorFlags = 0;
    
    memcpy(canMsg.data, twaiMsg.data, canMsg.dlc);
}

void CANInterface::convertToTWAI(const CANMessage& canMsg, twai_message_t& twaiMsg) {
    twaiMsg.identifier = canMsg.id;
    twaiMsg.data_length_code = canMsg.dlc;
    twaiMsg.extd = canMsg.extd;
    twaiMsg.rtr = canMsg.rtr;
    
    memcpy(twaiMsg.data, canMsg.data, canMsg.dlc);
}

// ===== INTERNAL METHODS =====

void CANInterface::handleCANError(uint16_t errorCode) {
    statistics.errorFrames++;
    
    // Check for bus-off condition
    if (errorCode == ESP_ERR_INVALID_STATE) {
        busOff = true;
        statistics.busOffEvents++;
    }
    
    String errorDesc = getErrorDescription(errorCode);
    Serial.printf("[CAN] ERROR: %s (code: 0x%X)\n", errorDesc.c_str(), errorCode);
    
    if (errorCallback) {
        errorCallback(errorCode, errorDesc);
    }
}

void CANInterface::updateStatistics() {
    // Calculate bus utilization (simplified)
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - statistics.lastMessageTime;
    
    if (timeDiff > 0) {
        uint32_t totalMessages = statistics.messagesReceived + statistics.messagesSent;
        float messagesPerSecond = (float)totalMessages / ((currentTime - interfaceStartTime) / 1000.0);
        
        // Estimate utilization based on message rate and average message size
        float avgMessageBits = 64 + 47; // 64-bit data + overhead
        float bitsPerSecond = messagesPerSecond * avgMessageBits;
        statistics.busUtilization = (bitsPerSecond / getSpeedBPS(currentSpeed)) * 100.0;
        
        if (statistics.busUtilization > 100.0) {
            statistics.busUtilization = 100.0;
        }
    }
}

String CANInterface::getErrorDescription(uint16_t errorCode) {
    switch (errorCode) {
        case ESP_OK:
            return "No error";
        case ESP_ERR_INVALID_ARG:
            return "Invalid argument";
        case ESP_ERR_TIMEOUT:
            return "Timeout";
        case ESP_ERR_INVALID_STATE:
            return "Driver not started or bus-off";
        case ESP_ERR_NOT_SUPPORTED:
            return "Operation not supported";
        case ESP_FAIL:
            return "General failure";
        default:
            return "Unknown error";
    }
}