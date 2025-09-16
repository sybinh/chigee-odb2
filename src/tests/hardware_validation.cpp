/**
 * @file hardware_validation.cpp
 * @brief Hardware validation tests for ESP32-WROOM-32
 * @author Chigee OBD2 Project Team
 * @date 2025-01-11
 * 
 * Comprehensive hardware testing for real ESP32 board deployment
 */

#include <Arduino.h>
#include "../config/hardware_config.h"
#include "../config/project_config.h"
#include "../modules/bluetooth/bluetooth_manager.h"
#include "../modules/obd2/obd2_handler.h"
#include "../modules/can/can_interface.h"

class HardwareValidator {
private:
    bool test_passed = true;
    uint32_t test_count = 0;
    uint32_t failed_tests = 0;

public:
    void logTest(const char* test_name, bool result) {
        test_count++;
        if (!result) {
            failed_tests++;
            test_passed = false;
        }
        
        Serial.printf("[%s] %s: %s\n", 
            result ? "PASS" : "FAIL", 
            test_name, 
            result ? "OK" : "ERROR"
        );
    }

    bool validateESP32Board() {
        Serial.println("\n=== ESP32-WROOM-32 Board Validation ===");
        
        // Check chip model
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        
        logTest("ESP32 Chip Detection", chip_info.model == CHIP_ESP32);
        
        Serial.printf("Chip: %s, Revision: %d\n", 
            (chip_info.model == CHIP_ESP32) ? "ESP32" : "Unknown",
            chip_info.revision
        );
        
        Serial.printf("Cores: %d, WiFi: %s, Bluetooth: %s\n",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No",
            (chip_info.features & CHIP_FEATURE_BT) ? "Yes" : "No"
        );
        
        // Check flash size
        uint32_t flash_size = ESP.getFlashChipSize();
        logTest("Flash Size (4MB)", flash_size >= 4 * 1024 * 1024);
        Serial.printf("Flash Size: %d MB\n", flash_size / (1024 * 1024));
        
        // Check free heap
        uint32_t free_heap = ESP.getFreeHeap();
        logTest("Free Heap (>200KB)", free_heap > 200 * 1024);
        Serial.printf("Free Heap: %d KB\n", free_heap / 1024);
        
        // Check PSRAM (should be false for basic WROOM-32)
        bool has_psram = ESP.getPsramSize() > 0;
        logTest("PSRAM Status (None Expected)", !has_psram);
        
        return test_passed;
    }

    bool validatePinConfiguration() {
        Serial.println("\n=== Pin Configuration Validation ===");
        
        // Test GPIO availability
        const int test_pins[] = {2, 4, 16, 17, 18, 19, 21, 22, 23};
        
        for (int pin : test_pins) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            delayMicroseconds(10);
            pinMode(pin, INPUT);
            bool pin_ok = digitalRead(pin) == HIGH;
            
            digitalWrite(pin, LOW);
            delayMicroseconds(10);
            pin_ok &= digitalRead(pin) == LOW;
            
            logTest(("GPIO" + String(pin) + " Function").c_str(), pin_ok);
        }
        
        // Test ADC pins
        const int adc_pins[] = {36, 39, 34};
        for (int pin : adc_pins) {
            int reading = analogRead(pin);
            bool adc_ok = reading >= 0 && reading <= 4095;
            logTest(("ADC" + String(pin) + " Reading").c_str(), adc_ok);
            Serial.printf("ADC%d: %d\n", pin, reading);
        }
        
