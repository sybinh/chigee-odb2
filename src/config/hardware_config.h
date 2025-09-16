#pragma once

/**
 * @file hardware_config.h
 * @brief Hardware abstraction and pin definitions for ESP32 platform
 * @author Chigee OBD2 Project Team
 * @date 2025-09-15
 * 
 * This file contains hardware-specific configurations and abstractions
 * for the ESP32-based Chigee XR2 OBD2 module.
 */

#include "project_config.h"

// ===== HARDWARE PLATFORM DETECTION =====
#if defined(ESP32)
  #define PLATFORM_ESP32          true
  #define PLATFORM_NAME           "ESP32"
  #define MCU_FREQUENCY           CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ
#else
  #error "Unsupported hardware platform"
#endif

// ===== ESP32 SPECIFIC CONFIGURATIONS =====
#ifdef PLATFORM_ESP32
  #include <Arduino.h>
  #include <SPI.h>
  #include <BluetoothSerial.h>
  
  // Flash and RAM specifications
  #define FLASH_SIZE_BYTES        (4 * 1024 * 1024)   // 4MB
  #define RAM_SIZE_BYTES          (520 * 1024)        // 520KB
  #define PSRAM_AVAILABLE         false               // No PSRAM on basic modules
  
  // Hardware capabilities
  #define HAS_BLUETOOTH_CLASSIC   true
  #define HAS_BLUETOOTH_BLE       true
  #define HAS_WIFI                true
  #define HAS_HARDWARE_SPI        true
  #define HAS_HARDWARE_I2C        true
  #define HAS_HARDWARE_UART       3     // Number of UARTs
  #define HAS_ADC                 true
  #define HAS_DAC                 true
  #define HAS_PWM                 true
  #define HAS_RTC                 true
  #define HAS_WATCHDOG            true
#endif

// ===== PIN MAPPING FOR ESP32-WROOM-32 =====

// SPI Bus 0 (used by MCP2515 CAN controller)
#define SPI_BUS_CAN             0
#define CAN_SPI_FREQUENCY       10000000  // 10MHz max for MCP2515
#define CAN_CS_PIN              5         // Chip Select
#define CAN_INT_PIN             21        // Interrupt (must be interrupt-capable)
#define CAN_RESET_PIN           22        // Reset pin (optional)

// Default SPI pins (VSPI)
#define SPI_MOSI_PIN            23        // Master Out Slave In
#define SPI_MISO_PIN            19        // Master In Slave Out  
#define SPI_SCK_PIN             18        // Serial Clock

// Status LEDs
#define LED_BUILTIN_PIN         2         // Built-in blue LED
#define LED_STATUS_PIN          LED_BUILTIN_PIN
#define LED_CAN_ACTIVITY_PIN    4         // External LED for CAN activity
#define LED_BT_CONNECTED_PIN    16        // External LED for Bluetooth status
#define LED_ERROR_PIN           17        // External LED for error indication

// Debug UART (UART2)
#define DEBUG_UART_NUM          2
#define DEBUG_UART_TX_PIN       17
#define DEBUG_UART_RX_PIN       16
#define DEBUG_UART_BAUD         115200

// I2C Bus (for future expansion)
#define I2C_SDA_PIN             21
#define I2C_SCL_PIN             22
#define I2C_FREQUENCY           400000    // 400kHz

// Analog inputs (for diagnostics)
#define ANALOG_VOLTAGE_PIN      36        // ADC1_CH0 - Battery voltage monitoring
#define ANALOG_CURRENT_PIN      39        // ADC1_CH3 - Current consumption
#define ANALOG_TEMP_PIN         34        // ADC1_CH6 - Temperature sensor

// Digital I/O
#define BUTTON_BOOT_PIN         0         // Boot button (built-in)
#define BUTTON_USER_PIN         35        // User button (external)
#define RELAY_POWER_PIN         32        // Power control relay

// GPIO reserved pins (do not use)
#define RESERVED_PINS           {6, 7, 8, 9, 10, 11}  // Connected to flash

// ===== HARDWARE ABSTRACTION MACROS =====

// LED control
#define LED_ON(pin)             digitalWrite(pin, HIGH)
#define LED_OFF(pin)            digitalWrite(pin, LOW)
#define LED_TOGGLE(pin)         digitalWrite(pin, !digitalRead(pin))

// Pin mode setup
#define PIN_OUTPUT(pin)         pinMode(pin, OUTPUT)
#define PIN_INPUT(pin)          pinMode(pin, INPUT)
#define PIN_INPUT_PULLUP(pin)   pinMode(pin, INPUT_PULLUP)
#define PIN_INPUT_PULLDOWN(pin) pinMode(pin, INPUT_PULLDOWN)

// Digital I/O
#define PIN_HIGH(pin)           digitalWrite(pin, HIGH)
#define PIN_LOW(pin)            digitalWrite(pin, LOW)
#define PIN_READ(pin)           digitalRead(pin)

// Analog I/O
#define ANALOG_READ(pin)        analogRead(pin)
#define ANALOG_WRITE(pin, val)  analogWrite(pin, val)

// ===== POWER MANAGEMENT =====
#define ENABLE_POWER_MANAGEMENT true
#define DEEP_SLEEP_ENABLED      false     // Disable for always-on OBD2
#define LIGHT_SLEEP_ENABLED     true      // Light sleep between operations

// Power consumption targets
#define IDLE_CURRENT_MA         80        // Target idle current
#define ACTIVE_CURRENT_MA       150       // Target active current
#define PEAK_CURRENT_MA         200       // Peak current during transmission

