#!/usr/bin/env python3
"""
Enhanced BLE Traffic Analyzer with Direct Connection
Based on Domyos EL500 reverse engineering methodology
"""
import asyncio
import time
from bleak import BleakClient, BleakScanner
from datetime import datetime

# Known device MAC addresses
XR2_MAC = "C0:76:5A:02:C0:33"
ESP32_MAC_PREFIX = "CGOBD"  # Look for devices starting with this

class BLETrafficAnalyzer:
    def __init__(self):
        self.traffic_log = []
        self.message_patterns = {}
        self.startup_sequence = []
        
    async def scan_for_targets(self):
        """Scan for known target devices"""
        print("[SCANNING] for target devices...")
        devices = await BleakScanner.discover(timeout=10)
        
        targets = []
        for device in devices:
            name = device.name or "Unknown"
            if device.address == XR2_MAC:
                print(f"[FOUND] XR-2: {name} ({device.address})")
                targets.append(("XR-2", device))
            elif ESP32_MAC_PREFIX in name:
                print(f"[FOUND] ESP32: {name} ({device.address})")
                targets.append(("ESP32", device))
                
        return targets
    
    async def analyze_device(self, device_type, device):
        """Connect and analyze BLE traffic patterns"""
        print(f"\n[ANALYZING] {device_type}: {device.address}")
        
        try:
            async with BleakClient(device.address) as client:
                print(f"[CONNECTED] to {device.address}")
                
                # Discover services and characteristics
                await self.discover_gatt_structure(client, device_type)
                
                # Subscribe to all notifications
                await self.subscribe_to_notifications(client, device_type)
                
                # Monitor traffic for pattern analysis
                await self.monitor_traffic(client, device_type)
                
        except Exception as e:
            print(f"[ERROR] Failed to analyze {device_type}: {e}")
    
    async def discover_gatt_structure(self, client, device_type):
        """Discover and log GATT structure"""
        print(f"[GATT_DISCOVERY] {device_type}")
        services = await client.get_services()
        
        for service in services:
            print(f"  Service: {service.uuid}")
            for char in service.characteristics:
                print(f"    Characteristic: {char.uuid}")
                print(f"      Properties: {char.properties}")
                
                # Log descriptors (CCCDs)
                for desc in char.descriptors:
                    print(f"        Descriptor: {desc.uuid}")
    
    async def subscribe_to_notifications(self, client, device_type):
        """Subscribe to all available notifications"""
        services = await client.get_services()
        
        for service in services:
            for char in service.characteristics:
                if "notify" in char.properties:
                    try:
                        def notification_handler(sender, data):
                            self.log_traffic(device_type, sender, data)
                        
                        await client.start_notify(char.uuid, notification_handler)
                        print(f"[SUBSCRIBED] {device_type} - {char.uuid}")
                    except Exception as e:
                        print(f"[ERROR] Subscribe failed {char.uuid}: {e}")
    
    def log_traffic(self, device_type, sender, data):
        """Log traffic and analyze patterns"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        hex_data = data.hex()
        
        # Log the traffic
        entry = {
            'timestamp': timestamp,
            'device': device_type,
            'characteristic': str(sender),
            'data': hex_data,
            'length': len(data)
        }
        self.traffic_log.append(entry)
        
        print(f"[TRAFFIC] {timestamp} {device_type} {sender}: {hex_data}")
        
        # Analyze patterns (like Domyos article)
        self.analyze_message_patterns(hex_data, len(data))
    
    def analyze_message_patterns(self, hex_data, length):
        """Analyze message patterns for common prefixes"""
        if len(hex_data) >= 4:
            prefix = hex_data[:4]  # First 2 bytes
            
            if length not in self.message_patterns:
                self.message_patterns[length] = {}
            
            if prefix not in self.message_patterns[length]:
                self.message_patterns[length][prefix] = 0
            
            self.message_patterns[length][prefix] += 1
    
    async def monitor_traffic(self, client, device_type):
        """Monitor traffic for specified duration"""
        print(f"[MONITORING] {device_type} traffic for 30 seconds...")
        await asyncio.sleep(30)
    
    def print_analysis_summary(self):
        """Print traffic analysis summary"""
        print("\n" + "="*60)
        print("TRAFFIC ANALYSIS SUMMARY")
        print("="*60)
        
        print(f"Total messages captured: {len(self.traffic_log)}")
        
        print("\nMessage patterns by length:")
        for length, prefixes in self.message_patterns.items():
            print(f"  {length} bytes:")
            for prefix, count in sorted(prefixes.items(), key=lambda x: x[1], reverse=True):
                print(f"    {prefix}: {count} occurrences")
        
        # Save detailed log
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"traffic_analysis_{timestamp}.log"
        
        with open(filename, 'w') as f:
            f.write("BLE Traffic Analysis Log\n")
            f.write("="*50 + "\n\n")
            
            for entry in self.traffic_log:
                f.write(f"{entry['timestamp']} {entry['device']} {entry['characteristic']}: {entry['data']}\n")
        
        print(f"\nDetailed log saved to: {filename}")

async def main():
    """Main analysis function"""
    print("BLE Traffic Analyzer - Domyos Methodology")
    print("="*50)
    
    analyzer = BLETrafficAnalyzer()
    
    # Scan for target devices
    targets = await analyzer.scan_for_targets()
    
    if not targets:
        print("[ERROR] No target devices found")
        print("Make sure XR-2 and ESP32 are powered on and discoverable")
        return
    
    # Analyze each target
    tasks = []
    for device_type, device in targets:
        task = analyzer.analyze_device(device_type, device)
        tasks.append(task)
    
    # Run analysis in parallel
    await asyncio.gather(*tasks, return_exceptions=True)
    
    # Print summary
    analyzer.print_analysis_summary()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[STOPPED] Analysis interrupted by user")