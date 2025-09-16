/**
 * @file obd2_handler.cpp
 * @brief Professional OBD2 protocol handler implementation
 * @author Chigee OBD2 Project Team
 * @date 2025-09-15
 */

#include "obd2_handler.h"
#include <Arduino.h>

// ===== CONSTRUCTOR & DESTRUCTOR =====

OBD2Handler::OBD2Handler() :
    currentProtocol(OBD2Protocol::AUTO_DETECT),
    commandState(ATCommandState::WAITING_RESET),
    simulationMode(SimulationMode::REALISTIC),
    echoEnabled(true),
    headersEnabled(false),
    linefeedsEnabled(true),
    spacesEnabled(true),
    deviceInfo("ELM327 v1.5"),
    protocolDescription("AUTO"),
    commandsProcessed(0),
    pidQueriesHandled(0),
    errorCount(0),
    totalProcessingTime(0)
{
    initializePIDDatabase();
    initializeVehicleState();
}

OBD2Handler::~OBD2Handler() {
    // Cleanup resources if needed
}

// ===== INITIALIZATION =====

bool OBD2Handler::initialize() {
    Serial.println(F("[OBD2] Initializing OBD2 handler..."));
    
    // Reset to initial state
    reset();
    
    // Initialize PID database
    initializePIDDatabase();
    
    // Initialize vehicle state
    initializeVehicleState();
    
    Serial.println(F("[OBD2] Handler initialized successfully"));
    return true;
}

void OBD2Handler::reset() {
    commandState = ATCommandState::WAITING_RESET;
    currentProtocol = OBD2Protocol::AUTO_DETECT;
    echoEnabled = true;
    headersEnabled = false;
    linefeedsEnabled = true;
    spacesEnabled = true;
    
    // Reset statistics
    commandsProcessed = 0;
    pidQueriesHandled = 0;
    errorCount = 0;
    totalProcessingTime = 0;
    
    Serial.println(F("[OBD2] Handler reset to initial state"));
}

void OBD2Handler::setSimulationMode(SimulationMode mode) {
    simulationMode = mode;
    Serial.print(F("[OBD2] Simulation mode set to: "));
    switch (mode) {
        case SimulationMode::STATIC:
            Serial.println(F("STATIC"));
            break;
        case SimulationMode::REALISTIC:
            Serial.println(F("REALISTIC"));
            break;
        case SimulationMode::LIVE_CAN:
            Serial.println(F("LIVE_CAN"));
            break;
        case SimulationMode::RECORDED_DATA:
            Serial.println(F("RECORDED_DATA"));
            break;
    }
}

void OBD2Handler::initializePIDDatabase() {
    // Clear existing PIDs
    supportedPIDs.clear();
    
    // Add standard PIDs with realistic ranges
    addSupportedPID(StandardPIDs::SUPPORTED_PIDS_01_20, "Supported PIDs 01-20", 4, "", 0, 0);
    addSupportedPID(StandardPIDs::ENGINE_LOAD, "Engine Load", 1, "%", 0, 100);
    addSupportedPID(StandardPIDs::COOLANT_TEMPERATURE, "Coolant Temperature", 1, "°C", -40, 215);
    addSupportedPID(StandardPIDs::ENGINE_RPM, "Engine RPM", 2, "RPM", 0, 16383.75);
    addSupportedPID(StandardPIDs::VEHICLE_SPEED, "Vehicle Speed", 1, "km/h", 0, 255);
    addSupportedPID(StandardPIDs::INTAKE_AIR_TEMP, "Intake Air Temperature", 1, "°C", -40, 215);
    addSupportedPID(StandardPIDs::MAF_FLOW_RATE, "MAF Flow Rate", 2, "g/s", 0, 655.35);
    addSupportedPID(StandardPIDs::THROTTLE_POSITION, "Throttle Position", 1, "%", 0, 100);
    addSupportedPID(StandardPIDs::RUNTIME_SINCE_START, "Runtime Since Start", 2, "s", 0, 65535);
    addSupportedPID(StandardPIDs::FUEL_TANK_LEVEL, "Fuel Tank Level", 1, "%", 0, 100);
    addSupportedPID(StandardPIDs::CONTROL_MODULE_VOLTAGE, "Control Module Voltage", 2, "V", 0, 65.535);
    addSupportedPID(StandardPIDs::AMBIENT_AIR_TEMP, "Ambient Air Temperature", 1, "°C", -40, 215);
    addSupportedPID(StandardPIDs::FUEL_PRESSURE, "Fuel Pressure", 1, "kPa", 0, 765);
    addSupportedPID(StandardPIDs::MAP_PRESSURE, "MAP Pressure", 1, "kPa", 0, 255);
    addSupportedPID(StandardPIDs::TIMING_ADVANCE, "Timing Advance", 1, "°", -64, 63.5);
    
    Serial.printf("[OBD2] Initialized %d PIDs in database\n", supportedPIDs.size());
}

