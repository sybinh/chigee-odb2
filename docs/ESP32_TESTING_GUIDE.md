# ESP32-WROOM-32 Hardware Testing Guide

## 🚀 Quick Start for Real Hardware Testing

### Prerequisites
- ESP32-WROOM-32 development board
- USB cable (Type-A to Micro-USB)
- PlatformIO IDE or Arduino IDE
- Chigee XR2 device for pairing test

### 📋 Hardware Validation Checklist

#### 1. Board Connections
```
ESP32-WROOM-32 Pin Layout Verification:
- Pin 2:  Built-in LED (should be present)
- Pin 4:  CAN Activity LED (external, optional)
- Pin 16: Bluetooth Status LED (external, optional) 
- Pin 17: Error LED (external, optional)
- Pin 18: SPI SCK (for future CAN controller)
- Pin 19: SPI MISO (for future CAN controller)
- Pin 23: SPI MOSI (for future CAN controller)
- Pin 5:  CAN CS (for future CAN controller)
- Pin 21: CAN INT (for future CAN controller)
```

#### 2. Power Requirements
- USB power: 5V input, 3.3V regulated
- Current consumption: ~150mA active, ~80mA idle
- Ensure stable USB connection

### 🔧 Testing Procedure

#### Step 1: Hardware Validation
```bash
# Build and upload test firmware
pio run -e esp32_test --target upload

# Monitor serial output
pio device monitor -e esp32_test
```

Expected output:
```
🔧 Starting ESP32-WROOM-32 Hardware Validation 🔧
[PASS] ESP32 Chip Detection: OK
[PASS] Flash Size (4MB): OK  
[PASS] Free Heap (>200KB): OK
[PASS] GPIO2 Function: OK
[PASS] GPIO4 Function: OK
...
✅ ESP32-WROOM-32 hardware validation successful!
```

#### Step 2: Bluetooth Pairing Test
```
📱 Initializing Bluetooth for pairing test...
🔍 Bluetooth device discoverable as:
    Name: ChigeeOBD2_ESP32
    PIN: 1234
📱 Please pair your Chigee device now...
```

**Pairing Steps:**
1. On Chigee XR2: Settings → Bluetooth → Search devices
2. Look for "ChigeeOBD2_ESP32" in available devices
3. Select and enter PIN: 1234
4. Wait for connection confirmation

Expected result:
```
✅ Bluetooth connected! Ready for OBD2 commands
```

#### Step 3: OBD2 Data Simulation
```
🚗 Starting OBD2 Simulation Mode 🚗
📊 === Vehicle Telemetry ===
Engine: ON | Speed: 0 km/h | RPM: 798
Load: 12% | Throttle: 0% | Temp: 23°C
Fuel: 75% | Rate: 0.80 L/h | Battery: 14.4V
MIL: OFF | DTCs: 0 | Runtime: 45 sec
```

### 🎮 Interactive Testing Commands

#### Serial Console Commands
```bash
# Mode switching
validate    # Re-run hardware validation
bt          # Switch to Bluetooth pairing mode  
obd2        # Switch to OBD2 simulation
demo        # Continuous demo mode

# Vehicle simulation
start       # Start engine simulation
stop        # Stop engine
city        # City driving mode (30-50 km/h)
highway     # Highway driving (90-120 km/h)
idle        # Idle mode (0 km/h, ~800 RPM)
accel       # Acceleration burst
brake       # Deceleration/braking
fault       # Simulate DTC P0171
clear       # Clear diagnostic codes

# System info
status      # Show system status
help        # Show command menu
```

### 📱 Chigee XR2 Testing

#### OBD2 Commands to Test
```
ATZ         # Reset (should respond: ELM327 v2.1)
ATE0        # Echo off (should respond: OK)
010C        # Engine RPM (should return current RPM)
010D        # Vehicle speed (should return current speed)
0104        # Engine load (should return load percentage)
0105        # Coolant temperature
0111        # Throttle position
012F        # Fuel level
0142        # Battery voltage
0101        # Monitor status (MIL and DTCs)
```

#### Expected Chigee Behavior
1. **Connection**: Chigee should show "OBD2 Connected" status
2. **Live Data**: Real-time RPM, speed, temperature updates
3. **Gauges**: All gauges should display simulated values
4. **DTC Display**: When fault is simulated, should show P0171

### 🐛 Troubleshooting

#### Hardware Issues
```
❌ ESP32 Chip Detection failed
→ Check board type, ensure it's ESP32-WROOM-32
→ Try different USB port/cable

❌ GPIO Function tests failed  
→ Check for loose connections
→ Verify pin numbers match board layout

❌ Flash Size incorrect
→ Board may be different variant
→ Check board markings for actual specifications
```

#### Bluetooth Issues
```
❌ Bluetooth initialization failed
→ Reset ESP32 and try again
→ Check if other Bluetooth devices are interfering

❌ Pairing fails on Chigee
→ Ensure PIN is "1234" 
→ Clear Bluetooth cache on Chigee
→ Try restarting both devices
```

#### OBD2 Simulation Issues
```
❌ No response to OBD2 commands
→ Check Bluetooth connection is stable
→ Verify command format (uppercase hex)
→ Monitor serial output for received commands

❌ Invalid data on Chigee display
→ Check data format in dummy_data_generator.cpp
→ Verify PID response calculations
→ Test with known working OBD2 scanner
```

### 📊 Success Criteria

#### ✅ Full Validation Success
- All hardware tests pass
- Bluetooth pairs successfully with Chigee XR2
- OBD2 commands receive valid responses  
- Live data updates on Chigee display
- Vehicle simulation modes work correctly
- Fault simulation triggers MIL on Chigee

#### 📈 Performance Targets
- Connection time: < 10 seconds
- OBD2 response time: < 100ms
- Data update rate: 1-2 Hz
- Memory usage: < 70% of available heap
- No crashes during 30+ minute test

### 🔄 Continuous Testing

#### Demo Mode Features
```bash
# Type 'demo' to start continuous simulation:
🏁 Demo: Starting engine
🏙️ Demo: City driving  
🚀 Demo: Acceleration
🛣️ Demo: Highway driving
🛑 Demo: Braking
⚠️ Demo: Fault simulation  
✅ Demo: Clearing faults
🛑 Demo: Stopping engine
```

This creates a realistic driving cycle for extended testing.

### 📝 Test Results Log

#### Hardware Validation Results
```
Date: ___________
Board: ESP32-WROOM-32
Flash: _______ MB
Free Heap: _______ KB
GPIO Tests: PASS/FAIL
ADC Tests: PASS/FAIL
SPI Tests: PASS/FAIL
Bluetooth: PASS/FAIL
Overall: PASS/FAIL
```

#### Chigee Pairing Results  
```
Pairing Time: _______ seconds
Connection Stable: YES/NO
OBD2 Response Time: _______ ms
Data Display: CORRECT/INCORRECT
Fault Detection: WORKING/NOT WORKING
```

#### Notes
```
_________________________________
_________________________________
_________________________________
```

---

## 🎯 Next Steps After Successful Testing

1. **Production Build**: Switch to `esp32_production` environment
2. **CAN Hardware**: Add MCP2515 CAN controller for real OBD2
3. **Enclosure**: Design 3D printed case for ESP32 module
4. **Documentation**: Update wiring diagrams with validated pin assignments