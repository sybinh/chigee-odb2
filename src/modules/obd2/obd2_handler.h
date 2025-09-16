#pragma once

/**
 * @file obd2_handler.h
 * @brief Professional OBD2 protocol handler with ELM327 emulation
 * @author Chigee OBD2 Project Team
 * @date 2025-09-15
 * 
 * This module handles OBD2 protocol processing, AT command emulation,
 * and vehicle data management for Chigee XR2 compatibility.
 */

#include <map>
#include <vector>
#include "../../config/project_config.h"
#include "../../config/hardware_config.h"

/**
 * @brief OBD2 protocol types
 */
enum class OBD2Protocol {
    AUTO_DETECT,        // Automatic protocol detection
    ISO_15765_4_CAN,    // ISO 15765-4 (CAN 11-bit, 500kbps) - Most common
    ISO_15765_4_CAN_B,  // ISO 15765-4 (CAN 29-bit, 500kbps)
    ISO_14230_4_KWP,    // ISO 14230-4 KWP2000 (Fast init)
    ISO_9141_2,         // ISO 9141-2 (K-Line)
    SAE_J1850_PWM,      // SAE J1850 PWM (41.6kbps)
    SAE_J1850_VPW,      // SAE J1850 VPW (10.4kbps)
    UNKNOWN
};

/**
 * @brief AT command processing states
 */
enum class ATCommandState {
    WAITING_RESET,      // Expecting ATZ
    ECHO_CONFIG,        // ATE0/ATE1 commands
    PROTOCOL_SELECT,    // ATSP commands
    HEADERS_CONFIG,     // ATH commands
    READY,              // Ready for OBD commands
    ERROR_STATE         // Error condition
};

/**
 * @brief Vehicle data simulation modes
 */
enum class SimulationMode {
    STATIC,             // Fixed values
    REALISTIC,          // Realistic variation
    LIVE_CAN,           // Real CAN bus data
    RECORDED_DATA       // Playback recorded data
};

/**
 * @brief PID data structure
 */
struct PIDData {
    uint16_t pid;           // Parameter ID
    String description;     // Human-readable description
    uint8_t dataBytes;      // Number of data bytes
    String unit;            // Units (RPM, km/h, °C, etc.)
    float minValue;         // Minimum possible value
    float maxValue;         // Maximum possible value
    bool supported;         // Whether this PID is supported
    unsigned long lastUpdate;  // Last update timestamp
    float currentValue;     // Current calculated value
    uint8_t rawData[8];     // Raw CAN data bytes
};

/**
 * @brief Vehicle state information
 */
struct VehicleState {
    // Engine parameters
    float engineRPM;            // Engine speed (RPM)
    float vehicleSpeed;         // Vehicle speed (km/h)
    float engineLoad;           // Calculated engine load (%)
    float throttlePosition;     // Throttle position (%)
    float coolantTemperature;   // Engine coolant temperature (°C)
    float intakeAirTemp;        // Intake air temperature (°C)
    float fuelPressure;         // Fuel rail pressure (kPa)
    
    // Electrical system
    float batteryVoltage;       // Control module voltage (V)
    float alternatorVoltage;    // Charging system voltage (V)
    
    // Fuel system
    float fuelLevel;            // Fuel tank level (%)
    float fuelConsumption;      // Instantaneous fuel consumption (L/h)
    
    // Environmental
    float ambientTemperature;   // Ambient air temperature (°C)
    float barometricPressure;   // Barometric pressure (kPa)
    
    // Status flags
    bool engineRunning;         // Engine is running
    bool diagnosticTrouble;     // DTC present
    uint16_t troubleCodes;      // Number of stored DTCs
    
    // Timing
    unsigned long lastUpdate;   // Last update timestamp
    uint32_t updateCount;       // Number of updates
};

/**
 * @brief AT command response structure
 */
struct ATResponse {
    String command;             // Original AT command
    String response;            // Response string
    bool success;               // Command successful
    unsigned long processingTime; // Processing time in ms
};

/**
 * @class OBD2Handler
 * @brief Professional OBD2 protocol handler with advanced features
 */