void OBD2Handler::initializeVehicleState() {
    // Initialize with realistic default values
    vehicleState.engineRPM = 800.0;           // Idle RPM
    vehicleState.vehicleSpeed = 0.0;          // Stationary
    vehicleState.engineLoad = 15.0;           // Light load at idle
    vehicleState.throttlePosition = 0.0;      // Closed throttle
    vehicleState.coolantTemperature = 90.0;   // Normal operating temp
    vehicleState.intakeAirTemp = 25.0;        // Ambient temperature
    vehicleState.fuelPressure = 300.0;        // Normal fuel pressure
    vehicleState.batteryVoltage = 12.6;       // Good battery
    vehicleState.alternatorVoltage = 14.2;    // Charging
    vehicleState.fuelLevel = 75.0;            // 3/4 tank
    vehicleState.fuelConsumption = 2.5;       // L/h at idle
    vehicleState.ambientTemperature = 22.0;   // Room temperature
    vehicleState.barometricPressure = 101.3;  // Sea level
    vehicleState.engineRunning = true;        // Engine running
    vehicleState.diagnosticTrouble = false;   // No DTCs
    vehicleState.troubleCodes = 0;            // No codes
    vehicleState.lastUpdate = millis();
    vehicleState.updateCount = 0;
    
    Serial.println(F("[OBD2] Vehicle state initialized with default values"));
}

// ===== COMMAND PROCESSING =====

String OBD2Handler::processCommand(const String& command) {
    unsigned long startTime = millis();
    commandsProcessed++;
    
    String cleanCommand = command;
    cleanCommand.trim();
    cleanCommand.toUpperCase();
    
    String response;
    
    // Handle AT commands
    if (cleanCommand.startsWith("AT")) {
        response = processATCommand(cleanCommand);
    }
    // Handle OBD commands (mode 01, 02, etc.)
    else if (cleanCommand.length() >= 2 && 
             (cleanCommand.startsWith("01") || cleanCommand.startsWith("02") ||
              cleanCommand.startsWith("03") || cleanCommand.startsWith("04") ||
              cleanCommand.startsWith("05") || cleanCommand.startsWith("06") ||
              cleanCommand.startsWith("07") || cleanCommand.startsWith("08") ||
              cleanCommand.startsWith("09"))) {
        response = processOBDCommand(cleanCommand);
        pidQueriesHandled++;
    }
    // Handle empty command (repeat last command)
    else if (cleanCommand.length() == 0) {
        response = "NO DATA";
    }
    // Handle unknown commands
    else {
        response = "?";
        errorCount++;
    }
    
    // Add echo if enabled
    if (echoEnabled && !cleanCommand.startsWith("ATE0")) {
        response = cleanCommand + "\r" + response;
    }
    
    // Add line ending
    if (linefeedsEnabled) {
        response += "\r>";
    } else {
        response += ">";
    }
    
    // Update timing statistics
    unsigned long processingTime = millis() - startTime;
    totalProcessingTime += processingTime;
    
    return response;
}

