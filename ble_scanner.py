#!/usr/bin/env python3
"""
BLE Scanner để tìm Chigee XR-2 và ESP32 CGOBD
"""
import asyncio
import time
from bleak import BleakScanner, BleakClient

async def scan_devices():
    """Scan for BLE devices"""
    print("[SCANNING] for BLE devices...")
    print("=" * 50)
    
    devices = await BleakScanner.discover(timeout=10)
    
    chigee_found = False
    esp32_found = False
    
    for device in devices:
        name = device.name or "Unknown"
        mac = device.address
        rssi = getattr(device, 'rssi', 'N/A')
        
        print(f"[DEVICE] {name}")
        print(f"   MAC: {mac}")
        print(f"   RSSI: {rssi}")
        print()
        
        # Check for Chigee XR-2
        if "CHIGEE" in name.upper():
            print("🎯 CHIGEE XR-2 FOUND!")
            chigee_found = True
            
        # Check for ESP32 CGOBD
        if "CGOBD" in name.upper():
            print("🎯 ESP32 CGOBD FOUND!")
            esp32_found = True
    
    print("=" * 50)
    print(f"[OK] Chigee XR-2: {'Found' if chigee_found else 'Not found'}")
    print(f"[OK] ESP32 CGOBD: {'Found' if esp32_found else 'Not found'}")
    
    return devices

async def connect_and_monitor(mac_address):
    """Connect to device and monitor GATT traffic"""
    print(f"\n[CONNECTING] to {mac_address}...")
    
    try:
        async with BleakClient(mac_address) as client:
            print(f"[CONNECTED] to {mac_address}")
            
            # Get services
            services = client.services
            print("\n📋 Available services:")
            
            for service in services:
                print(f"Service: {service.uuid}")
                for char in service.characteristics:
                    print(f"  Characteristic: {char.uuid}")
                    properties = [str(prop) for prop in char.properties]
                    print(f"    Properties: {', '.join(properties)}")
            
            # Monitor notifications
            print("\n[MONITORING] notifications (Press Ctrl+C to stop)...")
            
            def notification_handler(sender, data):
                print(f"[NOTIFICATION] From {sender}: {data.hex()}")
                print(f"   ASCII: {''.join(chr(b) if 32 <= b <= 126 else '.' for b in data)}")
            
            # Subscribe to notifications
            for service in services:
                for char in service.characteristics:
                    if "notify" in [str(prop).lower() for prop in char.properties]:
                        try:
                            await client.start_notify(char.uuid, notification_handler)
                            print(f"[SUBSCRIBED] to notifications from {char.uuid}")
                        except Exception as e:
                            print(f"[ERROR] Failed to subscribe to {char.uuid}: {e}")
            
            # Keep connection alive
            while True:
                await asyncio.sleep(1)
                
    except Exception as e:
        print(f"[ERROR] Connection failed: {e}")

async def main():
    """Main function"""
    print("BLE Traffic Monitor for Chigee XR-2 <-> ESP32")
    print("=" * 60)
    
    # Scan first
    devices = await scan_devices()
    
    # Ask user which device to monitor
    if devices:
        print("\n🎯 Select device to monitor:")
        for i, device in enumerate(devices):
            name = device.name or "Unknown"
            print(f"{i+1}. {name} ({device.address})")
        
        try:
            choice = input("\nEnter device number (or 'q' to quit): ").strip()
            if choice.lower() == 'q':
                return
                
            device_idx = int(choice) - 1
            if 0 <= device_idx < len(devices):
                selected_device = devices[device_idx]
                await connect_and_monitor(selected_device.address)
            else:
                print("[ERROR] Invalid selection")
        except (ValueError, KeyboardInterrupt):
            print("\n👋 Goodbye!")
    else:
        print("[ERROR] No devices found")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 Scan cancelled by user")