# Hardware Documentation

This document provides detailed hardware information for the Chigee XR2 OBD2 Module project.

## ?? Component Specifications

### ESP32 Development Board
- **Model**: ESP32-DevKitC-32E (recommended) or ESP32-WROOM-32D
- **Flash**: Minimum 4MB
- **RAM**: 520KB SRAM
- **Bluetooth**: Classic + BLE support required
- **GPIO**: Minimum 6 pins needed for this project
- **Power**: 3.3V logic, 5V tolerant on some pins

### MCP2515 CAN Bus Controller
- **Interface**: SPI communication with ESP32
- **Speed**: Supports up to 1Mbps (using 500kbps for motorcycles)
- **Crystal**: 16MHz (8MHz variants require code changes)
- **Power**: 3.3V or 5V operation
- **Termination**: 120? resistor required

### Power Supply Requirements
- **Input**: 12V from motorcycle battery
- **Output**: 5V/3.3V for ESP32 and MCP2515
- **Current**: ~200mA peak, ~100mA typical
- **Protection**: Reverse polarity, overvoltage
- **Regulation**: LM2596 or similar switching regulator

## ?? Pin Connections

### ESP32 to MCP2515 Wiring
```
ESP32 Pin    MCP2515 Pin    Function
---------    -----------    --------
3V3          VCC            Power (3.3V)
GND          GND            Ground
GPIO5        CS             Chip Select
GPIO23       MOSI           SPI Data Out
GPIO19       MISO           SPI Data In  
GPIO18       SCK            SPI Clock
GPIO2        INT            Interrupt (optional)
```

### MCP2515 to Vehicle CAN Bus
```
MCP2515 Pin    Vehicle Connector    Function
-----------    -----------------    --------
CANH           CAN High             CAN Bus High Signal
CANL           CAN Low              CAN Bus Low Signal
GND            Chassis Ground       Common Ground
```

### Status and Control Pins
```
ESP32 Pin    Component           Function
---------    ---------           --------
GPIO2        Status LED          Visual feedback
GPIO4        Emergency Button    Safety disconnect (pull-up)
GPIO0        Boot Button         Programming (built-in)
EN           Reset Button        System reset (built-in)
```

## 🔌 Vehicle Connection

### Husqvarna Svartpilen 401 / KTM Duke 390
- **Diagnostic Connector**: 6-pin connector under seat
- **CAN Bus Speed**: 500 kbps
- **Voltage**: 12V nominal (11-15V range)
- **Protocol**: ISO 11898-2 (High-speed CAN)

### Diagnostic Connector Pinout
```
Pin    Function        Wire Color    Notes
---    --------        ----------    -----
1      CAN High        Yellow        CAN bus positive
2      CAN Low         Green         CAN bus negative  
3      Ground          Black         Chassis ground
4      +12V            Red           Battery voltage
5      N/C             -             Not connected
6      N/C             -             Not connected
```

?? **WARNING**: Always verify pinout with vehicle service manual before connecting!

## 🔧 Assembly Instructions

### 1. PCB Layout (Prototype)
For breadboard/perfboard assembly:

```
[ESP32 DevKit]    [MCP2515 Module]    [Power Supply]
      |                   |                 |
      +------- SPI -------+                 |
      |                   |                 |
      +------- Power -----+-----------------+
      |                   |
      +-- Status LED      |
      +-- Emergency Btn   |
                          |
                    [CAN Connector]
                          |
                    [To Vehicle]
```

### 2. Component Placement
1. **ESP32**: Center of board for easy access to pins
2. **MCP2515**: Close to ESP32 to minimize SPI trace length
3. **Power Supply**: Separate section with heat dissipation
4. **Connectors**: Edge-mounted for easy access
5. **LEDs/Buttons**: Front panel for user interface

### 3. PCB Considerations
- **Trace Width**: Minimum 0.3mm for signal, 0.8mm for power
- **Ground Plane**: Continuous ground plane recommended
- **CAN Impedance**: 120? differential impedance for CAN traces
- **EMI Protection**: Ferrite beads on power lines
- **ESD Protection**: TVS diodes on CAN lines

## ?? Power System Design

### Input Protection
```
12V Vehicle ----[Fuse 2A]----[Reverse Polarity Protection]----[Voltage Regulator]
                                        |
                                   [TVS Diode]
                                        |
                                      GND
```