String OBD2Handler::processATCommand(const String& command) {
    String cmd = command.substring(2); // Remove "AT"
    cmd.trim();
    
    // ATZ - Reset
    if (cmd == "Z") {
        reset();
        commandState = ATCommandState::ECHO_CONFIG;
        return deviceInfo;
    }
    
    // ATE0/ATE1 - Echo control
    else if (cmd == "E0") {
        echoEnabled = false;
        commandState = ATCommandState::PROTOCOL_SELECT;
        return "OK";
    }
    else if (cmd == "E1") {
        echoEnabled = true;
        commandState = ATCommandState::PROTOCOL_SELECT;
        return "OK";
    }
    
    // ATL0/ATL1 - Linefeeds control
    else if (cmd == "L0") {
        linefeedsEnabled = false;
        return "OK";
    }
    else if (cmd == "L1") {
        linefeedsEnabled = true;
        return "OK";
    }
    
    // ATS0/ATS1 - Spaces control
    else if (cmd == "S0") {
        spacesEnabled = false;
        return "OK";
    }
    else if (cmd == "S1") {
        spacesEnabled = true;
        return "OK";
    }
    
    // ATH0/ATH1 - Headers control
    else if (cmd == "H0") {
        headersEnabled = false;
        return "OK";
    }
    else if (cmd == "H1") {
        headersEnabled = true;
        return "OK";
    }
    
    // ATSP - Set protocol
    else if (cmd.startsWith("SP")) {
        String protocolNum = cmd.substring(2);
        int protocol = protocolNum.toInt();
        
        switch (protocol) {
            case 0:
                currentProtocol = OBD2Protocol::AUTO_DETECT;
                protocolDescription = "AUTO";
                break;
            case 1:
                currentProtocol = OBD2Protocol::SAE_J1850_PWM;
                protocolDescription = "SAE J1850 PWM";
                break;
            case 2:
                currentProtocol = OBD2Protocol::SAE_J1850_VPW;
                protocolDescription = "SAE J1850 VPW";
                break;
            case 3:
                currentProtocol = OBD2Protocol::ISO_9141_2;
                protocolDescription = "ISO 9141-2";
                break;
            case 4:
                currentProtocol = OBD2Protocol::ISO_14230_4_KWP;
                protocolDescription = "ISO 14230-4 KWP2000";
                break;
            case 6:
                currentProtocol = OBD2Protocol::ISO_15765_4_CAN;
                protocolDescription = "ISO 15765-4 CAN (11-bit, 500kbps)";
                break;
            case 7:
                currentProtocol = OBD2Protocol::ISO_15765_4_CAN_B;
                protocolDescription = "ISO 15765-4 CAN (29-bit, 500kbps)";
                break;
            default:
                return "?";
        }
        commandState = ATCommandState::READY;
        return "OK";
    }
    
    // ATDP - Describe protocol
    else if (cmd == "DP") {
        if (currentProtocol == OBD2Protocol::AUTO_DETECT) {
            // Simulate auto-detection
            currentProtocol = autoDetectProtocol();
        }
        return protocolDescription;
    }
    
    // ATDPN - Describe protocol by number
    else if (cmd == "DPN") {
        switch (currentProtocol) {
            case OBD2Protocol::SAE_J1850_PWM:
                return "1";
            case OBD2Protocol::SAE_J1850_VPW:
                return "2";
            case OBD2Protocol::ISO_9141_2:
                return "3";
            case OBD2Protocol::ISO_14230_4_KWP:
                return "4";
            case OBD2Protocol::ISO_15765_4_CAN:
                return "6";
            case OBD2Protocol::ISO_15765_4_CAN_B:
                return "7";
            default:
                return "0";
        }
    }
    
    // ATI - Device information
    else if (cmd == "I") {
        return deviceInfo;
    }
    
    // AT@1 - Device description
    else if (cmd == "@1") {
        return "OBDII to RS232 Interpreter";
    }
    
    // AT@2 - Device identifier
    else if (cmd == "@2") {
        return "Chigee OBD2 Module v" PROJECT_VERSION;
    }
    
    // ATRV - Read voltage
    else if (cmd == "RV") {
        return String(vehicleState.batteryVoltage, 1) + "V";
    }
    
    // ATWS - Warm start
    else if (cmd == "WS") {
        // Simulate warm start delay
        delay(100);
        return deviceInfo;
    }
    
    // Unknown AT command
    else {
        errorCount++;
        return "?";
    }
}

