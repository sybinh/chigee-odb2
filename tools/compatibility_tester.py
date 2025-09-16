#!/usr/bin/env python3
"""
Chigee XR2 Compatibility Test Suite

This script tests our ESP32 OBD2 emulator against the security
and compatibility requirements discovered through reverse engineering.

Author: Chigee OBD2 Project
Date: 2025-09-14
"""

import json
import time
import bluetooth
import serial
import argparse
from datetime import datetime
from typing import Dict, List, Optional, Tuple

class ChigeeCompatibilityTester:
    """Test suite for validating Chigee XR2 compatibility"""
    
    def __init__(self, target_mac: str = None, serial_port: str = None):
        self.target_mac = target_mac
        self.serial_port = serial_port
        self.bluetooth_sock = None
        self.serial_conn = None
        self.test_results = []
        
    def connect_bluetooth(self) -> bool:
        """Connect to ESP32 via Bluetooth"""
        if not self.target_mac:
            print("❌ No target MAC address specified")
            return False
            
        try:
            print(f"📡 Connecting to ESP32 at {self.target_mac}...")
            
            # Create Bluetooth socket
            self.bluetooth_sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            self.bluetooth_sock.connect((self.target_mac, 1))  # SPP channel 1
            
            print("✅ Bluetooth connection established")
            return True
            
        except Exception as e:
            print(f"❌ Bluetooth connection failed: {e}")
            return False
            
    def connect_serial(self) -> bool:
        """Connect to ESP32 via Serial (for development)"""
        if not self.serial_port:
            print("❌ No serial port specified")
            return False
            
        try:
            print(f"🔌 Connecting to ESP32 at {self.serial_port}...")
            
            self.serial_conn = serial.Serial(self.serial_port, 115200, timeout=2)
            time.sleep(2)  # Wait for connection
            
            print("✅ Serial connection established")
            return True
            
        except Exception as e:
            print(f"❌ Serial connection failed: {e}")
            return False
            
    def send_command(self, command: str, timeout: float = 2.0) -> Tuple[str, float]:
        """Send AT command and measure response time"""
        start_time = time.time()
        
        try:
            if self.bluetooth_sock:
                # Send via Bluetooth
                self.bluetooth_sock.send((command + '\r').encode())
                response = self.bluetooth_sock.recv(1024).decode().strip()
            elif self.serial_conn:
                # Send via Serial
                self.serial_conn.write((command + '\r\n').encode())
                response = self.serial_conn.readline().decode().strip()
            else:
                return "ERROR: No connection", 0.0
                
            response_time = (time.time() - start_time) * 1000  # Convert to ms
            return response, response_time
            
        except Exception as e:
            return f"ERROR: {e}", 0.0
            
    def test_basic_at_commands(self) -> Dict:
        """Test basic AT command functionality"""
        print("\n🔧 Testing Basic AT Commands...")
        
        basic_tests = [
            ("ATZ", "ELM327 v1.5", "Reset command"),
            ("ATE0", "OK", "Echo off"),
            ("ATI", "ELM327 v1.5", "Device identification"),
            ("ATRV", "12.", "Battery voltage"),
            ("ATDP", "AUTO", "Current protocol"),
        ]
        
        results = {"passed": 0, "failed": 0, "details": []}
        
        for cmd, expected_partial, description in basic_tests:
            response, response_time = self.send_command(cmd)
            
            if expected_partial in response:
                status = "✅ PASS"
                results["passed"] += 1
            else:
                status = "❌ FAIL"
                results["failed"] += 1
                
            test_detail = {
                "command": cmd,
                "description": description,
                "expected": expected_partial,
                "actual": response,
                "response_time_ms": response_time,
                "status": status
            }
            
            results["details"].append(test_detail)
            print(f"   {status} {cmd}: {response} ({response_time:.1f}ms)")
            
        return results
        
    def test_device_fingerprinting(self) -> Dict:
        """Test device fingerprinting resistance"""
        print("\n🔍 Testing Device Fingerprinting...")
        
        fingerprint_tests = [
            ("AT@1", "Device description"),
            ("AT@2", "Device identifier"),
            ("AT@3", "Device copyright"),
            ("ATCS", "Checksum command"),
            ("ATCV", "Calibration voltage"),
        ]
        
        results = {"responses": {}, "timing_analysis": []}
        
        for cmd, description in fingerprint_tests:
            response, response_time = self.send_command(cmd)
            results["responses"][cmd] = {
                "response": response,
                "response_time_ms": response_time,
                "description": description
            }
            print(f"   🔍 {cmd}: {response} ({response_time:.1f}ms)")
            
        # Timing analysis
        timing_tests = ["ATI"] * 10  # Test timing consistency
        times = []
        
        for cmd in timing_tests:
            _, response_time = self.send_command(cmd)
            times.append(response_time)
            time.sleep(0.1)
            
        avg_time = sum(times) / len(times)
        time_variance = max(times) - min(times)
        
        results["timing_analysis"] = {
            "average_ms": avg_time,
            "variance_ms": time_variance,
            "times": times,
            "realistic": 40 <= avg_time <= 150  # Realistic hardware timing
        }
        
        print(f"   ⏱️  Timing: {avg_time:.1f}ms avg, {time_variance:.1f}ms variance")
        
        return results
        
    def test_security_bypass(self) -> Dict:
        """Test security bypass mechanisms"""
        print("\n🔐 Testing Security Bypass...")
        
        security_tests = [
            ("ATZ", "Initialization security"),
            ("ATCHGID", "Chigee proprietary ID"),
            ("ATCHGAUTH", "Chigee authentication"),
            ("ATCHGCRYPT12345", "Crypto challenge"),
        ]
        
        results = {"security_responses": {}, "bypass_effectiveness": "unknown"}
        
        for cmd, description in security_tests:
            response, response_time = self.send_command(cmd)
            results["security_responses"][cmd] = {
                "response": response,
                "response_time_ms": response_time,
                "description": description
            }
            print(f"   🔐 {cmd}: {response} ({response_time:.1f}ms)")
            
        # Test command sequence validation
        sequence_test = ["ATZ", "ATE0", "ATI", "ATSP0"]
        sequence_success = True
        
        for cmd in sequence_test:
            response, _ = self.send_command(cmd)
            if "ERROR" in response or response == "?":
                sequence_success = False
                break
                
        results["sequence_validation"] = {
            "passed": sequence_success,
            "description": "Standard initialization sequence"
        }
        
        print(f"   📋 Sequence validation: {'✅ PASS' if sequence_success else '❌ FAIL'}")
        
        return results
        
    def test_chigee_specific_behavior(self) -> Dict:
        """Test Chigee XR2 specific behavior patterns"""
        print("\n🏍️  Testing Chigee XR2 Specific Behavior...")
        
        # Test rapid command handling (some security systems check for this)
        rapid_commands = ["ATI", "ATRV", "ATDP", "ATI"]
        rapid_results = []
        
        start_time = time.time()
        for cmd in rapid_commands:
            response, response_time = self.send_command(cmd, timeout=1.0)
            rapid_results.append({
                "command": cmd,
                "response": response,
                "time": response_time
            })
            # Minimal delay between commands
            time.sleep(0.01)
            
        total_time = (time.time() - start_time) * 1000
        
        # Test error handling
        error_commands = ["ATXYZ", "AT!@#", "INVALIDCMD"]
        error_responses = []
        
        for cmd in error_commands:
            response, response_time = self.send_command(cmd)
            error_responses.append({
                "command": cmd,
                "response": response,
                "expected_error": "?" in response or "ERROR" in response
            })
            
        results = {
            "rapid_command_handling": {
                "total_time_ms": total_time,
                "commands": rapid_results,
                "avg_response_ms": sum([r["time"] for r in rapid_results]) / len(rapid_results)
            },
            "error_handling": error_responses,
            "connection_stability": True  # Will be set based on connection drops
        }
        
        print(f"   ⚡ Rapid commands: {total_time:.1f}ms total")
        print(f"   ❌ Error handling: {len([r for r in error_responses if r['expected_error']])}/{len(error_responses)} correct")
        
        return results
        
    def test_obd_pid_simulation(self) -> Dict:
        """Test OBD PID simulation accuracy"""
        print("\n📊 Testing OBD PID Simulation...")
        
        test_pids = [
            ("010C", "Engine RPM"),
            ("010D", "Vehicle speed"),
            ("0105", "Engine coolant temperature"),
            ("010F", "Intake air temperature"),
            ("0111", "Throttle position"),
            ("0142", "Control module voltage"),
        ]
        
        results = {"pid_responses": {}, "simulation_quality": "unknown"}
        
        for pid, description in test_pids:
            response, response_time = self.send_command(pid)
            
            # Parse response format (should be like "41 0C 1A F8")
            is_valid_format = False
            if len(response.split()) >= 3:
                parts = response.split()
                if parts[0] == "41" and parts[1] == pid[2:4]:
                    is_valid_format = True
                    
            results["pid_responses"][pid] = {
                "description": description,
                "response": response,
                "response_time_ms": response_time,
                "valid_format": is_valid_format
            }
            
            status = "✅" if is_valid_format else "❌"
            print(f"   {status} {pid} ({description}): {response} ({response_time:.1f}ms)")
            
        # Calculate simulation quality
        valid_responses = len([r for r in results["pid_responses"].values() if r["valid_format"]])
        total_responses = len(results["pid_responses"])
        quality_score = (valid_responses / total_responses) * 100
        
        results["simulation_quality"] = {
            "score_percent": quality_score,
            "valid_responses": valid_responses,
            "total_responses": total_responses
        }
        
        print(f"   📈 Simulation quality: {quality_score:.1f}% ({valid_responses}/{total_responses})")
        
        return results
        
    def run_full_test_suite(self) -> Dict:
        """Run the complete test suite"""
        print("🧪 Chigee XR2 Compatibility Test Suite")
        print("=" * 50)
        
        # Connect to device
        if self.target_mac:
            if not self.connect_bluetooth():
                return {"error": "Failed to connect via Bluetooth"}
        elif self.serial_port:
            if not self.connect_serial():
                return {"error": "Failed to connect via Serial"}
        else:
            return {"error": "No connection method specified"}
            
        # Run all tests
        test_results = {
            "test_timestamp": datetime.now().isoformat(),
            "target_device": self.target_mac or self.serial_port,
            "basic_at_commands": self.test_basic_at_commands(),
            "device_fingerprinting": self.test_device_fingerprinting(),
            "security_bypass": self.test_security_bypass(),
            "chigee_behavior": self.test_chigee_specific_behavior(),
            "obd_simulation": self.test_obd_pid_simulation(),
        }
        
        # Calculate overall compatibility score
        basic_score = (test_results["basic_at_commands"]["passed"] / 
                      (test_results["basic_at_commands"]["passed"] + test_results["basic_at_commands"]["failed"])) * 100
        
        timing_realistic = test_results["device_fingerprinting"]["timing_analysis"]["realistic"]
        security_sequence = test_results["security_bypass"]["sequence_validation"]["passed"]
        simulation_quality = test_results["obd_simulation"]["simulation_quality"]["score_percent"]
        
        overall_score = (basic_score + (50 if timing_realistic else 0) + 
                        (50 if security_sequence else 0) + simulation_quality) / 3
        
        test_results["compatibility_assessment"] = {
            "overall_score": overall_score,
            "basic_functionality": basic_score,
            "timing_realistic": timing_realistic,
            "security_bypass": security_sequence,
            "simulation_quality": simulation_quality,
            "chigee_ready": overall_score >= 80
        }
        
        print(f"\n📊 Overall Compatibility Score: {overall_score:.1f}%")
        print(f"🏍️  Chigee XR2 Ready: {'✅ YES' if overall_score >= 80 else '❌ NO'}")
        
        return test_results
        
    def disconnect(self):
        """Clean up connections"""
        if self.bluetooth_sock:
            self.bluetooth_sock.close()
        if self.serial_conn:
            self.serial_conn.close()

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='Chigee XR2 Compatibility Test Suite')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--bluetooth', '-b', help='ESP32 Bluetooth MAC address')
    group.add_argument('--serial', '-s', help='ESP32 Serial port (e.g., COM3, /dev/ttyUSB0)')
    parser.add_argument('--output', '-o', help='Output file for test results (JSON)')
    parser.add_argument('--quick', action='store_true', help='Run quick test (basic commands only)')
    
    args = parser.parse_args()
    
    # Create tester instance
    tester = ChigeeCompatibilityTester(
        target_mac=args.bluetooth,
        serial_port=args.serial
    )
    
    try:
        if args.quick:
            # Quick test - basic AT commands only
            print("⚡ Running Quick Compatibility Test...")
            if args.bluetooth:
                tester.connect_bluetooth()
            else:
                tester.connect_serial()
            results = {"basic_at_commands": tester.test_basic_at_commands()}
        else:
            # Full test suite
            results = tester.run_full_test_suite()
            
        # Save results if requested
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(results, f, indent=2)
            print(f"📄 Test results saved to: {args.output}")
            
        return 0 if results.get("compatibility_assessment", {}).get("chigee_ready", False) else 1
        
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
        return 1
    except Exception as e:
        print(f"❌ Test failed: {e}")
        return 1
    finally:
        tester.disconnect()

if __name__ == '__main__':
    import sys
    sys.exit(main())