class OBD2Handler {
private:
    // Protocol state
    OBD2Protocol currentProtocol;
    ATCommandState commandState;
    SimulationMode simulationMode;
    
    // Vehicle data
    VehicleState vehicleState;
    std::map<uint16_t, PIDData> supportedPIDs;
    
    // Configuration
    bool echoEnabled;
    bool headersEnabled;
    bool linefeedsEnabled;
    bool spacesEnabled;
    String deviceInfo;
    String protocolDescription;
    
    // Statistics
    uint32_t commandsProcessed;
    uint32_t pidQueriesHandled;
    uint32_t errorCount;
    unsigned long totalProcessingTime;
    
    // Internal methods
    void initializePIDDatabase();
    void initializeVehicleState();
    String processATCommand(const String& command);
    String processOBDCommand(const String& command);
    String processPIDQuery(uint16_t pid);
    String formatPIDResponse(uint16_t pid, const uint8_t* data, uint8_t length);
    void updateVehicleSimulation();
    bool validatePIDRequest(uint16_t pid);
    float calculatePIDValue(uint16_t pid);
    void encodePIDData(uint16_t pid, float value, uint8_t* data);
    String getProtocolDescription(OBD2Protocol protocol);
    
public:
    /**
     * @brief Constructor
     */
    OBD2Handler();
    
    /**
     * @brief Destructor
     */
    ~OBD2Handler();
    
    // ===== INITIALIZATION =====
    
    /**
     * @brief Initialize OBD2 handler
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Reset handler to initial state
     */
    void reset();
    
    /**
     * @brief Set simulation mode
     * @param mode Simulation mode to use
     */
    void setSimulationMode(SimulationMode mode);
    
    // ===== COMMAND PROCESSING =====
    
    /**
     * @brief Process incoming command (AT or OBD)
     * @param command Command string to process
     * @return Response string
     */
    String processCommand(const String& command);
    
    /**
     * @brief Process AT command with detailed response
     * @param command AT command string
     * @return ATResponse structure with details
     */
    ATResponse processATCommandDetailed(const String& command);
    
    /**
     * @brief Get current command state
     * @return Current AT command state
     */
    ATCommandState getCommandState() const;
    
    // ===== PROTOCOL MANAGEMENT =====
    
    /**
     * @brief Set OBD2 protocol
     * @param protocol Protocol to use
     * @return true if protocol set successfully
     */
    bool setProtocol(OBD2Protocol protocol);
    
    /**
     * @brief Get current protocol
     * @return Current OBD2 protocol
     */
    OBD2Protocol getCurrentProtocol() const;
    
    /**
     * @brief Get protocol description string
     * @return Protocol description
     */
    String getCurrentProtocolDescription() const;
    
    /**
     * @brief Auto-detect protocol (simulation)
     * @return Detected protocol
     */
    OBD2Protocol autoDetectProtocol();
    
    // ===== PID MANAGEMENT =====
    
    /**
     * @brief Add supported PID
     * @param pid Parameter ID
     * @param description PID description
     * @param dataBytes Number of data bytes
     * @param unit Units string
     * @param minVal Minimum value
     * @param maxVal Maximum value
     */
    void addSupportedPID(uint16_t pid, const String& description, 
                        uint8_t dataBytes, const String& unit,
                        float minVal, float maxVal);
    
    /**
     * @brief Check if PID is supported
     * @param pid Parameter ID to check
     * @return true if PID is supported
     */
    bool isPIDSupported(uint16_t pid) const;
    
    /**
     * @brief Get supported PIDs list
     * @return Vector of supported PID numbers
     */
    std::vector<uint16_t> getSupportedPIDs() const;
    
    /**
     * @brief Get PID information
     * @param pid Parameter ID
     * @return PID data structure
     */
    PIDData getPIDInfo(uint16_t pid) const;
    
    // ===== VEHICLE DATA =====
    
    /**
     * @brief Update vehicle state with new data
     * @param state New vehicle state
     */
    void updateVehicleState(const VehicleState& state);
    
    /**
     * @brief Get current vehicle state
     * @return Current vehicle state
     */
    VehicleState getVehicleState() const;
    