String OBD2Handler::processOBDCommand(const String& command) {
    if (commandState != ATCommandState::READY) {
        return "BUS INIT: ...ERROR";
    }
    
    // Update vehicle simulation
    updateVehicleSimulation();
    
    // Parse mode and PID
    if (command.length() < 2) {
        return "NO DATA";
    }
    
    String modeStr = command.substring(0, 2);
    int mode = strtol(modeStr.c_str(), NULL, 16);
    
    // Mode 01 - Current data
    if (mode == 0x01) {
        if (command.length() >= 4) {
            String pidStr = command.substring(2, 4);
            uint16_t pid = 0x0100 | strtol(pidStr.c_str(), NULL, 16);
            return processPIDQuery(pid);
        }
        else {
            return "NO DATA";
        }
    }
    
    // Mode 03 - Show stored DTCs
    else if (mode == 0x03) {
        if (vehicleState.troubleCodes == 0) {
            return "NO DATA";
        } else {
            // Return some dummy DTCs for testing
            return "43 02 P0300 P0171";
        }
    }
    
    // Mode 04 - Clear DTCs
    else if (mode == 0x04) {
        vehicleState.troubleCodes = 0;
        vehicleState.diagnosticTrouble = false;
        return "44";
    }
    
    // Mode 09 - Vehicle information
    else if (mode == 0x09) {
        if (command.length() >= 4) {
            String pidStr = command.substring(2, 4);
            uint8_t pid = strtol(pidStr.c_str(), NULL, 16);
            
            switch (pid) {
                case 0x02: // VIN
                    return "49 02 01 00 00 00 31 44 34 47 50 30 30 42 35 35 42 31 32 33 34 35 36";
                case 0x0A: // ECU name
                    return "49 0A 01 43 48 49 47 45 45 20 45 43 55 00 00 00 00 00 00 00 00";
                default:
                    return "NO DATA";
            }
        }
        return "NO DATA";
    }
    
    // Unsupported mode
    else {
        return "NO DATA";
    }
}

String OBD2Handler::processPIDQuery(uint16_t pid) {
    if (!isPIDSupported(pid)) {
        return "NO DATA";
    }
    
    uint8_t responseData[8] = {0};
    uint8_t dataLength = 0;
    
    // Handle supported PIDs list
    if (pid == StandardPIDs::SUPPORTED_PIDS_01_20) {
        // Build supported PIDs bitmask for 01-20
        uint32_t supportedMask = 0;
        for (auto& pidPair : supportedPIDs) {
            uint16_t pidNum = pidPair.first;
            if (pidNum >= 0x0101 && pidNum <= 0x0120) {
                uint8_t bitPos = pidNum - 0x0101;
                if (bitPos < 32) {
                    supportedMask |= (1UL << (31 - bitPos));
                }
            }
        }
        responseData[0] = (supportedMask >> 24) & 0xFF;
        responseData[1] = (supportedMask >> 16) & 0xFF;
        responseData[2] = (supportedMask >> 8) & 0xFF;
        responseData[3] = supportedMask & 0xFF;
        dataLength = 4;
    }
    
    // Handle specific PIDs
    else {
        float value = calculatePIDValue(pid);
        encodePIDData(pid, value, responseData);
        
        PIDData pidInfo = getPIDInfo(pid);
        dataLength = pidInfo.dataBytes;
    }
    
    return formatPIDResponse(pid, responseData, dataLength);
}

