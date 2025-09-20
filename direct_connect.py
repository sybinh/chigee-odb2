#!/usr/bin/env python3
"""
Direct XR-2 Connection Tool
"""
import asyncio
from bleak import BleakClient

XR2_MAC = "C0:76:5A:02:C0:33"

async def connect_direct():
    """Connect directly to XR-2 using known MAC"""
    print(f"[DIRECT_CONNECT] Attempting to connect to XR-2: {XR2_MAC}")
    
    try:
        async with BleakClient(XR2_MAC) as client:
            print(f"[SUCCESS] Connected to XR-2!")
            
            # Get services
            services = client.services
            print(f"[SERVICES] Found services:")
            
            for service in services:
                print(f"  Service: {service.uuid}")
                for char in service.characteristics:
                    print(f"    Char: {char.uuid} - Properties: {char.properties}")
                    
            print("\n[MONITORING] notifications for 10 seconds...")
            
            # Subscribe to notifications
            def notification_handler(sender, data):
                print(f"[NOTIFICATION] {sender}: {data.hex()}")
            
            for service in services:
                for char in service.characteristics:
                    if "notify" in char.properties:
                        try:
                            await client.start_notify(char.uuid, notification_handler)
                            print(f"[SUBSCRIBED] {char.uuid}")
                        except:
                            pass
            
            await asyncio.sleep(10)
            
    except Exception as e:
        print(f"[ERROR] Connection failed: {e}")
        print("Possible causes:")
        print("- XR-2 is not powered on")
        print("- XR-2 is not in pairing/discoverable mode")
        print("- XR-2 is connected to another device")
        print("- MAC address has changed")

if __name__ == "__main__":
    asyncio.run(connect_direct())