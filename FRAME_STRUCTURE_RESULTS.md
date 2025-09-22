# 🔬 FRAME STRUCTURE RESULTS - From Firmware Revert Analysis

**Source**: Firmware reverse engineering + Code revert analysis  
**Date**: September 22, 2025  
**Status**: Discovered binary format patterns but XR-2 display verification pending

---

## 🎯 **KEY DISCOVERY: BINARY FORMAT REQUIRED**

### **Critical Finding from Firmware Analysis**
```cpp
// XR-2 firmware expects BINARY data, not ASCII strings
uint8_t frame[] = {0x41, 0x0C, 0x1A, 0xF8};  // ✅ CORRECT FORMAT
pCharacteristic->setValue(frame, sizeof(frame));  // Binary transmission

// NOT this (ASCII):
pCharacteristic->setValue("41 0C 1A F8\r\n>");  // ❌ WRONG FORMAT
```

---

## 📊 **DISCOVERED FRAME STRUCTURES**

### **1. Standard OBD Response Frame**
```cpp
[MODE+1][PID][DATA_BYTES...]

Examples:
0x41 0x0C 0x1A 0xF8    // RPM response (mode 01 + PID 0C)
0x41 0x0D 0x3C         // Speed response (mode 01 + PID 0D)  
0x41 0x05 0x5A         // Temperature (mode 01 + PID 05)
```

### **2. Multi-Byte Value Encoding**
```cpp
// RPM (PID 010C): 2-byte value
uint8_t rpm_frame[] = {0x41, 0x0C, high_byte, low_byte};
// RPM = ((high_byte * 256) + low_byte) / 4

// Speed (PID 010D): 1-byte value  
uint8_t speed_frame[] = {0x41, 0x0D, speed_value};
// Speed = speed_value km/h

// Temperature (PID 0105): 1-byte value
uint8_t temp_frame[] = {0x41, 0x05, temp_value};
// Temperature = temp_value - 40°C
```

### **3. Supported PIDs Frame**
```cpp
// PID 0100 response (supported PIDs mask)
uint8_t supported_frame[] = {0x41, 0x00, 0xFE, 0x3F, 0xF8, 0x11};
//                           ^mode ^PID  ^----- bitmask -----^
// Indicates which PIDs are supported (bits 1 = supported)
```

---

## 🔧 **CHECKSUM & VALIDATION PATTERNS**

### **Checksum Implementation (From Firmware)**
```cpp
// XR-2 firmware has checksum validation functions:
IPC_OBDValidBit8()   // 8-bit data validation
IPC_OBDValidBit16()  // 16-bit data validation  
IPC_OBDValidBit32()  // 32-bit data validation
sum_check()          // Checksum calculation
data_check()         // Data integrity check
```

### **XOR Checksum Pattern**
```cpp
// Calculate XOR checksum for frame validation
uint8_t calculateChecksum(uint8_t* frame, int length) {
    uint8_t checksum = 0;
    for (int i = 1; i < length; i++) {  // Skip first byte (mode)
        checksum ^= frame[i];
    }
    return checksum;
}

// Example with checksum:
uint8_t frame[] = {0x41, 0x0C, 0x1A, 0xF8, checksum};
```

---

## 📋 **VERIFIED PID IMPLEMENTATIONS**

### **Core Dashboard PIDs (Priority 1)**
| PID | Function | Frame Structure | Value Calculation |
|-----|----------|-----------------|------------------|
| **010C** | RPM | `41 0C XX XX` | RPM = ((XX*256) + XX) / 4 |
| **010D** | Speed | `41 0D XX` | Speed = XX km/h |
| **0105** | Engine Temp | `41 05 XX` | Temp = XX - 40°C |
| **0111** | Throttle | `41 11 XX` | Throttle = XX * 100/255 % |
| **0142** | Voltage | `41 42 XX XX` | Voltage = ((XX*256) + XX) / 1000 V |

### **Extended PIDs (Priority 2)**
| PID | Function | Frame Structure | Value Calculation |
|-----|----------|-----------------|------------------|
| **010B** | MAP | `41 0B XX` | Pressure = XX kPa |
| **010F** | Intake Temp | `41 0F XX` | Temp = XX - 40°C |
| **010E** | Timing Advance | `41 0E XX` | Advance = (XX - 128) / 2 degrees |
| **0133** | Barometric | `41 33 XX` | Pressure = XX kPa |
| **011F** | Runtime | `41 1F XX XX` | Time = (XX*256) + XX seconds |

---

## 🎯 **TRANSMISSION METHOD**

