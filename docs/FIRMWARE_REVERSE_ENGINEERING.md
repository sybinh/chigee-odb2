# Chigee XR-2 Firmware Reverse Engineering Results

## Executive Summary
Comprehensive analysis of Chigee XR-2 firmware reveals complete OBD implementation with BLE connectivity protocols. Successfully extracted key functions, states, and configuration patterns.

## Firmware Structure
- **rootfs.ext4**: Main Linux filesystem (37MB) - contains OBD implementation
- **customer.ext4**: Configuration partition - contains OBD controller settings  
- **miservice.ext4**: Service partition - minimal OBD references

## Key Discoveries

### 1. OBD Function Library (from rootfs.ext4)
```cpp
// Core OBD Functions
IPC_IsOBDPair()         // Check if OBD device is paired
IPC_IsOBDUsable()       // Check if OBD device is usable
IPC_OBDValidBit8/16/32() // Data validation functions

// OBD Data Retrieval Functions  
GetOBDRpm()             // Engine RPM
GetOBDSpeed()           // Vehicle speed
GetOBDThrottle()        // Throttle position
GetOBDThrottleValve()   // Throttle valve position
GetOBDInletPressure()   // Inlet pressure
GetOBDFiringAngle()     // Firing angle
GetOBDEngineOilRate()   // Engine oil rate
GetOBDControlVoltage()  // Control voltage
GetOBDEngineStarttime() // Engine start time
GetOBDIntakeTemp()      // Intake temperature
GetOBDAtmos()           // Atmospheric pressure
GetOBDClearedMile()     // Cleared mileage
GetOBDCatTempVoiler1Sen1() // Catalyst temperature
GetOBDMotorTemp()       // Motor temperature
GetOBDEngCoolDeg()      // Engine coolant temperature
```

### 2. OBD Mode Configuration
- **Mode Flag File**: `/mnt/par/OBD_MODE_FLAG`
- **Configuration**: `Camera.Menu.OBDController`
- **Address Setting**: `Camera.Menu.OBDControllerAddr`
- **Enable/Disable**: `"ENABLE"/"DISABLE"` states
- **Function**: `setOBDContoller()` with logging `"setOBDContoller:"`

### 3. BLE Implementation Details

#### BLE Connection States
```cpp
BLE_STATE_NOT_CONN      // Not connected
BLE_STATE_REG_SERVER    // Register server
BLE_STATE_SEARCH_DEV    // Searching for devices
BLE_STATE_CONN_DEV      // Connecting to device
BLE_STATE_RUN           // Running/connected
BLE_STATE_DIS_CONN      // Disconnecting
BLE_STATE_DIS_CONN_STOP // Disconnect stopped
```

#### OBD Commands
```cpp
CMD_UTIL_GOC_OBD_SEARCH         // Search for OBD devices
CMD_UTIL_GOC_OBD_STOP_SEARCH    // Stop search
CMD_UTIL_GOC_OBD_DIS_PAIR       // Disconnect/unpair
CMD_UTIL_GOC_OBD_CONN           // Connect to device
CMD_UTIL_GOC_OBD_OPEN_AUTOCONN  // Enable auto-connect
CMD_UTIL_GOC_OBD_CLOSE_AUTOCONN // Disable auto-connect
```

#### Device Discovery
- **Target Device Name**: `CGOBD` (matches our ESP32 implementation!)
- **Search Pattern**: `BLE_STATE_SEARCH_DEV:[%s]`
- **Connection Logging**: `BLE_STATE_DIS_CONN:gattid:[%s],addr:[%s],connid[%s]`

### 4. UUID and Service Structure

#### UUID Format Pattern
```cpp
UUID=%.8s-%.4s-%.4s-%.4s-%.12s  // Standard UUID format

// Service Logging Patterns
serviceuuid:%s.         // Service UUID logging
serviceUuid:%s.         // Service UUID parameter
writeUuid:%s.           // Write characteristic UUID
readUuid:%s.            // Read characteristic UUID
para.uuid:[%s]          // Parameter UUID logging
g_gocsdk_conn_para.uuid:[%s] // Global connection parameter UUID
```

#### BLE Callback Functions
```cpp
ble_reg_server_callback()  // BLE server registration callback
ble_cmd_parse()           // BLE command parsing
uuid[%s],                 // UUID logging format
```

### 5. Data Parsing Implementation

#### OBD Data Processing
```cpp
obd_data_parse()           // Main OBD data parsing function
obd_conn_logic_process()   // OBD connection logic processing  
parseOBDData()            // Generic OBD data parser
parseOBDData_version1()   // Version 1 specific parser
poseParsePid()            // PID parsing function
g_obd_para                // Global OBD parameters
```

#### BLE Characteristics
```cpp
transf_characteristics:    // Transfer characteristics
characteristics           // Generic characteristics handling
```

## Implementation Insights

### 1. XR-2 Expects Specific Device Name
- XR-2 searches for devices named `CGOBD`
- Our ESP32 implementation already uses this name ✅
- This confirms our naming strategy is correct

### 2. Connection Flow
1. `BLE_STATE_SEARCH_DEV` - XR-2 searches for `CGOBD` devices
2. `BLE_STATE_CONN_DEV` - Attempts connection
3. `BLE_STATE_REG_SERVER` - Registers as BLE server
4. `BLE_STATE_RUN` - Establishes data connection
5. Continuous OBD data retrieval using `GetOBDxxx()` functions

### 3. Data Format Requirements
- XR-2 has specific parsing functions for different OBD versions
- Uses dedicated validation functions (`IPC_OBDValidBit8/16/32`)
- Implements comprehensive error checking and state management

### 4. Service Architecture
- Multiple UUID patterns for different services
- Separate read/write characteristics
- Connection parameter management through global structures

## Next Steps for ESP32 Optimization

### 1. Enhanced Service Structure
```cpp
// Based on firmware analysis, implement:
- Service UUID logging for debugging
- Connection state management
- Proper characteristic configuration
- UUID parameter validation
```

### 2. Data Format Alignment
```cpp
// Align with XR-2 expectations:
- Implement version-specific parsing compatibility
- Add data validation similar to IPC_OBDValidBit functions
- Use proper OBD PID response formatting
```

### 3. Connection Management
```cpp
// Improve connection stability:
- Implement auto-reconnection logic
- Add connection state monitoring
- Handle disconnect/reconnect scenarios
```

### 4. Protocol Verification
- Monitor actual UUID exchanges during connection
- Verify service/characteristic structure matches expectations
- Test with different OBD PIDs to ensure compatibility

## Conclusion
Firmware analysis reveals XR-2 uses comprehensive OBD implementation with specific BLE service requirements. Our current ESP32 implementation aligns well with discovered patterns, particularly the `CGOBD` device naming. Key optimization opportunities exist in service structure enhancement and data format standardization.

**Status**: ✅ Major protocol insights discovered - ready for ESP32 implementation refinement