### Voltage Regulation
- **Primary**: 12V to 5V switching regulator (LM2596 or similar)
- **Secondary**: 5V to 3.3V linear regulator (AMS1117-3.3)
- **Backup**: USB power for development/testing

### Power Consumption
| Component | Typical | Peak | Notes |
|-----------|---------|------|-------|
| ESP32 | 80mA | 240mA | During WiFi/BT transmission |
| MCP2515 | 20mA | 30mA | Active CAN communication |
| LED | 10mA | 10mA | Status indicator |
| **Total** | **110mA** | **280mA** | Add 20% safety margin |

## ?? Antenna Considerations

### Bluetooth Range
- **Built-in Antenna**: 10-30 meter range typical
- **External Antenna**: Optional U.FL connector for better range
- **Placement**: Avoid metal enclosure interference
- **Testing**: Verify range with Chigee XR2 in typical mounting position

## 🛡️ Protection Circuits

### CAN Bus Protection
```
Vehicle CAN ----[120? Term]----[TVS Diodes]----[Common Mode Choke]----[MCP2515]
     |                              |                                       |
     +------[120? Term]------[TVS Diodes]----[Common Mode Choke]------------+
                                    |
                                   GND
```

### Power Protection
- **Fuse**: 2A fast-blow on 12V input
- **Reverse Polarity**: P-channel MOSFET or Schottky diode
- **Overvoltage**: 18V TVS diode on input
- **ESD**: TVS diodes on all external connections

## ?? Enclosure Design

### Requirements
- **Waterproof**: IP65 minimum for motorcycle use
- **Temperature**: -20�C to +85�C operating range
- **Vibration**: Motorcycle vibration resistant
- **Size**: Compact for under-seat mounting
- **Access**: USB port for programming/debugging

### Recommended Materials
- **ABS Plastic**: Good chemical resistance
- **Aluminum**: Excellent heat dissipation
- **Gaskets**: Silicone or EPDM rubber
- **Connectors**: Weatherpack or Deutsch automotive connectors

### Mounting Options
1. **Under Seat**: Protected from weather
2. **Tool Box**: Easy access for maintenance
3. **Frame Mount**: Secure but exposed
4. **Tank Bag**: Portable but temporary

## ?? Testing Equipment

### Essential Tools
- **Multimeter**: Voltage and continuity testing
- **Oscilloscope**: CAN signal analysis (optional)
- **CAN Analyzer**: Protocol verification (optional)
- **Bluetooth Scanner**: Connection testing

### Test Points
Include test points on PCB for:
- **Power Rails**: 3.3V, 5V, 12V
- **CAN Signals**: CANH, CANL
- **SPI Signals**: MOSI, MISO, SCK, CS
- **Control Signals**: Reset, Boot, Interrupt

## ?? Signal Integrity

### CAN Bus Specifications
- **Bit Rate**: 500 kbps
- **Logic Levels**: Recessive ~2.5V, Dominant ~1.5V/3.5V
- **Termination**: 60? differential (120? || 120?)
- **Cable**: Twisted pair, 120? characteristic impedance

### SPI Timing
- **Clock Speed**: Up to 10MHz (typically use 1MHz)
- **Setup Time**: Minimum 10ns
- **Hold Time**: Minimum 10ns
- **Propagation**: Keep traces < 6 inches

## ?? Troubleshooting Hardware

### Common Issues
1. **No CAN Communication**
   - Check termination resistor (should measure 60?)
   - Verify CANH/CANL not swapped
   - Confirm 16MHz crystal on MCP2515

2. **Bluetooth Connection Fails**
   - Check ESP32 power supply stability
   - Verify antenna placement
   - Test with different devices

3. **Random Resets**
   - Check power supply ripple
   - Verify decoupling capacitors
   - Test at temperature extremes

### Debugging Steps
1. **Power Check**: Verify all voltage rails
2. **Communication**: Test SPI with oscilloscope
3. **Software**: Use Serial Monitor for debug output
4. **Isolation**: Test each subsystem separately

## ?? Reference Documents

### Datasheets
- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [MCP2515 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP2515-Family-Data-Sheet-DS20001801J.pdf)
- [ISO 11898-2 CAN Standard](https://www.iso.org/standard/33423.html)

### Application Notes
- ESP32 Bluetooth Classic Configuration
- MCP2515 CAN Controller Application
- Automotive CAN Bus Design Guidelines

---

**?? Safety Note**: Always double-check connections before applying power. Incorrect wiring can damage expensive motorcycle electronics!
