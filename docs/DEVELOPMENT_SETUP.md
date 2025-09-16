# Professional Development Setup for Chigee XR2 OBD2 Module

## Overview

This guide will help you set up a professional development environment for the Chigee XR2 OBD2 project using modern tools and best practices. We've moved away from Arduino IDE to embrace a more professional workflow with PlatformIO, VS Code, and proper modular architecture.

## Prerequisites

### Required Software
- **VS Code**: Latest version with C/C++ support
- **PlatformIO Extension**: Professional IoT development platform
- **Git**: Version control (Windows Git Bash recommended)
- **Python 3.8+**: For analysis tools and automation scripts

### Hardware Requirements
- **ESP32 Development Board**: NodeMCU-32S, WROOM-32, or similar
- **MCP2515 CAN Controller**: SPI-based CAN bus interface
- **CAN Transceiver**: TJA1050 or similar
- **Debug Probe** (Optional): ESP-PROG for advanced debugging
- **Oscilloscope/Logic Analyzer** (Optional): For signal analysis

## Step 1: VS Code and PlatformIO Setup

### 1.1 Install VS Code
```bash
# Download from https://code.visualstudio.com/
# Or via package manager (Windows)
winget install Microsoft.VisualStudioCode

# macOS
brew install --cask visual-studio-code

# Ubuntu/Debian
sudo snap install code --classic
```

### 1.2 Install PlatformIO Extension
1. Open VS Code
2. Go to Extensions (`Ctrl+Shift+X`)
3. Search for "PlatformIO IDE"
4. Install the official PlatformIO extension
5. Restart VS Code

### 1.3 Install Additional Extensions
```
Recommended Extensions:
- C/C++ (Microsoft)
- GitLens (Git integration)
- Prettier (Code formatting)
- Todo Tree (Task management)
- Bracket Pair Colorizer
- Error Lens (Better error display)
```

## Step 2: Project Setup

### 2.1 Clone Repository
```bash
git clone https://github.com/sybinh/chigee-odb2.git
cd chigee-odb2
```

### 2.2 Open in VS Code
```bash
code .
# Or open the workspace file
code chigee-odb2.code-workspace
```

### 2.3 PlatformIO Project Structure
The project follows professional embedded development structure:

```
chigee-odb2/
├── platformio.ini          # Build configuration
├── src/                     # Source code (modular)
│   ├── main.cpp            # Entry point
│   ├── modules/            # Feature modules
│   │   ├── bluetooth/      # Bluetooth communication
│   │   ├── obd2/           # OBD2 protocol handling
│   │   ├── security/       # Advanced security features
│   │   ├── can/            # CAN bus interface
│   │   └── diagnostics/    # System diagnostics
│   ├── config/             # Configuration headers
│   └── utils/              # Utility functions
├── lib/                    # Custom libraries
├── test/                   # Unit tests
├── tools/                  # Analysis and testing tools
├── data/                   # Device databases
├── docs/                   # Documentation
└── hardware/               # Schematics and PCB files
```

## Step 3: Build Configuration

### 3.1 PlatformIO Configuration (`platformio.ini`)
```ini
[env:esp32dev]
platform = espressif32 @ 6.4.0
board = esp32dev
framework = arduino

# Build settings
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
upload_speed = 921600

# Compiler flags
build_flags = 
    -std=c++17
    -Wall
    -Wextra
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_ARDUHAL_LOG_COLORS=1
    -DTEST_MODE=true
    -DENABLE_ADVANCED_SECURITY=true
    -DCAN_SPEED=CAN_500KBPS

# Dependencies
lib_deps = 
    mcp_can @ ^1.5.0
    ArduinoJson @ ^6.21.0

# Debugging configuration
debug_tool = esp-prog
debug_init_break = tbreak setup
debug_build_flags = -O0 -g3 -ggdb

[env:esp32dev_release]
extends = env:esp32dev
build_flags = 
    ${env:esp32dev.build_flags}
    -DTEST_MODE=false
    -O2
    -DNDEBUG

[env:testing]
extends = env:esp32dev
build_flags = 
    ${env:esp32dev.build_flags}
    -DUNIT_TESTING=true
```