String OBD2Handler::formatPIDResponse(uint16_t pid, const uint8_t* data, uint8_t length) {
    String response = "41 ";
    
    // Add PID number
    uint8_t pidByte = pid & 0xFF;
    if (pidByte < 0x10) response += "0";
    response += String(pidByte, HEX);
    response += " ";
    
    // Add data bytes
    for (int i = 0; i < length; i++) {
        if (spacesEnabled && i > 0) response += " ";
        if (data[i] < 0x10) response += "0";
        response += String(data[i], HEX);
    }
    
    response.toUpperCase();
    return response;
}

void OBD2Handler::updateVehicleSimulation() {
    if (simulationMode == SimulationMode::STATIC) {
        return; // No updates in static mode
    }
    
    unsigned long currentTime = millis();
    
    if (simulationMode == SimulationMode::REALISTIC) {
        // Add some realistic variation
        float timeFactor = (currentTime % 10000) / 10000.0;
        
        // Engine RPM varies slightly
        if (vehicleState.engineRunning) {
            vehicleState.engineRPM = 800 + sin(timeFactor * 2 * PI) * 50;
        } else {
            vehicleState.engineRPM = 0;
        }
        
        // Engine load varies with RPM
        vehicleState.engineLoad = 15 + (vehicleState.engineRPM - 800) / 50.0 * 5;
        
        // Coolant temperature slowly rises to operating temperature
        if (vehicleState.coolantTemperature < 90) {
            vehicleState.coolantTemperature += 0.1;
        }
        
        // Battery voltage varies slightly
        vehicleState.batteryVoltage = 12.6 + sin(timeFactor * 4 * PI) * 0.1;
        
        // Fuel level slowly decreases
        if (vehicleState.fuelLevel > 0) {
            vehicleState.fuelLevel -= 0.001; // Very slow decrease
        }
    }
    
    vehicleState.lastUpdate = currentTime;
    vehicleState.updateCount++;
}

bool OBD2Handler::validatePIDRequest(uint16_t pid) {
    return supportedPIDs.find(pid) != supportedPIDs.end();
}

float OBD2Handler::calculatePIDValue(uint16_t pid) {
    switch (pid) {
        case StandardPIDs::ENGINE_RPM:
            return vehicleState.engineRPM;
        case StandardPIDs::VEHICLE_SPEED:
            return vehicleState.vehicleSpeed;
        case StandardPIDs::ENGINE_LOAD:
            return vehicleState.engineLoad;
        case StandardPIDs::THROTTLE_POSITION:
            return vehicleState.throttlePosition;
        case StandardPIDs::COOLANT_TEMPERATURE:
            return vehicleState.coolantTemperature;
        case StandardPIDs::INTAKE_AIR_TEMP:
            return vehicleState.intakeAirTemp;
        case StandardPIDs::FUEL_PRESSURE:
            return vehicleState.fuelPressure;
        case StandardPIDs::CONTROL_MODULE_VOLTAGE:
            return vehicleState.batteryVoltage;
        case StandardPIDs::FUEL_TANK_LEVEL:
            return vehicleState.fuelLevel;
        case StandardPIDs::AMBIENT_AIR_TEMP:
            return vehicleState.ambientTemperature;
        case StandardPIDs::RUNTIME_SINCE_START:
            return (millis() - vehicleState.lastUpdate) / 1000.0;
        default:
            return 0.0;
    }
}

