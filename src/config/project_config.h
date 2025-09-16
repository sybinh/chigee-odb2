#pragma once

/**
 * @file project_config.h
 * @brief Main configuration file for Chigee XR2 OBD2 Module
 * @author Chigee OBD2 Project Team
 * @date 2025-09-15
 * 
 * This file contains all project-wide configuration constants and feature flags.
 * Modify these settings to customize behavior for different deployment scenarios.
 */

// ===== PROJECT INFORMATION =====
#define PROJECT_NAME        "Chigee XR2 OBD2 Module"
#define PROJECT_VERSION     "1.2.0"
#define PROJECT_BUILD_DATE  __DATE__ " " __TIME__
#define PROJECT_AUTHOR      "Chigee OBD2 Project Team"

// ===== HARDWARE CONFIGURATION =====
#define TARGET_BOARD        "ESP32-WROOM-32"
#define CPU_FREQUENCY_MHZ   240
#define FLASH_SIZE_MB       4

// ===== FEATURE FLAGS =====
#ifndef TEST_MODE
#define TEST_MODE           true    // Safe testing without real CAN connection
#endif

#ifndef ENABLE_ADVANCED_SECURITY
#define ENABLE_ADVANCED_SECURITY  true   // Chigee XR2 bypass mechanisms
#endif

#define ENABLE_DEBUG_LOGGING      true   // Verbose debug output
#define ENABLE_PERFORMANCE_MONITOR true  // System performance tracking
#define ENABLE_MEMORY_MONITORING  true   // Heap usage monitoring
#define ENABLE_OTA_UPDATES        false  // Over-the-air firmware updates
#define ENABLE_WEB_INTERFACE      false  // Web-based configuration

// ===== SECURITY CONFIGURATION =====
#define SPOOF_DEVICE_MAC          false  // WARNING: Changes MAC permanently!
#define SPOOF_TARGET_DEVICE       "genuine_elm327_v15"
#define ENABLE_MAC_RANDOMIZATION  false  // Random MAC on each boot
#define ENABLE_CRYPTO_CHALLENGES  true   // Handle proprietary auth
#define SECURITY_LOGGING_LEVEL    2      // 0=none, 1=basic, 2=verbose

// ===== COMMUNICATION SETTINGS =====
#define BLUETOOTH_DEVICE_NAME     "OBDII_CHIGEE"
#define BLUETOOTH_PIN             "1234"
#define BLUETOOTH_TIMEOUT_MS      30000
#define BLUETOOTH_BUFFER_SIZE     512

#define SERIAL_BAUD_RATE          115200
#define SERIAL_BUFFER_SIZE        256

// ===== CAN BUS CONFIGURATION =====
#ifndef CAN_SPEED
#define CAN_SPEED                 CAN_500KBPS  // Husqvarna/KTM standard
#endif

#define CAN_TIMEOUT_MS            100
#define CAN_RETRY_COUNT           3
#define CAN_FILTER_ENABLE         true

// Vehicle-specific settings
#define VEHICLE_MANUFACTURER      "Husqvarna"
#define VEHICLE_MODEL             "Svartpilen 401"
#define VEHICLE_YEAR              2020
#define ENGINE_DISPLACEMENT_CC    373

// ===== OBD2 PROTOCOL SETTINGS =====
#define OBD2_PROTOCOL_AUTO        true
#define OBD2_DEFAULT_PROTOCOL     "ISO 15765-4 (CAN 11/500)"
#define OBD2_RESPONSE_TIMEOUT_MS  200
#define OBD2_MAX_RETRIES          2

// ELM327 Emulation Settings
#define ELM327_VERSION            "1.5"
#define ELM327_DEVICE_ID          "ELM327"
#define ELM327_COPYRIGHT          "Copyright (c) ELM Electronics"

// ===== DATA SIMULATION SETTINGS =====
#define SIMULATION_UPDATE_RATE_MS 500   // How often to update simulated data
#define SIMULATION_VARIANCE       0.1   // ±10% variation in simulated values

// Realistic vehicle data ranges
#define ENGINE_RPM_IDLE           800
#define ENGINE_RPM_MAX            9000
#define VEHICLE_SPEED_MAX         160   // km/h
#define COOLANT_TEMP_NORMAL       85    // °C
#define THROTTLE_POSITION_MAX     100   // %