### 3.2 VS Code Workspace Configuration
Create `chigee-odb2.code-workspace`:

```json
{
    "folders": [
        {
            "path": "."
        }
    ],
    "settings": {
        "C_Cpp.intelliSenseEngine": "Tag Parser",
        "files.associations": {
            "*.ino": "cpp",
            "*.h": "c",
            "*.hpp": "cpp"
        },
        "editor.tabSize": 2,
        "editor.insertSpaces": true,
        "files.trimTrailingWhitespace": true,
        "files.insertFinalNewline": true
    },
    "extensions": {
        "recommendations": [
            "platformio.platformio-ide",
            "ms-vscode.cpptools",
            "eamodio.gitlens",
            "gruntfuggly.todo-tree"
        ]
    }
}
```

## Step 4: Modular Code Architecture

### 4.1 Main Application Structure
```cpp
// src/main.cpp - Clean entry point
#include "config/project_config.h"
#include "modules/bluetooth/bluetooth_manager.h"
#include "modules/obd2/obd2_handler.h"
#include "modules/security/security_manager.h"
#include "modules/can/can_interface.h"
#include "modules/diagnostics/system_monitor.h"

BluetoothManager bluetooth;
OBD2Handler obd2;
SecurityManager security;
CANInterface canBus;
SystemMonitor monitor;

void setup() {
    // Initialize all modules
    monitor.begin();
    security.initialize();
    bluetooth.begin();
    obd2.initialize();
    canBus.setup();
}

void loop() {
    // Main application loop
    monitor.update();
    bluetooth.handleConnections();
    obd2.processCommands();
    canBus.processMessages();
    
    // Yield to watchdog
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

### 4.2 Module Structure Example
```cpp
// src/modules/bluetooth/bluetooth_manager.h
#pragma once

#include "BluetoothSerial.h"
#include "../../config/project_config.h"

class BluetoothManager {
private:
    BluetoothSerial serialBT;
    bool isConnected;
    String deviceName;
    
public:
    bool begin();
    void handleConnections();
    bool sendResponse(const String& response);
    String receiveCommand();
    bool isClientConnected() const;
    void setDeviceName(const String& name);
};
```

## Step 5: Building and Debugging

### 5.1 Basic Build Commands
```bash
# Build project
pio run

# Build specific environment
pio run -e esp32dev

# Upload to device
pio run --target upload

# Open serial monitor
pio device monitor
```

### 5.2 VS Code Integration
- **Build**: `Ctrl+Alt+B` or `Ctrl+Shift+P` > "PlatformIO: Build"
- **Upload**: `Ctrl+Alt+U` or `Ctrl+Shift+P` > "PlatformIO: Upload"
- **Debug**: `F5` (requires debug probe)
- **Monitor**: `Ctrl+Alt+S` or "PlatformIO: Serial Monitor"

### 5.3 Advanced Debugging
With ESP-PROG debugger:

```json
// .vscode/launch.json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "platformio-debug",
            "request": "launch",
            "name": "PIO Debug",
            "executable": ".pio/build/esp32dev/firmware.elf",
            "projectEnvName": "esp32dev",
            "toolchainBinDir": "~/.platformio/packages/toolchain-xtensa-esp32/bin",
            "preLaunchTask": {
                "type": "PlatformIO",
                "task": "Pre-Debug"
            }
        }
    ]
}
```

## Step 6: Testing Framework

### 6.1 Unit Testing Setup
```cpp
// test/test_bluetooth/test_bluetooth.cpp
#include <unity.h>
#include "modules/bluetooth/bluetooth_manager.h"

void test_bluetooth_initialization() {
    BluetoothManager bt;
    TEST_ASSERT_TRUE(bt.begin());
}