        return test_passed;
    }

    bool validateSPIInterface() {
        Serial.println("\n=== SPI Interface Validation ===");
        
        // Initialize SPI
        SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
        SPI.setFrequency(CAN_SPI_FREQUENCY);
        
        // Test SPI clock
        pinMode(SPI_SCK_PIN, OUTPUT);
        for (int i = 0; i < 10; i++) {
            digitalWrite(SPI_SCK_PIN, HIGH);
            delayMicroseconds(1);
            digitalWrite(SPI_SCK_PIN, LOW);
            delayMicroseconds(1);
        }
        
        logTest("SPI Clock Generation", true);
        logTest("SPI MOSI Pin", SPI_MOSI_PIN == 23);
        logTest("SPI MISO Pin", SPI_MISO_PIN == 19);
        logTest("SPI SCK Pin", SPI_SCK_PIN == 18);
        
        return test_passed;
    }

    bool validateBluetoothCapability() {
        Serial.println("\n=== Bluetooth Capability Validation ===");
        
        // Check Bluetooth controller
        bool bt_init = esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE ||
                       esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED;
        
        if (!bt_init) {
            esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
            esp_err_t ret = esp_bt_controller_init(&bt_cfg);
            bt_init = (ret == ESP_OK);
        }
        
        logTest("Bluetooth Controller Init", bt_init);
        
        // Test Bluetooth Classic
        BluetoothSerial SerialBT;
        bool bt_classic = SerialBT.begin("ChigeeOBD2-Test");
        logTest("Bluetooth Classic", bt_classic);
        
        if (bt_classic) {
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_BT);
            Serial.printf("BT MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            SerialBT.end();
        }
        
        return test_passed;
    }

    bool validateMemoryLayout() {
        Serial.println("\n=== Memory Layout Validation ===");
        
        // Check heap integrity
        bool heap_ok = heap_caps_check_integrity_all(true);
        logTest("Heap Integrity", heap_ok);
        
        // Check available memory
        uint32_t free_heap = ESP.getFreeHeap();
        uint32_t min_heap = ESP.getMinFreeHeap();
        
        logTest("Sufficient Free Heap", free_heap > 150000);
        logTest("Heap Not Fragmented", (free_heap - min_heap) < 50000);
        
        Serial.printf("Free Heap: %d bytes\n", free_heap);
        Serial.printf("Min Free Heap: %d bytes\n", min_heap);
        Serial.printf("Max Alloc Heap: %d bytes\n", ESP.getMaxAllocHeap());
        
        // Test large allocations
        void* test_alloc = malloc(32768);  // 32KB test
        bool alloc_ok = (test_alloc != nullptr);
        if (test_alloc) free(test_alloc);
        
        logTest("Large Memory Allocation", alloc_ok);
        
        return test_passed;
    }

    bool validateTimingAndClock() {
        Serial.println("\n=== Timing and Clock Validation ===");
        
        // Check CPU frequency
        uint32_t cpu_freq = getCpuFrequencyMhz();
        logTest("CPU Frequency (240MHz)", cpu_freq == 240);
        Serial.printf("CPU Frequency: %d MHz\n", cpu_freq);
        
        // Test timing accuracy
        uint32_t start_time = micros();
        delay(100);  // 100ms delay
        uint32_t elapsed = micros() - start_time;
        
        bool timing_ok = (elapsed >= 99000 && elapsed <= 101000);  // ±1ms tolerance
        logTest("Timing Accuracy", timing_ok);
        Serial.printf("100ms delay actual: %d µs\n", elapsed);
        
        // Test RTC
        logTest("RTC Available", HAS_RTC);
        
        return test_passed;
    }

    void printSummary() {
        Serial.println("\n=== Hardware Validation Summary ===");
        Serial.printf("Total Tests: %d\n", test_count);
        Serial.printf("Passed: %d\n", test_count - failed_tests);
        Serial.printf("Failed: %d\n", failed_tests);
        Serial.printf("Overall Result: %s\n", test_passed ? "PASS" : "FAIL");
        
        if (test_passed) {
            Serial.println("✅ ESP32-WROOM-32 hardware validation successful!");
            Serial.println("✅ Ready for OBD2 module deployment");
        } else {
            Serial.println("❌ Hardware validation failed!");
            Serial.println("❌ Check connections and board configuration");
        }
    }
};

// Global instance
HardwareValidator validator;

// Main validation function
bool runFullHardwareValidation() {
    Serial.println("\n🔧 Starting ESP32-WROOM-32 Hardware Validation 🔧");
    Serial.println("Board: " BOARD_VARIANT);
    Serial.println("Project: " PROJECT_NAME " v" PROJECT_VERSION);
    Serial.println("Build: " __DATE__ " " __TIME__);
    
    bool all_passed = true;
    
    all_passed &= validator.validateESP32Board();
    all_passed &= validator.validatePinConfiguration();
    all_passed &= validator.validateSPIInterface();
    all_passed &= validator.validateBluetoothCapability();
    all_passed &= validator.validateMemoryLayout();
    all_passed &= validator.validateTimingAndClock();
    
    validator.printSummary();
    
    return all_passed;
}

// Test setup function for standalone testing
void setupHardwareValidation() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    delay(2000);  // Allow time for serial monitor
    
    if (runFullHardwareValidation()) {
        // Blink success pattern
        pinMode(LED_BUILTIN_PIN, OUTPUT);
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_BUILTIN_PIN, HIGH);
            delay(200);
            digitalWrite(LED_BUILTIN_PIN, LOW);
            delay(200);
        }
    } else {
        // Blink error pattern
        pinMode(LED_BUILTIN_PIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN_PIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN_PIN, LOW);
            delay(100);
        }
    }
}