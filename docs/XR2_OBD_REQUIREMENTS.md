# XR-2 OBD Data Requirements - From Firmware Analysis

## Core OBD Functions XR-2 Calls

Based on firmware reverse engineering, XR-2 uses these specific functions to retrieve OBD data:

### **High Priority (Core Dashboard Data):**
```cpp
GetOBDRpm()             // Engine RPM → PID 010C
GetOBDSpeed()           // Vehicle Speed → PID 010D  
GetOBDEngCoolDeg()      // Engine Coolant Temperature → PID 0105
GetOBDThrottle()        // Throttle Position → PID 0111
GetOBDControlVoltage()  // Control Module Voltage → PID 0142
```

### **Medium Priority (Extended Dashboard):**
```cpp
GetOBDInletPressure()   // Intake Manifold Pressure → PID 010B
GetOBDIntakeTemp()      // Intake Air Temperature → PID 010F
GetOBDFiringAngle()     // Timing Advance → PID 010E
GetOBDAtmos()           // Barometric Pressure → PID 0133
GetOBDEngineStarttime() // Time Since Engine Start → PID 011F
```

### **Lower Priority (Diagnostics & Maintenance):**
```cpp
GetOBDEngineOilRate()   // Engine Oil Pressure/Rate → PID 015E
GetOBDThrottleValve()   // Throttle Valve Position → PID 0111 (alternate)
GetOBDClearedMile()     // Distance Since Codes Cleared → PID 0131
GetOBDCatTempVoiler1Sen1() // Catalyst Temperature → PID 013C
GetOBDMotorTemp()       // Motor Temperature → PID 0105 (alternate)
```

## PID to Function Mapping

| Firmware Function | OBD PID | Response Format | Description |
|-------------------|---------|-----------------|-------------|
| `GetOBDRpm()` | 010C | 41 0C XX XX | RPM = ((XX*256) + XX) / 4 |
| `GetOBDSpeed()` | 010D | 41 0D XX | Speed = XX km/h |
| `GetOBDEngCoolDeg()` | 0105 | 41 05 XX | Temp = XX - 40°C |
| `GetOBDThrottle()` | 0111 | 41 11 XX | Throttle = XX * 100/255 % |
| `GetOBDControlVoltage()` | 0142 | 41 42 XX XX | Voltage = ((XX*256) + XX) / 1000 V |
| `GetOBDInletPressure()` | 010B | 41 0B XX | Pressure = XX kPa |
| `GetOBDIntakeTemp()` | 010F | 41 0F XX | Temp = XX - 40°C |
| `GetOBDFiringAngle()` | 010E | 41 0E XX | Advance = (XX - 128) / 2 degrees |
| `GetOBDAtmos()` | 0133 | 41 33 XX | Pressure = XX kPa |
| `GetOBDEngineStarttime()` | 011F | 41 1F XX XX | Time = (XX*256) + XX seconds |

## XR-2 Data Reading Pattern

From firmware analysis, XR-2 likely:

### **1. Connection Sequence:**
```cpp
BLE_STATE_SEARCH_DEV    // Scan for "CGOBD" devices
BLE_STATE_CONN_DEV      // Connect to device
BLE_STATE_REG_SERVER    // Register as server
BLE_STATE_RUN           // Start data acquisition
```

### **2. Data Acquisition Method:**
```cpp
// Option A: Polling Mode (most likely)
IPC_IsOBDUsable()       // Check if device ready
GetOBDRpm()             // Request RPM
GetOBDSpeed()           // Request Speed
GetOBDEngCoolDeg()      // Request Temperature
// ... continue for all parameters

// Option B: Streaming Mode
// Continuous data stream with all parameters
```

### **3. Data Validation:**
```cpp
IPC_OBDValidBit8()      // Validate 8-bit values
IPC_OBDValidBit16()     // Validate 16-bit values  
IPC_OBDValidBit32()     // Validate 32-bit values
```

## Recommended ESP32 Implementation

### **Priority 1: Core Dashboard Data**
Ensure these PIDs are always available and respond quickly:
- `010C` (RPM)
- `010D` (Speed) 
- `0105` (Coolant Temp)
- `0111` (Throttle)
- `0142` (Voltage)

### **Priority 2: Send Supported PIDs**
Return comprehensive supported PIDs mask:
```cpp
0100 → 41 00 FE 3F F8 11  // Support PIDs 05,0B,0C,0D,0E,0F,11,1F,31,33,3C,42,5E
```

### **Priority 3: Data Update Frequency**
Based on firmware patterns:
- **Core Data**: Update every 100-200ms
- **Extended Data**: Update every 500-1000ms
- **Diagnostic Data**: Update every 2-5 seconds

### **Priority 4: Response Timing**
- **Command Response**: < 50ms
- **Data Validation**: Use proper bit validation
- **Connection Stability**: Implement auto-reconnect

## Status
✅ **Firmware Analysis Complete**
🔄 **ESP32 Implementation Updated** 
⏳ **Testing XR-2 Display Response**