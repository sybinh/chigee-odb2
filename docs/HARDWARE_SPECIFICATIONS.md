# XR-2 Hardware Specifications

## Recently Discovered Information

### Part Numbers
- **MFP0120** / **MFP0121** ✅ **CONFIRMED OFFICIAL**
  - Official Chigee OBD module model numbers  
  - Found on Chigee website - legitimate commercial products
  - MFP0120/MFP0121 likely different variants or generations
  - Our reverse engineering targets real commercial hardware

### Physical Interface
- **Connector**: JST-04T-JWPF-VSLE
  - JST 4-pin connector
  - Waterproof/sealed variant (VSLE suffix)
  - Compact form factor suitable for motorcycle applications

### Communication Protocol
- **BLE 5.0**
  - Latest Bluetooth Low Energy standard
  - Enhanced range and reliability
  - Support for multiple simultaneous connections
  - Better power efficiency

## Implications for Our Project

### BLE 5.0 Features
1. **Extended Range**: Up to 4x range improvement over BLE 4.2
2. **Higher Speed**: Up to 2x faster data transmission
3. **Multiple Connections**: Can handle multiple peripheral connections
4. **Enhanced Security**: Improved encryption and authentication

### Physical Connector Analysis
The JST-04T-JWPF-VSLE suggests:
- **4-pin configuration**: Likely VCC, GND, and 2 data lines
- **Waterproof design**: Suitable for motorcycle environment
- **Professional connector**: Not a hobbyist solution

### Protocol Implications
BLE 5.0 support means:
- Our ESP32 implementation should leverage BLE 5.0 features
- Multiple device connections possible (XR-2 + phone app?)
- Enhanced reliability for motorcycle vibration environment

## Research Questions

1. **Are MFP0120/MFP0121 official Chigee products?**
2. **What's the pinout of the JST connector?**
3. **Does XR-2 use extended BLE 5.0 features?**
4. **Can XR-2 handle multiple BLE connections simultaneously?**

## Next Steps

1. **Update ESP32 code** to use BLE 5.0 features
2. **Research connector pinout** for potential direct interface
3. **Test multiple connection scenarios**
4. **Optimize for BLE 5.0 enhanced features**

---
*Information discovered: September 21, 2025*