// ===== SYSTEM LIMITS =====
#define MAX_CONCURRENT_CONNECTIONS 1
#define MAX_COMMAND_LENGTH        32
#define MAX_RESPONSE_LENGTH       128
#define MAX_LOG_ENTRIES          100

#define WATCHDOG_TIMEOUT_MS      30000  // 30 second watchdog
#define TASK_STACK_SIZE          4096   // Default task stack size
#define TASK_PRIORITY_HIGH       3
#define TASK_PRIORITY_NORMAL     2
#define TASK_PRIORITY_LOW        1

// ===== MEMORY CONFIGURATION =====
#define HEAP_WARNING_THRESHOLD   10240  // Warn when free heap < 10KB
#define HEAP_CRITICAL_THRESHOLD  5120   // Critical when free heap < 5KB
#define MAX_MALLOC_SIZE          8192   // Maximum single allocation

// ===== TIMING CONFIGURATION =====
#define MAIN_LOOP_DELAY_MS       10     // Main loop cycle time
#define BLUETOOTH_POLL_RATE_MS   50     // Bluetooth connection check rate
#define CAN_POLL_RATE_MS         20     // CAN message processing rate
#define DIAGNOSTICS_UPDATE_MS    5000   // System diagnostics update rate

// ===== PIN DEFINITIONS =====
// SPI pins for MCP2515 CAN controller
#define CAN_CS_PIN               5      // Chip Select
#define CAN_INT_PIN              21     // Interrupt pin
#define CAN_MOSI_PIN             23     // SPI MOSI
#define CAN_MISO_PIN             19     // SPI MISO
#define CAN_SCK_PIN              18     // SPI Clock

// Status and debug pins
#define LED_STATUS_PIN           2      // Built-in LED
#define LED_CAN_ACTIVITY_PIN     4      // CAN activity indicator
#define LED_BT_CONNECTED_PIN     16     // Bluetooth connection indicator

// Debug UART (optional)
#define DEBUG_UART_TX_PIN        17
#define DEBUG_UART_RX_PIN        16
#define DEBUG_UART_BAUD          115200

// ===== ADVANCED FEATURES =====
#define ENABLE_FREQ_MEASUREMENT  false  // Frequency counter on spare pins
#define ENABLE_ANALOG_SENSORS    false  // Additional analog sensor inputs
#define ENABLE_I2C_EXPANSION     false  // I2C sensor bus
#define ENABLE_BOOTLOADER_MODE   false  // Custom bootloader support

// ===== DEBUGGING AND DEVELOPMENT =====
#ifdef DEBUG
#define DEBUG_PRINT(x)           Serial.print(x)
#define DEBUG_PRINTLN(x)         Serial.println(x)
#define DEBUG_PRINTF(...)        Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

// Assertion macros for development
#ifdef TEST_MODE
#define ASSERT(condition, message) \
  do { \
    if (!(condition)) { \
      Serial.printf("ASSERTION FAILED: %s at %s:%d\n", message, __FILE__, __LINE__); \
      while(1); \
    } \
  } while(0)
#else
#define ASSERT(condition, message)
#endif

// ===== COMPATIBILITY DEFINITIONS =====
// Ensure compatibility with older code during transition
#define SIMULATE_REALISTIC_DATA   true
#define ENABLE_PHASE_MANAGEMENT   true

// Legacy defines (will be removed in future versions)
#if defined(ARDUINO_ESP32_DEV)
#define PLATFORM_ESP32           true
#endif

// ===== VERSION COMPATIBILITY =====
#if __cplusplus < 201103L
#error "This project requires C++11 or later"
#endif

// ===== BUILD CONFIGURATION VALIDATION =====
#if TEST_MODE && !ENABLE_DEBUG_LOGGING
#warning "Test mode enabled but debug logging disabled - consider enabling for better debugging"
#endif

#if ENABLE_ADVANCED_SECURITY && !ENABLE_DEBUG_LOGGING
#warning "Advanced security enabled but debug logging disabled - security events may not be visible"
#endif

#if SPOOF_DEVICE_MAC
#warning "MAC address spoofing enabled - this permanently changes ESP32 MAC address!"
#endif

// ===== RUNTIME ENVIRONMENT DETECTION =====
#define IS_DEVELOPMENT_BUILD     (TEST_MODE || ENABLE_DEBUG_LOGGING)
#define IS_PRODUCTION_BUILD      (!TEST_MODE && !ENABLE_DEBUG_LOGGING)

#endif // PROJECT_CONFIG_H