void test_bluetooth_device_name() {
    BluetoothManager bt;
    bt.setDeviceName("TEST_DEVICE");
    TEST_ASSERT_EQUAL_STRING("TEST_DEVICE", bt.getDeviceName().c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_bluetooth_initialization);
    RUN_TEST(test_bluetooth_device_name);
    UNITY_END();
}

void loop() {}
```

### 6.2 Running Tests
```bash
# Run all tests
pio test

# Run specific test
pio test -e testing

# Run tests with verbose output
pio test -v
```

## Step 7: Professional Development Workflow

### 7.1 Git Workflow
```bash
# Feature development
git checkout -b feature/bluetooth-security
git add .
git commit -m "feat: implement MAC address spoofing"
git push origin feature/bluetooth-security

# Create pull request for code review
```

### 7.2 Code Quality
```bash
# Format code (add to tasks.json)
clang-format -i src/**/*.cpp src/**/*.h

# Static analysis
cppcheck src/ --enable=all

# Memory analysis (with debugging)
valgrind --tool=memcheck .pio/build/esp32dev/firmware.elf
```

### 7.3 Continuous Integration
Create `.github/workflows/ci.yml`:

```yaml
name: PlatformIO CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - uses: actions/cache@v3
      with:
        path: |
          ~/.cache/pip
          ~/.platformio/.cache
        key: ${{ runner.os }}-pio
    - uses: actions/setup-python@v4
      with:
        python-version: '3.9'
    - name: Install PlatformIO Core
      run: pip install --upgrade platformio
    - name: Build PlatformIO Project
      run: pio run
    - name: Run Tests
      run: pio test -e testing
```

## Step 8: Hardware Testing Setup

### 8.1 Development Board Configuration
```cpp
// src/config/hardware_config.h
#pragma once

// ESP32 Pin Definitions
#define CAN_CS_PIN     5
#define CAN_INT_PIN    21
#define CAN_MOSI_PIN   23
#define CAN_MISO_PIN   19
#define CAN_SCK_PIN    18

#define LED_STATUS_PIN 2
#define DEBUG_TX_PIN   17
#define DEBUG_RX_PIN   16

// Hardware capabilities detection
#define HAS_EXTERNAL_CAN    true
#define HAS_DEBUG_UART      true
#define HAS_STATUS_LED      true
```

### 8.2 Real Hardware Validation
```cpp
// src/modules/diagnostics/hardware_test.cpp
class HardwareTest {
public:
    bool testCANController();
    bool testBluetoothRadio();
    bool testMemoryIntegrity();
    bool testPowerSupply();
    void runFullDiagnostics();
};
```

## Troubleshooting

### Common Issues

1. **"PlatformIO not found"**:
   ```bash
   # Reload VS Code window
   Ctrl+Shift+P > "Developer: Reload Window"
   ```

2. **"Board not detected"**:
   ```bash
   # Check USB drivers
   pio device list
   
   # Manual port specification
   pio run --upload-port COM3  # Windows
   pio run --upload-port /dev/ttyUSB0  # Linux
   ```

3. **"Build errors"**:
   ```bash
   # Clean build
   pio run --target clean
   pio run
   ```

4. **"Debug not working"**:
   - Verify ESP-PROG connections
   - Check debug_tool configuration in platformio.ini
   - Ensure debug build flags are set

### Performance Optimization

1. **Build Speed**:
   ```ini
   # In platformio.ini
   build_flags = -j4  # Parallel compilation
   ```

2. **Code Size**:
   ```ini
   build_flags = -Os  # Optimize for size
   ```

3. **Debug Performance**:
   ```ini
   debug_build_flags = -Og  # Optimize for debugging
   ```

## Best Practices

1. **Code Organization**: Keep modules independent and testable
2. **Error Handling**: Use proper error codes and logging
3. **Memory Management**: Monitor heap usage and avoid leaks
4. **Documentation**: Comment complex algorithms and hardware interfaces
5. **Version Control**: Commit frequently with descriptive messages
6. **Testing**: Write unit tests for critical functionality

---

**Next Steps**: Once setup is complete, proceed to the hardware testing phase and start implementing the modular architecture described in this guide.