void OBD2Handler::encodePIDData(uint16_t pid, float value, uint8_t* data) {
    switch (pid) {
        case StandardPIDs::ENGINE_RPM:
            {
                uint16_t rpm = (uint16_t)(value * 4); // RPM = ((A*256)+B)/4
                data[0] = (rpm >> 8) & 0xFF;
                data[1] = rpm & 0xFF;
            }
            break;
            
        case StandardPIDs::VEHICLE_SPEED:
            data[0] = (uint8_t)value; // km/h directly
            break;
            
        case StandardPIDs::ENGINE_LOAD:
        case StandardPIDs::THROTTLE_POSITION:
        case StandardPIDs::FUEL_TANK_LEVEL:
            data[0] = (uint8_t)(value * 2.55); // Percentage: A/2.55
            break;
            
        case StandardPIDs::COOLANT_TEMPERATURE:
        case StandardPIDs::INTAKE_AIR_TEMP:
        case StandardPIDs::AMBIENT_AIR_TEMP:
            data[0] = (uint8_t)(value + 40); // Temperature: A-40
            break;
            
        case StandardPIDs::FUEL_PRESSURE:
            data[0] = (uint8_t)(value / 3); // Fuel pressure: A*3
            break;
            
        case StandardPIDs::CONTROL_MODULE_VOLTAGE:
            {
                uint16_t voltage = (uint16_t)(value * 1000); // Voltage: ((A*256)+B)/1000
                data[0] = (voltage >> 8) & 0xFF;
                data[1] = voltage & 0xFF;
            }
            break;
            
        case StandardPIDs::RUNTIME_SINCE_START:
            {
                uint16_t runtime = (uint16_t)value; // Runtime: (A*256)+B seconds
                data[0] = (runtime >> 8) & 0xFF;
                data[1] = runtime & 0xFF;
            }
            break;
            
        default:
            data[0] = 0x00;
            break;
    }
}

// ===== PROTOCOL MANAGEMENT =====

bool OBD2Handler::setProtocol(OBD2Protocol protocol) {
    currentProtocol = protocol;
    protocolDescription = getProtocolDescription(protocol);
    
    Serial.print(F("[OBD2] Protocol set to: "));
    Serial.println(protocolDescription);
    
    return true;
}

OBD2Protocol OBD2Handler::getCurrentProtocol() const {
    return currentProtocol;
}

String OBD2Handler::getCurrentProtocolDescription() const {
    return protocolDescription;
}

OBD2Protocol OBD2Handler::autoDetectProtocol() {
    // Simulate protocol detection - prefer CAN for modern vehicles
    Serial.println(F("[OBD2] Auto-detecting protocol..."));
    delay(200); // Simulate detection time
    
    OBD2Protocol detectedProtocol = OBD2Protocol::ISO_15765_4_CAN;
    protocolDescription = getProtocolDescription(detectedProtocol);
    
    Serial.print(F("[OBD2] Detected protocol: "));
    Serial.println(protocolDescription);
    
    return detectedProtocol;
}

String OBD2Handler::getProtocolDescription(OBD2Protocol protocol) {
    switch (protocol) {
        case OBD2Protocol::AUTO_DETECT:
            return "AUTO";
        case OBD2Protocol::SAE_J1850_PWM:
            return "SAE J1850 PWM";
        case OBD2Protocol::SAE_J1850_VPW:
            return "SAE J1850 VPW";
        case OBD2Protocol::ISO_9141_2:
            return "ISO 9141-2";
        case OBD2Protocol::ISO_14230_4_KWP:
            return "ISO 14230-4 KWP2000";
        case OBD2Protocol::ISO_15765_4_CAN:
            return "ISO 15765-4 CAN (11-bit, 500kbps)";
        case OBD2Protocol::ISO_15765_4_CAN_B:
            return "ISO 15765-4 CAN (29-bit, 500kbps)";
        default:
            return "UNKNOWN";
    }
}

// ===== PID MANAGEMENT =====

void OBD2Handler::addSupportedPID(uint16_t pid, const String& description, 
                                  uint8_t dataBytes, const String& unit,
                                  float minVal, float maxVal) {
    PIDData pidData;
    pidData.pid = pid;
    pidData.description = description;
    pidData.dataBytes = dataBytes;
    pidData.unit = unit;
    pidData.minValue = minVal;
    pidData.maxValue = maxVal;
    pidData.supported = true;
    pidData.lastUpdate = 0;
    pidData.currentValue = 0.0;
    memset(pidData.rawData, 0, sizeof(pidData.rawData));
    
    supportedPIDs[pid] = pidData;
}

