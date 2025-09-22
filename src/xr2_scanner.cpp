#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* pBLEScan;

class ScanCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    Serial.println("Device found:");
    Serial.print("  Address: ");
    Serial.println(device.getAddress().toString().c_str());
    
    if(device.haveName()) {
      Serial.print("  Name: ");
      Serial.println(device.getName().c_str());
      
      // Check for XR-2 related names
      String name = device.getName().c_str();
      if (name.indexOf("XR") >= 0 || 
          name.indexOf("CHIGEE") >= 0 ||
          name.indexOf("chigee") >= 0) {
        Serial.println("  *** POTENTIAL XR-2 DEVICE! ***");
      }
    }
    
    if(device.haveServiceUUID()) {
      Serial.print("  Service UUID: ");
      Serial.println(device.getServiceUUID().toString().c_str());
    }
    
    Serial.print("  RSSI: ");
    Serial.println(device.getRSSI());
    Serial.println("  ---------");
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("XR-2 BLE Scanner Starting...");
  
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  Serial.println("\nScanning for BLE devices...");
  BLEScanResults results = pBLEScan->start(5, false);
  Serial.print("Total found: ");
  Serial.println(results.getCount());
  Serial.println("========================");
  delay(3000);
}