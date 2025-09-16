# Chigee XR2 OBD2 Module - System Architecture

## Overview Diagram

```
┌─────────────────────────────────────────────────┐
│           Chigee XR2 OBD2 Module                │
│         Professional Modular Architecture       │
└─────────────────────────────────────────────────┘
                         │
            ┌────────────┼────────────┐
            │            │            │
    ┌───────▼──────┐ ┌───▼───┐ ┌──────▼──────┐
    │ ESP32-WROOM-32│ │Config │ │Core Modules │
    │   Hardware    │ │Layer  │ │             │
    │   Platform    │ │       │ │             │
    └───────────────┘ └───┬───┘ └──────┬──────┘
                          │            │
              ┌───────────┼────────────┼───────────┐
              │           │            │           │
        ┌─────▼─────┐ ┌───▼───┐ ┌──────▼──────┐ ┌─▼─┐
        │project_   │ │hardware│ │BluetoothMgr │ │...│
        │config.h   │ │_config │ │OBD2Handler  │ │   │
        │           │ │.h      │ │CANInterface │ │   │
        └───────────┘ └────────┘ └─────────────┘ └───┘
```

## Layer Architecture

### 1. Hardware Layer
- **ESP32-WROOM-32**: Main microcontroller
- **GPIO Pins**: CAN TX/RX, LEDs, Debug UART
- **Power Management**: 12V to 3.3V conversion
- **Protection Circuits**: ESD, reverse polarity

### 2. Configuration Layer
- **project_config.h**: 
  - Feature flags
  - Pin definitions  
  - Build settings
  - Debug options
- **hardware_config.h**:
  - ESP32 abstraction
  - Hardware validation
  - Power management
  - Pin mappings

### 3. Core Modules

#### BluetoothManager
```
┌─────────────────────────────────────┐
│          BluetoothManager           │
├─────────────────────────────────────┤
│ • Device Spoofing                   │
│   - MAC address spoofing            │
│   - Device profiles                 │
│   - ELM327 emulation               │
│   - Manufacturer ID                 │
│                                     │
│ • Connection Management             │
│   - SPP service                     │
│   - Pairing control                 │
│   - Auto-reconnect                  │
│   - Status monitoring               │
│                                     │
│ • Security Features                 │
│   - PIN validation                  │
│   - Fingerprinting                  │
│   - Access control                  │
│   - Logging                         │
└─────────────────────────────────────┘
```

#### OBD2Handler
```
┌─────────────────────────────────────┐
│            OBD2Handler              │
├─────────────────────────────────────┤
│ • AT Commands                       │
│   - ATZ reset                       │
│   - ATE echo control                │
│   - ATSP protocol                   │
│   - ATI device info                 │
│                                     │
│ • PID Processing                    │
│   - Mode 01 current data            │
│   - Mode 03 DTCs                    │
│   - Mode 09 VIN info                │
│   - Custom PIDs                     │
│                                     │
│ • Vehicle Simulation                │
│   - Realistic data                  │
│   - Dynamic values                  │
│   - Statistics tracking             │
│   - Error simulation                │
└─────────────────────────────────────┘
```

#### CANInterface
```
┌─────────────────────────────────────┐
│            CANInterface             │
├─────────────────────────────────────┤
│ • ESP32 TWAI Driver                 │
│   - CAN controller init             │
│   - Message TX/RX                   │
│   - Error handling                  │
│   - Bus-off recovery                │
│                                     │
│ • Message Filtering                 │
│   - Whitelist/blacklist             │
│   - ID ranges                       │
│   - Custom filters                  │
│   - OBD2 specific                   │
│                                     │
│ • Statistics & Debug                │
│   - Message counters                │
│   - Error tracking                  │
│   - Bus utilization                 │
│   - Diagnostics                     │
└─────────────────────────────────────┘
```

## Data Flow

```
Chigee XR2    BluetoothMgr    OBD2Handler    CANInterface    Vehicle ECU
    │              │              │              │              │
    │─── BT SPP ───▶│              │              │              │
    │              │─── AT Cmd ───▶│              │              │
    │              │              │─── CAN Msg ──▶│              │
    │              │              │              │─── OBD2 ────▶│
    │              │              │              │◄── Data ─────│
    │              │◄─── OBD2 ────│◄─── Response ─│              │
    │◄─── Data ────│              │              │              │
    │              │              │              │              │
```

## External Interfaces

### Chigee XR2 Radar
- **Interface**: Bluetooth SPP
- **Protocol**: OBD2 AT commands
- **Function**: Vehicle data monitoring
- **Connection**: Pairs as ELM327

### Vehicle ECU
- **Interface**: CAN bus (OBD2 port)
- **Protocol**: ISO 15765-4 (CAN 11-bit, 500kbps)
- **Function**: Real-time vehicle data
- **Connection**: Physical OBD2 connector

## Development Tools

### PlatformIO
- **Professional build system**
- **Dependency management**
- **Testing framework**
- **Cross-platform support**

### VS Code + Debug
- **Code completion**
- **Hardware debugging**
- **Serial monitoring**
- **Git integration**

## Key Features

✅ **Professional modular architecture**
✅ **Hardware abstraction layer**  
✅ **Device spoofing (ELM327)**
✅ **Real-time vehicle simulation**
✅ **Professional development tools**
✅ **Comprehensive error handling**
✅ **Security features**
✅ **Statistics and diagnostics**

## System Flow Steps

1. **Chigee XR2 connects via Bluetooth**
2. **BluetoothManager handles connection**
3. **AT commands sent to OBD2Handler**
4. **OBD2Handler processes commands** 
5. **CANInterface sends to vehicle ECU**
6. **Response flows back through chain**
7. **Realistic vehicle data simulated**
8. **Statistics and diagnostics updated**

---

**This replaces the amateur Arduino IDE approach with professional modular architecture!**