// Battery monitoring
#define BATTERY_VOLTAGE_MIN     11.0      // Minimum battery voltage (V)
#define BATTERY_VOLTAGE_MAX     14.5      // Maximum battery voltage (V)
#define BATTERY_VOLTAGE_NOMINAL 12.6      // Nominal battery voltage (V)

// ===== TIMING AND PERFORMANCE =====
#define CPU_CLOCK_MHZ           240       // ESP32 CPU frequency
#define APB_CLOCK_MHZ           80        // APB bus frequency
#define XTAL_FREQUENCY          40000000  // 40MHz crystal

// Task timing
#define TICK_RATE_HZ            1000      // FreeRTOS tick rate
#define MS_TO_TICKS(ms)         ((ms) * TICK_RATE_HZ / 1000)
#define TICKS_TO_MS(ticks)      ((ticks) * 1000 / TICK_RATE_HZ)

// Performance monitoring
#define ENABLE_CPU_USAGE_MONITOR true
#define ENABLE_MEMORY_MONITOR    true
#define ENABLE_TASK_MONITOR      true

// ===== INTERRUPT CONFIGURATION =====
#define CAN_INTERRUPT_PRIORITY  5         // High priority for CAN
#define TIMER_INTERRUPT_PRIORITY 3        // Medium priority for timers
#define GPIO_INTERRUPT_PRIORITY 1         // Low priority for GPIO

// Interrupt types
#define INT_RISING              RISING
#define INT_FALLING             FALLING
#define INT_CHANGE              CHANGE
#define INT_LOW                 LOW
#define INT_HIGH                HIGH

// ===== MEMORY LAYOUT =====
// ESP32 memory regions
#define IRAM_START              0x40080000
#define IRAM_SIZE               (128 * 1024)
#define DRAM_START              0x3FFB0000  
#define DRAM_SIZE               (176 * 1024)
#define FLASH_START             0x400C2000
#define FLASH_SIZE              FLASH_SIZE_BYTES

// Memory allocation guidelines
#define STACK_SIZE_MAIN         8192      // Main task stack
#define STACK_SIZE_CAN          4096      // CAN task stack
#define STACK_SIZE_BLUETOOTH    4096      // Bluetooth task stack
#define STACK_SIZE_OBD2         4096      // OBD2 handler stack

// ===== WATCHDOG CONFIGURATION =====
#define WATCHDOG_TIMEOUT_SEC    30        // 30 second timeout
#define ENABLE_TASK_WATCHDOG    true      // Enable task watchdog
#define ENABLE_INTERRUPT_WATCHDOG true    // Enable interrupt watchdog

// ===== DEBUGGING FEATURES =====
#ifdef ENABLE_DEBUG_LOGGING
  #define DEBUG_LED_ENABLE      true
  #define DEBUG_UART_ENABLE     true
  #define DEBUG_GPIO_MONITOR    true
  
  // Debug output macros
  #define HW_DEBUG_PRINT(...)   Serial.printf(__VA_ARGS__)
  #define HW_DEBUG_PRINTLN(x)   Serial.println(x)
#else
  #define HW_DEBUG_PRINT(...)
  #define HW_DEBUG_PRINTLN(x)
#endif

// ===== SAFETY FEATURES =====
#define ENABLE_BROWNOUT_DETECTION true
#define BROWNOUT_THRESHOLD_V    2.6       // Reset if VDD < 2.6V
#define ENABLE_STACK_OVERFLOW_CHECK true
#define ENABLE_HEAP_CORRUPTION_CHECK true

// Emergency shutdown
#define EMERGENCY_SHUTDOWN_PIN  0         // Boot button for emergency stop
#define ENABLE_EMERGENCY_SHUTDOWN true

// ===== HARDWARE VALIDATION =====
// Function prototypes for hardware validation
namespace HardwareValidation {
    bool validatePinConfiguration();
    bool testSPIInterface();
    bool testBluetoothRadio();
    bool testCANController();
    bool testMemoryIntegrity();
    bool runFullHardwareTest();
}

// ===== BOARD VARIANT DETECTION =====
#if defined(ARDUINO_ESP32_DEV)
  #define BOARD_VARIANT "ESP32-WROOM-32 DevKit"
#elif defined(ARDUINO_ESP32S2_DEV)
  #define BOARD_VARIANT "ESP32-S2 DevKit"
#elif defined(ARDUINO_ESP32C3_DEV)
  #define BOARD_VARIANT "ESP32-C3 DevKit"
#else
  #define BOARD_VARIANT "Generic ESP32"
#endif

// ===== COMPATIBILITY CHECKS =====
#if !defined(ESP32)
  #error "This hardware configuration is only compatible with ESP32"
#endif

#if defined(SPOOF_DEVICE_MAC) && SPOOF_DEVICE_MAC
  #warning "MAC address spoofing enabled - ensure you understand the implications"
#endif

#if CAN_SPI_FREQUENCY > 10000000
  #error "MCP2515 maximum SPI frequency is 10MHz"
#endif

// ===== FEATURE AVAILABILITY MATRIX =====
struct HardwareCapabilities {
    static constexpr bool bluetooth_classic = HAS_BLUETOOTH_CLASSIC;
    static constexpr bool bluetooth_ble = HAS_BLUETOOTH_BLE;
    static constexpr bool wifi = HAS_WIFI;
    static constexpr bool can_controller = true;  // External MCP2515
    static constexpr bool rtc = HAS_RTC;
    static constexpr bool watchdog = HAS_WATCHDOG;
    static constexpr uint8_t uart_count = HAS_HARDWARE_UART;
    static constexpr uint8_t adc_channels = 18;
    static constexpr uint8_t dac_channels = 2;
    static constexpr uint8_t pwm_channels = 16;
};

#endif // HARDWARE_CONFIG_H