/*
 * XR-2 Bluetooth Device Scanner
 * Scan for XR-2 and analyze its Bluetooth advertising/pairing behavior
 */

#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BluetoothSerial SerialBT;

class XR2ScanCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String deviceName = advertisedDevice.getName().c_str();
    String deviceAddr = advertisedDevice.getAddress().toString().c_str();
    
    Serial.println("=== BLE Device Found ===");
    Serial.println("Name: " + deviceName);
    Serial.println("Address: " + deviceAddr);
    Serial.println("RSSI: " + String(advertisedDevice.getRSSI()));
    
    // Look for XR-2 related names
    if (deviceName.indexOf("XR") >= 0 || 
        deviceName.indexOf("CHIGEE") >= 0 ||
        deviceName.indexOf("chigee") >= 0) {
      Serial.println("*** POTENTIAL XR-2 DEVICE FOUND! ***");
      
      // Get more details
      if (advertisedDevice.haveServiceUUID()) {
        Serial.print("Service UUID: ");
        Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
      }
      
      if (advertisedDevice.haveManufacturerData()) {
        std::string mfData = advertisedDevice.getManufacturerData();
        Serial.print("Manufacturer Data: ");
        for (int i = 0; i < mfData.length(); i++) {
          Serial.printf("%02X ", (uint8_t)mfData[i]);
        }
        Serial.println();
      }
    }
    Serial.println("========================");
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("XR-2 Bluetooth Scanner Starting...");
  
  // Initialize BLE
  BLEDevice::init("ESP32_XR2_Scanner");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new XR2ScanCallback());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  
  // Initialize Classic Bluetooth
  if (!SerialBT.begin("ESP32_XR2_Scanner")) {
    Serial.println("Bluetooth Classic init failed!");
    return;
  }
  
  Serial.println("=== Starting XR-2 Discovery ===");
  Serial.println("1. Turn on your XR-2 device");
  Serial.println("2. Put XR-2 in pairing mode");
  Serial.println("3. Wait for device discovery...");
  Serial.println("================================");
}

void loop() {
  // BLE Scan
  Serial.println("\n--- BLE Scan ---");
  BLEScan* pBLEScan = BLEDevice::getScan();
  BLEScanResults foundDevices = pBLEScan->start(5, false);
  Serial.printf("Found %d BLE devices\n", foundDevices.getCount());
  pBLEScan->clearResults();
  
  // Classic Bluetooth Discovery
  Serial.println("\n--- Classic BT Scan ---");
  BTScanResults* btScanResults = SerialBT.discover(10000);
  if (btScanResults) {
    for (int i = 0; i < btScanResults->getCount(); i++) {
      BTAdvertisedDevice* device = btScanResults->getDevice(i);
      String deviceName = device->getName().c_str();
      String deviceAddr = device->getAddress().toString().c_str();
      
      Serial.println("=== Classic BT Device ===");
      Serial.println("Name: " + deviceName);
      Serial.println("Address: " + deviceAddr);
      Serial.println("RSSI: " + String(device->getRSSI()));
      
      // Look for XR-2
      if (deviceName.indexOf("XR") >= 0 || 
          deviceName.indexOf("CHIGEE") >= 0) {
        Serial.println("*** POTENTIAL XR-2 CLASSIC BT! ***");
      }
      Serial.println("=========================");
    }
  }
  
  delay(10000); // Wait 10 seconds between scans
}