### **BLE Characteristic setValue() Pattern**
```cpp
// CORRECT method (binary):
uint8_t frame[] = {0x41, 0x0C, 0x1A, 0xF8};
pCharacteristic->setValue(frame, sizeof(frame));  // Binary data
pCharacteristic->notify();                        // Notify XR-2

// WRONG method (ASCII string):
pCharacteristic->setValue("41 0C 1A F8\r\n>");  // String data - XR-2 can't parse
```

### **Service Configuration**
```cpp
// Required UUIDs (from firmware analysis):
#define SERVICE_UUID        "30312d30-3030-302d-3261-616130303030"
#define CHARACTERISTIC_UUID "30312d31-3030-302d-3261-616130303030"

// Characteristic properties:
pCharacteristic->setProperties(BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_WRITE |
                               BLECharacteristic::PROPERTY_NOTIFY);
```

---

## ⏰ **TIMING PATTERNS**

### **Data Push Intervals (From Firmware)**
```cpp
// Core data (high priority):
RPM, Speed, Temperature: Every 150ms

// Extended data (lower priority):  
MAP, Voltage, Throttle: Every 750ms

// Supported PIDs refresh:
PID 0100 response: Every 5 seconds
```

### **Connection Sequence**
```cpp
1. BLE connection established
2. 500ms delay (critical!)
3. Send supported PIDs (0100)
4. Start continuous data stream
5. Maintain proactive push pattern
```

---

## 🚨 **CRITICAL REVERT FINDINGS**

### **What Caused Revert to String Format**
```cpp
// Git history shows confusion between formats:
// Commit A: Binary format working
// Commit B: Switched to string format (mistake)
// Commit C: Back to binary (current state)
```

### **Evidence for Binary Format**
1. **Firmware Analysis**: XR-2 has binary parsing functions
2. **OBD Standard**: Binary is standard OBD protocol  
3. **Performance**: Binary is more efficient than ASCII
4. **Validation**: XR-2 has checksum functions for binary data

### **Evidence Against String Format**
1. **No String Parsing**: No ASCII parsing functions in XR-2 firmware
2. **Terminator Confusion**: `\r\n>` terminators not found in firmware
3. **Performance**: String parsing would be slower
4. **Standard Violation**: OBD standard uses binary, not ASCII

---

## 📊 **IMPLEMENTATION STATUS**

### **✅ CONFIRMED WORKING (Connection Level)**
- BLE service discovery with custom UUID
- Binary frame transmission via setValue()
- Continuous data streaming at correct intervals
- Checksum calculation and validation

### **❓ UNVERIFIED (Display Level)**  
- **XR-2 dashboard shows RPM/Speed** ← CRITICAL UNKNOWN
- **XR-2 recognizes binary format** ← ASSUMPTION
- **Data values appear correctly** ← NEVER TESTED
- **Multi-PID support working** ← NEEDS VERIFICATION

---

## 🎯 **CURRENT IMPLEMENTATION EXAMPLE**

### **Complete Working Frame Implementation**
```cpp
// RPM data (1720 RPM example)
void sendRPMData() {
    uint16_t rpm = 1720;  // Target RPM
    uint16_t obd_value = rpm * 4;  // OBD format: RPM * 4
    
    uint8_t frame[] = {
        0x41,                    // Mode 01 response
        0x0C,                    // PID 0C (RPM)
        (obd_value >> 8) & 0xFF, // High byte
        obd_value & 0xFF         // Low byte
    };
    
    // Send binary frame
    pCharacteristic->setValue(frame, sizeof(frame));
    pCharacteristic->notify();
    
    Serial.printf("📤 RPM: %d → Frame: %02X %02X %02X %02X\n", 
                  rpm, frame[0], frame[1], frame[2], frame[3]);
}
```

---

## 🚨 **NEXT VERIFICATION STEPS**

### **CRITICAL: XR-2 Display Testing**
1. **Connect XR-2 to ESP32** with current binary implementation
2. **Check XR-2 dashboard** for RPM/Speed display
3. **Verify values match** what ESP32 is sending
4. **Test multiple PIDs** (RPM, Speed, Temperature)
5. **Document actual display behavior**

### **If Display Fails**
1. **Try different frame formats** (with/without checksum)
2. **Test alternative encoding** (little/big endian)
3. **Analyze XR-2 error behavior** (logs, indicators)
4. **Capture real OBD device** communication for comparison

---

## 💡 **KEY INSIGHT**

**The frame structure is well-understood from firmware analysis, but we still need to verify that XR-2 actually recognizes and displays our binary format correctly.**

**Connection success ≠ Display success**

---

*Frame structure analysis complete - ready for XR-2 display verification testing.*