    /**
     * @brief Set specific vehicle parameter
     * @param pid Parameter ID
     * @param value New value
     */
    void setVehicleParameter(uint16_t pid, float value);
    
    /**
     * @brief Get specific vehicle parameter
     * @param pid Parameter ID
     * @return Current parameter value
     */
    float getVehicleParameter(uint16_t pid) const;
    
    // ===== CONFIGURATION =====
    
    /**
     * @brief Enable/disable echo
     * @param enable Echo enabled state
     */
    void setEchoEnabled(bool enable);
    
    /**
     * @brief Enable/disable headers in responses
     * @param enable Headers enabled state
     */
    void setHeadersEnabled(bool enable);
    
    /**
     * @brief Enable/disable linefeeds
     * @param enable Linefeeds enabled state
     */
    void setLinefeedsEnabled(bool enable);
    
    /**
     * @brief Enable/disable spaces in responses
     * @param enable Spaces enabled state
     */
    void setSpacesEnabled(bool enable);
    
    /**
     * @brief Set device information string
     * @param info Device info string
     */
    void setDeviceInfo(const String& info);
    
    // ===== STATISTICS =====
    
    /**
     * @brief Get processing statistics
     * @return Statistics string
     */
    String getStatistics() const;
    
    /**
     * @brief Reset statistics counters
     */
    void resetStatistics();
    
    /**
     * @brief Get average response time
     * @return Average response time in milliseconds
     */
    float getAverageResponseTime() const;
    
    /**
     * @brief Print diagnostic information
     */
    void printDiagnostics() const;
    
    // ===== UTILITY FUNCTIONS =====
    
    /**
     * @brief Convert hex string to bytes
     * @param hexString Hex string (e.g., "01 0C")
     * @param bytes Output byte array
     * @param maxBytes Maximum bytes to convert
     * @return Number of bytes converted
     */
    static int hexStringToBytes(const String& hexString, uint8_t* bytes, int maxBytes);
    
    /**
     * @brief Convert bytes to hex string
     * @param bytes Input byte array
     * @param length Number of bytes
     * @param spaces Include spaces between bytes
     * @return Hex string
     */
    static String bytesToHexString(const uint8_t* bytes, int length, bool spaces = true);
    
    /**
     * @brief Calculate checksum for data
     * @param data Data bytes
     * @param length Number of bytes
     * @return Checksum value
     */
    static uint8_t calculateChecksum(const uint8_t* data, int length);
    
    /**
     * @brief Parse PID from command string
     * @param command OBD command string
     * @return PID number or 0 if invalid
     */
    static uint16_t parsePIDFromCommand(const String& command);
    
    /**
     * @brief Get PID name from number
     * @param pid PID number
     * @return Human-readable PID name
     */
    static String getPIDName(uint16_t pid);
};

