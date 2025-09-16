# XR-2 vs XR-5 Lite/nAdvanced - Architecture Analysis

## Key Discovery
**CGOBD module chỉ hỗ trợ XR-5 Lite/nAdvanced, KHÔNG hỗ trợ XR-2!**

## Phân tích sự khác biệt:

### XR-2 Architecture:
- **4.3" display unit** 
- **Dual dashcam functionality**
- **Screen mirroring system**
- **Bluetooth name**: CHIGEE-6697
- **MAC**: 34:76:5a:02:c0:33
- **🚫 KHÔNG hỗ trợ external OBD modules**

### XR-5 Lite/nAdvanced Architecture:
- **Larger display systems** (likely 5" or more)
- **✅ HỖ TRỢ CGOBD modules**
- **Different Bluetooth protocol/pairing**
- **Different device naming convention**
- **Built-in OBD integration support**

---

## Điều này giải thích tại sao:

1. **XR-2 không scan được CGOBD-5F72** 
   - XR-2 firmware không có OBD module support
   
2. **Connection failures trong testing**
   - XR-2 không expect external OBD connections
   
3. **No OBD menu/settings trên XR-2**
   - XR-2 chỉ focus vào dashcam + screen mirroring

---

## XR-2 OBD Options:

### Option 1: Phone App Integration
- XR-2 connect với phone qua app
- Phone đọc OBD data từ ELM327/OBD adapter
- Phone gửi data lên XR-2 qua app protocol

### Option 2: Screen Mirroring Approach  
- Dùng Android phone với OBD app
- Mirror phone screen lên XR-2
- XR-2 chỉ là external display

### Option 3: Custom Firmware Mod
- Reverse engineer XR-2 firmware
- Add OBD module support
- Flash modified firmware (risky!)

### Option 4: Web Interface Approach
- XR-2 browser mở web interface
- Web page nhận OBD data từ ESP32/server
- Display data qua web UI

---

## Recommended Approach for XR-2:

### 🎯 Phone App Protocol Analysis
1. **Analyze Chigee mobile app**
2. **Reverse engineer XR-2 ↔ Phone communication**
3. **Create custom OBD data sender** 
4. **Integrate với existing app protocol**

### Technical Steps:
1. **Install Chigee mobile app**
2. **Sniff WiFi/Bluetooth traffic** giữa phone và XR-2
3. **Analyze data protocol/format**
4. **Create ESP32 OBD bridge** gửi data theo format này
5. **XR-2 receive OBD data** như từ phone app

---

## Next Actions:

1. **Download Chigee mobile app**
2. **Setup traffic monitoring** (Wireshark/packet capture)
3. **Analyze phone ↔ XR-2 communication protocol**
4. **Reverse engineer data format**
5. **Implement ESP32 phone app emulator**

---

## Questions to Research:

1. **Chigee app có OBD features không?**
2. **XR-2 có nhận data từ 3rd party apps không?**
3. **XR-2 web browser có limitations không?**
4. **Có cách nào mod XR-2 firmware không?**