bool OBD2Handler::isPIDSupported(uint16_t pid) const {
    return supportedPIDs.find(pid) != supportedPIDs.end();
}

std::vector<uint16_t> OBD2Handler::getSupportedPIDs() const {
    std::vector<uint16_t> pids;
    for (const auto& pair : supportedPIDs) {
        pids.push_back(pair.first);
    }
    return pids;
}

PIDData OBD2Handler::getPIDInfo(uint16_t pid) const {
    auto it = supportedPIDs.find(pid);
    if (it != supportedPIDs.end()) {
        return it->second;
    }
    return PIDData{}; // Return empty struct if not found
}

// ===== VEHICLE DATA =====

void OBD2Handler::updateVehicleState(const VehicleState& state) {
    vehicleState = state;
    vehicleState.lastUpdate = millis();
    vehicleState.updateCount++;
}

VehicleState OBD2Handler::getVehicleState() const {
    return vehicleState;
}

void OBD2Handler::setVehicleParameter(uint16_t pid, float value) {
    switch (pid) {
        case StandardPIDs::ENGINE_RPM:
            vehicleState.engineRPM = value;
            break;
        case StandardPIDs::VEHICLE_SPEED:
            vehicleState.vehicleSpeed = value;
            break;
        case StandardPIDs::ENGINE_LOAD:
            vehicleState.engineLoad = value;
            break;
        case StandardPIDs::THROTTLE_POSITION:
            vehicleState.throttlePosition = value;
            break;
        case StandardPIDs::COOLANT_TEMPERATURE:
            vehicleState.coolantTemperature = value;
            break;
        // Add more cases as needed
    }
    vehicleState.lastUpdate = millis();
}

float OBD2Handler::getVehicleParameter(uint16_t pid) const {
    return calculatePIDValue(pid);
}

// ===== CONFIGURATION =====

void OBD2Handler::setEchoEnabled(bool enable) {
    echoEnabled = enable;
}

void OBD2Handler::setHeadersEnabled(bool enable) {
    headersEnabled = enable;
}

void OBD2Handler::setLinefeedsEnabled(bool enable) {
    linefeedsEnabled = enable;
}

void OBD2Handler::setSpacesEnabled(bool enable) {
    spacesEnabled = enable;
}

void OBD2Handler::setDeviceInfo(const String& info) {
    deviceInfo = info;
}

ATCommandState OBD2Handler::getCommandState() const {
    return commandState;
}

// ===== STATISTICS =====

String OBD2Handler::getStatistics() const {
    String stats = "OBD2 Handler Statistics:\n";
    stats += "Commands processed: " + String(commandsProcessed) + "\n";
    stats += "PID queries handled: " + String(pidQueriesHandled) + "\n";
    stats += "Error count: " + String(errorCount) + "\n";
    stats += "Average response time: " + String(getAverageResponseTime(), 2) + " ms\n";
    stats += "Current protocol: " + protocolDescription + "\n";
    stats += "Supported PIDs: " + String(supportedPIDs.size()) + "\n";
    stats += "Vehicle updates: " + String(vehicleState.updateCount) + "\n";
    return stats;
}

void OBD2Handler::resetStatistics() {
    commandsProcessed = 0;
    pidQueriesHandled = 0;
    errorCount = 0;
    totalProcessingTime = 0;
}

float OBD2Handler::getAverageResponseTime() const {
    if (commandsProcessed == 0) return 0.0;
    return (float)totalProcessingTime / commandsProcessed;
}

