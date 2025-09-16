/**
 * @file dummy_data_generator.cpp
 * @brief Realistic vehicle data simulation for testing
 * @author Chigee OBD2 Project Team
 * @date 2025-01-11
 * 
 * Generates realistic vehicle telemetry for testing without real car
 */

#include <Arduino.h>
#include "../config/project_config.h"

class DummyDataGenerator {
private:
    // Vehicle state
    bool engine_running = false;
    bool vehicle_moving = false;
    uint32_t engine_runtime = 0;
    uint32_t last_update = 0;
    
    // Engine parameters
    float engine_rpm = 0.0;
    float engine_load = 0.0;
    float engine_temp = 20.0;  // Start at ambient
    float throttle_position = 0.0;
    
    // Vehicle dynamics
    float vehicle_speed = 0.0;
    float fuel_level = 75.0;   // Start with 3/4 tank
    float fuel_rate = 0.0;
    
    // Diagnostic data
    uint8_t dtc_count = 0;
    bool mil_status = false;
    
    // Simulation parameters
    float ambient_temp = 25.0;
    bool ac_on = false;
    bool headlights_on = false;

public:
    struct VehicleData {
        // Engine
        uint16_t rpm;
        uint8_t load_pct;
        int16_t coolant_temp;
        uint8_t throttle_pct;
        uint16_t map_pressure;
        uint16_t maf_rate;
        
        // Vehicle
        uint8_t speed_kmh;
        uint8_t fuel_level_pct;
        uint16_t fuel_rate_lph;
        
        // Status
        bool engine_on;
        bool mil_on;
        uint8_t dtc_count;
        
        // Environmental
        int8_t ambient_temp;
        int8_t intake_temp;
        
        // System
        uint16_t battery_voltage;  // In 0.1V units
        uint8_t fuel_trim_short;
        uint8_t fuel_trim_long;
    };

    void startEngine() {
        if (!engine_running) {
            engine_running = true;
            engine_rpm = 800 + random(-50, 51);  // Idle RPM with variation
            engine_temp = ambient_temp;
            Serial.println("🚗 Engine started - generating realistic telemetry");
        }
    }

    void stopEngine() {
        if (engine_running) {
            engine_running = false;
            engine_rpm = 0;
            vehicle_speed = 0;
            throttle_position = 0;
            vehicle_moving = false;
            Serial.println("🛑 Engine stopped");
        }
    }

    void setDrivingMode(bool city_driving = true) {
        if (!engine_running) return;
        
        if (city_driving) {
            // City driving simulation
            vehicle_speed = 30 + random(-10, 21);  // 20-50 km/h
            engine_rpm = 1500 + random(-200, 301);  // 1300-1800 RPM
            throttle_position = 25 + random(-10, 21);  // 15-45%
            engine_load = 35 + random(-10, 16);  // 25-50%
            vehicle_moving = true;
            Serial.println("🏙️ City driving mode activated");
        } else {
            // Highway driving simulation
            vehicle_speed = 100 + random(-10, 21);  // 90-120 km/h
            engine_rpm = 2200 + random(-100, 201);  // 2100-2400 RPM
            throttle_position = 45 + random(-15, 21);  // 30-65%
            engine_load = 55 + random(-15, 21);  // 40-75%
            vehicle_moving = true;
            Serial.println("🛣️ Highway driving mode activated");
        }
    }

    void setIdleMode() {
        if (!engine_running) return;
        
        vehicle_speed = 0;
        engine_rpm = 750 + random(-50, 101);  // 700-850 RPM
        throttle_position = 0;
        engine_load = 15 + random(-5, 11);  // 10-25%
        vehicle_moving = false;
        Serial.println("⏸️ Idle mode - parked with engine running");
    }

    void simulateAcceleration() {
        if (!engine_running) return;
        
        // Simulate acceleration burst
        throttle_position = 80 + random(-10, 21);  // 70-100%
        engine_rpm = 4000 + random(-500, 1001);   // 3500-5000 RPM
        engine_load = 85 + random(-10, 16);       // 75-100%
        
        if (vehicle_moving) {
            vehicle_speed = min(140.0f, vehicle_speed + 20 + random(-5, 11));
        }
        
        Serial.println("🚀 Acceleration simulation");
    }

    void simulateDeceleration() {
        if (!engine_running || !vehicle_moving) return;
        
        // Simulate braking/deceleration
        throttle_position = 0;
        engine_rpm = max(750.0f, engine_rpm - 500 - random(0, 501));
        engine_load = 10 + random(-5, 11);  // 5-20%
        vehicle_speed = max(0.0f, vehicle_speed - 15 - random(0, 11));
        
        if (vehicle_speed < 5) {
            vehicle_moving = false;
            vehicle_speed = 0;
        }
        
        Serial.println("🛑 Deceleration simulation");
    }

    void simulateFault() {
        // Simulate a diagnostic trouble code
        dtc_count = 1;
        mil_status = true;
        
        // Simulate P0171 - System Too Lean (Bank 1)
        Serial.println("⚠️ Simulating DTC P0171 - System Too Lean");
        Serial.println("💡 MIL (Check Engine Light) ON");
    }

    void clearFaults() {
        dtc_count = 0;
        mil_status = false;
        Serial.println("✅ DTCs cleared - MIL OFF");
    }

