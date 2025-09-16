#!/usr/bin/env python3
"""
Bluetooth Packet Capture Helper

This script helps set up and manage Bluetooth packet captures
for analyzing Chigee XR2 traffic on different platforms.

Author: Chigee OBD2 Project
Date: 2025-09-14
"""

import os
import sys
import time
import platform
import subprocess
import argparse
from datetime import datetime

class BluetoothCapture:
    """Cross-platform Bluetooth packet capture manager"""
    
    def __init__(self):
        self.platform = platform.system().lower()
        self.capture_process = None
        self.output_file = None
        
    def check_requirements(self) -> bool:
        """Check if required tools are available"""
        print(f"🔍 Checking requirements for {self.platform}...")
        
        if self.platform == 'linux':
            return self._check_linux_requirements()
        elif self.platform == 'windows':
            return self._check_windows_requirements()
        elif self.platform == 'darwin':  # macOS
            return self._check_macos_requirements()
        else:
            print(f"❌ Unsupported platform: {self.platform}")
            return False
            
    def _check_linux_requirements(self) -> bool:
        """Check Linux requirements"""
        required_tools = ['hcidump', 'btmon', 'hciconfig']
        missing_tools = []
        
        for tool in required_tools:
            if not self._command_exists(tool):
                missing_tools.append(tool)
                
        if missing_tools:
            print(f"❌ Missing tools: {', '.join(missing_tools)}")
            print("Install with: sudo apt-get install bluez-tools bluez-hcidump")
            return False
            
        # Check if user has bluetooth permissions
        if os.geteuid() != 0:
            print("⚠️  Root privileges required for Bluetooth capture")
            print("Run with: sudo python3 capture_helper.py")
            return False
            
        print("✅ Linux requirements satisfied")
        return True
        
    def _check_windows_requirements(self) -> bool:
        """Check Windows requirements"""
        print("🪟 Windows Bluetooth capture options:")
        print("   1. Use Wireshark with Bluetooth adapter")
        print("   2. Use external Bluetooth sniffer (Ubertooth)")
        print("   3. Use Android device with HCI snoop logging")
        print("✅ Manual setup required - see documentation")
        return True
        
    def _check_macos_requirements(self) -> bool:
        """Check macOS requirements"""
        print("🍎 macOS Bluetooth capture options:")
        print("   1. Use PacketLogger from Additional Tools for Xcode")
        print("   2. Use external Bluetooth sniffer (Ubertooth)")
        print("   3. Use Wireshark with external capture")
        print("✅ Manual setup required - see documentation")
        return True
        
    def _command_exists(self, command: str) -> bool:
        """Check if command exists in system PATH"""
        try:
            subprocess.run([command, '--version'], 
                          capture_output=True, check=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
            
    def list_bluetooth_devices(self) -> None:
        """List available Bluetooth devices"""
        print("📡 Scanning for Bluetooth devices...")
        
        if self.platform == 'linux':
            try:
                # Use hcitool to scan for devices
                result = subprocess.run(['hcitool', 'scan'], 
                                      capture_output=True, text=True, timeout=30)
                if result.returncode == 0:
                    print("Found devices:")
                    print(result.stdout)
                else:
                    print("❌ Error scanning for devices")
            except subprocess.TimeoutExpired:
                print("⏱️  Scan timeout - no devices found or scan taking too long")
            except FileNotFoundError:
                print("❌ hcitool not found")
                
        else:
            print("🔍 Use your system's Bluetooth settings to discover devices")
            
    def start_capture(self, target_mac: str = None, output_file: str = None) -> bool:
        """Start Bluetooth packet capture"""
        if not output_file:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            output_file = f'chigee_capture_{timestamp}.pcap'
            
        self.output_file = output_file
        
        print(f"🎯 Starting capture to: {output_file}")
        if target_mac:
            print(f"🔍 Targeting device: {target_mac}")
            
        if self.platform == 'linux':
            return self._start_linux_capture(target_mac, output_file)
        else:
            return self._start_manual_capture_guide(target_mac, output_file)
            
    def _start_linux_capture(self, target_mac: str, output_file: str) -> bool:
        """Start Linux Bluetooth capture using btmon"""
        try:
            # Use btmon for better packet capture
            cmd = ['btmon', '-w', output_file]
            
            if target_mac:
                # Note: btmon doesn't have direct MAC filtering
                # We'll filter in post-processing
                print(f"ℹ️  Will filter for {target_mac} in post-processing")
                
            print(f"🚀 Starting btmon capture...")
            self.capture_process = subprocess.Popen(cmd)
            
            print(f"✅ Capture started (PID: {self.capture_process.pid})")
            print("Press Ctrl+C to stop capture")
            
            return True
            
        except Exception as e:
            print(f"❌ Error starting capture: {e}")
            return False
            
    def _start_manual_capture_guide(self, target_mac: str, output_file: str) -> bool:
        """Provide manual capture instructions for non-Linux platforms"""
        print("📋 Manual Capture Instructions:")
        print("=" * 40)
        
        if self.platform == 'windows':
            print("Windows Capture Options:")
            print("1. Wireshark Method:")
            print("   - Install Wireshark")
            print("   - Start capture on Bluetooth interface")
            print(f"   - Save to: {output_file}")
            print("")
            print("2. Android HCI Snoop Method:")
            print("   - Enable Developer Options on Android")
            print("   - Enable 'Bluetooth HCI snoop log'")
            print("   - Connect Chigee to Android device")
            print("   - Extract log from /data/misc/bluetooth/logs/")
            
        elif self.platform == 'darwin':
            print("macOS Capture Options:")
            print("1. PacketLogger Method:")
            print("   - Install Xcode Additional Tools")
            print("   - Run PacketLogger")
            print("   - Enable Bluetooth logging")
            print(f"   - Export to: {output_file}")
            
        print("")
        print("3. External Sniffer (All Platforms):")
        print("   - Use Ubertooth One or similar")
        print("   - Follow device-specific instructions")
        
        return True
        
    def stop_capture(self) -> None:
        """Stop the capture process"""
        if self.capture_process:
            print("🛑 Stopping capture...")
            self.capture_process.terminate()
            self.capture_process.wait()
            print(f"✅ Capture saved to: {self.output_file}")
        else:
            print("ℹ️  No active capture to stop")
            
    def convert_capture(self, input_file: str, output_format: str = 'json') -> str:
        """Convert capture to analysis-friendly format"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        
        if output_format == 'json':
            output_file = f'chigee_capture_{timestamp}.json'
            
            # Use tshark to convert to JSON
            cmd = ['tshark', '-r', input_file, '-T', 'json']
            
            try:
                with open(output_file, 'w') as f:
                    subprocess.run(cmd, stdout=f, check=True)
                    
                print(f"✅ Converted to JSON: {output_file}")
                return output_file
                
            except subprocess.CalledProcessError:
                print("❌ Error converting capture - ensure tshark is installed")
                return None
            except FileNotFoundError:
                print("❌ tshark not found - install Wireshark")
                return None
                
        else:
            print(f"❌ Unsupported output format: {output_format}")
            return None

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='Bluetooth Packet Capture Helper')
    parser.add_argument('--target', '-t', help='Target device MAC address')
    parser.add_argument('--output', '-o', help='Output capture file')
    parser.add_argument('--scan', action='store_true', help='Scan for Bluetooth devices')
    parser.add_argument('--convert', help='Convert existing capture file to JSON')
    parser.add_argument('--duration', '-d', type=int, default=0, 
                       help='Capture duration in seconds (0 = manual stop)')
    
    args = parser.parse_args()
    
    print("📡 Bluetooth Packet Capture Helper")
    print("=" * 40)
    
    capture = BluetoothCapture()
    
    # Check requirements
    if not capture.check_requirements():
        return 1
        
    try:
        # Handle convert mode
        if args.convert:
            output_file = capture.convert_capture(args.convert)
            if output_file:
                print(f"🔍 Analyze with: python3 traffic_analyzer.py {output_file}")
            return 0
            
        # Handle scan mode
        if args.scan:
            capture.list_bluetooth_devices()
            return 0
            
        # Start capture
        if capture.start_capture(args.target, args.output):
            
            # Handle timed capture
            if args.duration > 0:
                print(f"⏱️  Capturing for {args.duration} seconds...")
                time.sleep(args.duration)
                capture.stop_capture()
            else:
                # Manual stop mode
                try:
                    while True:
                        time.sleep(1)
                except KeyboardInterrupt:
                    capture.stop_capture()
                    
        return 0
        
    except KeyboardInterrupt:
        print("\n🛑 Interrupted by user")
        capture.stop_capture()
        return 0
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return 1

if __name__ == '__main__':
    sys.exit(main())