void OBD2Handler::printDiagnostics() const {
    Serial.println(F("=== OBD2 Handler Diagnostics ==="));
    Serial.print(F("Protocol: ")); Serial.println(protocolDescription);
    Serial.print(F("Command state: "));
    switch (commandState) {
        case ATCommandState::WAITING_RESET:
            Serial.println(F("WAITING_RESET"));
            break;
        case ATCommandState::ECHO_CONFIG:
            Serial.println(F("ECHO_CONFIG"));
            break;
        case ATCommandState::PROTOCOL_SELECT:
            Serial.println(F("PROTOCOL_SELECT"));
            break;
        case ATCommandState::HEADERS_CONFIG:
            Serial.println(F("HEADERS_CONFIG"));
            break;
        case ATCommandState::READY:
            Serial.println(F("READY"));
            break;
        case ATCommandState::ERROR_STATE:
            Serial.println(F("ERROR_STATE"));
            break;
    }
    
    Serial.print(F("Configuration: Echo=")); Serial.print(echoEnabled ? "ON" : "OFF");
    Serial.print(F(", Headers=")); Serial.print(headersEnabled ? "ON" : "OFF");
    Serial.print(F(", Linefeeds=")); Serial.print(linefeedsEnabled ? "ON" : "OFF");
    Serial.print(F(", Spaces=")); Serial.println(spacesEnabled ? "ON" : "OFF");
    
    Serial.println(getStatistics());
    Serial.println(F("================================"));
}

// ===== UTILITY FUNCTIONS =====

int OBD2Handler::hexStringToBytes(const String& hexString, uint8_t* bytes, int maxBytes) {
    int byteCount = 0;
    String cleanHex = hexString;
    cleanHex.replace(" ", "");
    
    for (int i = 0; i < cleanHex.length() && byteCount < maxBytes; i += 2) {
        if (i + 1 < cleanHex.length()) {
            String byteString = cleanHex.substring(i, i + 2);
            bytes[byteCount] = strtol(byteString.c_str(), NULL, 16);
            byteCount++;
        }
    }
    
    return byteCount;
}

String OBD2Handler::bytesToHexString(const uint8_t* bytes, int length, bool spaces) {
    String hexString = "";
    for (int i = 0; i < length; i++) {
        if (i > 0 && spaces) hexString += " ";
        if (bytes[i] < 0x10) hexString += "0";
        hexString += String(bytes[i], HEX);
    }
    hexString.toUpperCase();
    return hexString;
}

uint8_t OBD2Handler::calculateChecksum(const uint8_t* data, int length) {
    uint8_t checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum += data[i];
    }
    return (0x100 - checksum) & 0xFF;
}

uint16_t OBD2Handler::parsePIDFromCommand(const String& command) {
    if (command.length() < 4) return 0;
    
    String modeStr = command.substring(0, 2);
    String pidStr = command.substring(2, 4);
    
    int mode = strtol(modeStr.c_str(), NULL, 16);
    int pid = strtol(pidStr.c_str(), NULL, 16);
    
    return (mode << 8) | pid;
}

String OBD2Handler::getPIDName(uint16_t pid) {
    switch (pid) {
        case StandardPIDs::ENGINE_RPM:
            return "Engine RPM";
        case StandardPIDs::VEHICLE_SPEED:
            return "Vehicle Speed";
        case StandardPIDs::ENGINE_LOAD:
            return "Engine Load";
        case StandardPIDs::THROTTLE_POSITION:
            return "Throttle Position";
        case StandardPIDs::COOLANT_TEMPERATURE:
            return "Coolant Temperature";
        case StandardPIDs::INTAKE_AIR_TEMP:
            return "Intake Air Temperature";
        case StandardPIDs::FUEL_PRESSURE:
            return "Fuel Pressure";
        case StandardPIDs::CONTROL_MODULE_VOLTAGE:
            return "Control Module Voltage";
        case StandardPIDs::FUEL_TANK_LEVEL:
            return "Fuel Tank Level";
        case StandardPIDs::AMBIENT_AIR_TEMP:
            return "Ambient Air Temperature";
        case StandardPIDs::RUNTIME_SINCE_START:
            return "Runtime Since Start";
        default:
            return "Unknown PID";
    }
}

ATResponse OBD2Handler::processATCommandDetailed(const String& command) {
    ATResponse response;
    response.command = command;
    
    unsigned long startTime = millis();
    response.response = processATCommand(command);
    response.processingTime = millis() - startTime;
    response.success = !response.response.equals("?");
    
    return response;
}