    VehicleData generateRealisticData() {
        uint32_t now = millis();
        
        // Update timing
        if (last_update == 0) last_update = now;
        uint32_t delta_ms = now - last_update;
        last_update = now;
        
        // Update engine runtime
        if (engine_running) {
            engine_runtime += delta_ms;
            
            // Gradual temperature increase
            if (engine_temp < 90) {
                engine_temp += (delta_ms / 1000.0) * 0.5;  // 0.5°C per second
            }
            
            // Fuel consumption
            if (vehicle_moving) {
                float consumption = (engine_load / 100.0) * (engine_rpm / 1000.0) * 0.1;
                fuel_level -= consumption * (delta_ms / 1000.0) * 0.001;  // Very slow for demo
                fuel_rate = consumption * 3.6;  // L/h
            } else {
                fuel_rate = 0.8;  // Idle consumption
                fuel_level -= 0.8 * (delta_ms / 1000.0) * 0.0001;  // Minimal idle consumption
            }
        }
        
        // Add realistic variations
        if (engine_running) {
            engine_rpm += random(-20, 21);  // ±20 RPM variation
            engine_load += random(-2, 3);   // ±2% load variation
            
            // Constrain values
            engine_rpm = constrain(engine_rpm, 600, 6500);
            engine_load = constrain(engine_load, 0, 100);
            throttle_position = constrain(throttle_position, 0, 100);
            vehicle_speed = constrain(vehicle_speed, 0, 200);
        }
        
        // Build data structure
        VehicleData data = {};
        
        // Engine data
        data.rpm = (uint16_t)engine_rpm;
        data.load_pct = (uint8_t)engine_load;
        data.coolant_temp = (int16_t)(engine_temp - 40);  // OBD2 offset
        data.throttle_pct = (uint8_t)throttle_position;
        data.map_pressure = (uint16_t)(50 + (engine_load * 2));  // Rough MAP calculation
        data.maf_rate = (uint16_t)((engine_rpm * engine_load) / 100);  // Rough MAF
        
        // Vehicle data
        data.speed_kmh = (uint8_t)vehicle_speed;
        data.fuel_level_pct = (uint8_t)constrain(fuel_level, 0, 100);
        data.fuel_rate_lph = (uint16_t)(fuel_rate * 100);  // 0.01 L/h resolution
        
        // Status
        data.engine_on = engine_running;
        data.mil_on = mil_status;
        data.dtc_count = dtc_count;
        
        // Environmental
        data.ambient_temp = (int8_t)ambient_temp;
        data.intake_temp = (int8_t)(ambient_temp + (engine_running ? 10 : 0));
        
        // System
        data.battery_voltage = (uint16_t)((12.6 + (engine_running ? 1.8 : 0)) * 10);  // 0.1V units
        data.fuel_trim_short = 128 + random(-10, 11);  // ±8% trim
        data.fuel_trim_long = 128 + random(-5, 6);     // ±4% trim
        
        return data;
    }

    void printTelemetry() {
        VehicleData data = generateRealisticData();
        
        Serial.println("\n📊 === Vehicle Telemetry ===");
        Serial.printf("Engine: %s | Speed: %d km/h | RPM: %d\n", 
            data.engine_on ? "ON" : "OFF", data.speed_kmh, data.rpm);
        Serial.printf("Load: %d%% | Throttle: %d%% | Temp: %d°C\n",
            data.load_pct, data.throttle_pct, data.coolant_temp + 40);
        Serial.printf("Fuel: %d%% | Rate: %.2f L/h | Battery: %.1fV\n",
            data.fuel_level_pct, data.fuel_rate_lph / 100.0, data.battery_voltage / 10.0);
        Serial.printf("MIL: %s | DTCs: %d | Runtime: %d sec\n",
            data.mil_on ? "ON" : "OFF", data.dtc_count, engine_runtime / 1000);
    }

    // OBD2 PID response simulation
    String getOBD2Response(const String& pid) {
        VehicleData data = generateRealisticData();
        
        if (pid == "010C") {        // Engine RPM
            uint16_t rpm_raw = data.rpm * 4;
            return String("41 0C ") + String(rpm_raw >> 8, HEX) + " " + String(rpm_raw & 0xFF, HEX);
        }
        else if (pid == "010D") {   // Vehicle Speed
            return String("41 0D ") + String(data.speed_kmh, HEX);
        }
        else if (pid == "0104") {   // Engine Load
            return String("41 04 ") + String((data.load_pct * 255) / 100, HEX);
        }
        else if (pid == "0105") {   // Coolant Temperature
            return String("41 05 ") + String(data.coolant_temp + 40, HEX);
        }
        else if (pid == "0111") {   // Throttle Position
            return String("41 11 ") + String((data.throttle_pct * 255) / 100, HEX);
        }
        else if (pid == "012F") {   // Fuel Level
            return String("41 2F ") + String((data.fuel_level_pct * 255) / 100, HEX);
        }
        else if (pid == "0142") {   // Battery Voltage
            return String("41 42 ") + String(data.battery_voltage >> 8, HEX) + " " + String(data.battery_voltage & 0xFF, HEX);
        }
        else if (pid == "0101") {   // Monitor Status
            uint32_t status = data.mil_on ? 0x80000000 : 0x00000000;
            return String("41 01 ") + String(status >> 24, HEX) + " " + String((status >> 16) & 0xFF, HEX) + " " + 
                   String((status >> 8) & 0xFF, HEX) + " " + String(status & 0xFF, HEX);
        }
        
        return "NO DATA";  // Unknown PID
    }
};

// Global dummy data generator
DummyDataGenerator dummyData;