// ===== COMMON PID DEFINITIONS =====
namespace StandardPIDs {
    // Mode 01 - Current data
    constexpr uint16_t SUPPORTED_PIDS_01_20     = 0x0100;  // Supported PIDs 01-20
    constexpr uint16_t MONITOR_STATUS           = 0x0101;  // Monitor status since DTCs cleared
    constexpr uint16_t FREEZE_DTC               = 0x0102;  // Freeze DTC
    constexpr uint16_t FUEL_SYSTEM_STATUS       = 0x0103;  // Fuel system status
    constexpr uint16_t ENGINE_LOAD              = 0x0104;  // Calculated engine load
    constexpr uint16_t COOLANT_TEMPERATURE      = 0x0105;  // Engine coolant temperature
    constexpr uint16_t SHORT_FUEL_TRIM_1        = 0x0106;  // Short term fuel trim—Bank 1
    constexpr uint16_t LONG_FUEL_TRIM_1         = 0x0107;  // Long term fuel trim—Bank 1
    constexpr uint16_t SHORT_FUEL_TRIM_2        = 0x0108;  // Short term fuel trim—Bank 2
    constexpr uint16_t LONG_FUEL_TRIM_2         = 0x0109;  // Long term fuel trim—Bank 2
    constexpr uint16_t FUEL_PRESSURE            = 0x010A;  // Fuel pressure
    constexpr uint16_t MAP_PRESSURE             = 0x010B;  // Intake manifold absolute pressure
    constexpr uint16_t ENGINE_RPM               = 0x010C;  // Engine RPM
    constexpr uint16_t VEHICLE_SPEED            = 0x010D;  // Vehicle speed
    constexpr uint16_t TIMING_ADVANCE           = 0x010E;  // Timing advance
    constexpr uint16_t INTAKE_AIR_TEMP          = 0x010F;  // Intake air temperature
    constexpr uint16_t MAF_FLOW_RATE            = 0x0110;  // MAF air flow rate
    constexpr uint16_t THROTTLE_POSITION        = 0x0111;  // Throttle position
    constexpr uint16_t SECONDARY_AIR_STATUS     = 0x0112;  // Secondary air status
    constexpr uint16_t OXYGEN_SENSORS_PRESENT   = 0x0113;  // Oxygen sensors present
    constexpr uint16_t OXYGEN_SENSOR_1_A        = 0x0114;  // Oxygen sensor 1 A
    constexpr uint16_t OXYGEN_SENSOR_1_B        = 0x0115;  // Oxygen sensor 1 B
    constexpr uint16_t OXYGEN_SENSOR_2_A        = 0x0116;  // Oxygen sensor 2 A
    constexpr uint16_t OXYGEN_SENSOR_2_B        = 0x0117;  // Oxygen sensor 2 B
    constexpr uint16_t OBD_STANDARDS            = 0x011C;  // OBD standards this vehicle conforms to
    constexpr uint16_t SUPPORTED_PIDS_21_40     = 0x0120;  // Supported PIDs 21-40
    constexpr uint16_t DISTANCE_WITH_MIL        = 0x0121;  // Distance traveled with MIL on
    constexpr uint16_t FUEL_RAIL_PRESSURE       = 0x0122;  // Fuel rail pressure
    constexpr uint16_t FUEL_RAIL_GAUGE_PRESSURE = 0x0123;  // Fuel rail gauge pressure
    constexpr uint16_t COMMANDED_EGR            = 0x012C;  // Commanded EGR
    constexpr uint16_t EGR_ERROR                = 0x012D;  // EGR Error
    constexpr uint16_t COMMANDED_EVAP_PURGE     = 0x012E;  // Commanded evaporative purge
    constexpr uint16_t FUEL_TANK_LEVEL          = 0x012F;  // Fuel tank level input
    constexpr uint16_t RUNTIME_SINCE_START      = 0x011F;  // Runtime since engine start
    constexpr uint16_t SUPPORTED_PIDS_41_60     = 0x0140;  // Supported PIDs 41-60
    constexpr uint16_t CONTROL_MODULE_VOLTAGE   = 0x0142;  // Control module power supply voltage
    constexpr uint16_t ABSOLUTE_LOAD_VALUE      = 0x0143;  // Absolute load value
    constexpr uint16_t FUEL_AIR_COMMANDED_EQUIV = 0x0144;  // Fuel–air commanded equivalence ratio
    constexpr uint16_t RELATIVE_THROTTLE_POS    = 0x0145;  // Relative throttle position
    constexpr uint16_t AMBIENT_AIR_TEMP         = 0x0146;  // Ambient air temperature
    constexpr uint16_t ABSOLUTE_THROTTLE_POS_B  = 0x0147;  // Absolute throttle position B
    constexpr uint16_t ABSOLUTE_THROTTLE_POS_C  = 0x0148;  // Absolute throttle position C
    constexpr uint16_t ACCELERATOR_PEDAL_POS_D  = 0x0149;  // Accelerator pedal position D
    constexpr uint16_t ACCELERATOR_PEDAL_POS_E  = 0x014A;  // Accelerator pedal position E
    constexpr uint16_t ACCELERATOR_PEDAL_POS_F  = 0x014B;  // Accelerator pedal position F
    constexpr uint16_t COMMANDED_THROTTLE_ACT   = 0x014C;  // Commanded throttle actuator
}

#endif // OBD2_HANDLER_H