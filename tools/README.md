# Traffic Analysis Tools

This directory contains tools for capturing and analyzing Bluetooth traffic between Chigee XR2 and genuine OBD2 devices to reverse engineer authentication and security protocols.

## Tools Overview

### 1. Traffic Analyzer (`traffic_analyzer.py`)
**Purpose**: Analyze captured Bluetooth packets to identify security mechanisms and device fingerprints.

**Features**:
- Parse Wireshark JSON exports and Android HCI logs
- Extract AT command sequences and timing patterns
- Detect authentication mechanisms and security events
- Generate device fingerprints for comparison
- Provide bypass recommendations based on analysis

**Usage**:
```bash
# Analyze Wireshark capture
python3 traffic_analyzer.py capture.json --output analysis_report.json

# Compare two devices
python3 traffic_analyzer.py capture.json --compare AA:BB:CC:DD:EE:FF 11:22:33:44:55:66

# Analyze Android HCI log
python3 traffic_analyzer.py hci_log.bin --format android
```

### 2. Capture Helper (`capture_helper.py`)
**Purpose**: Cross-platform tool for setting up Bluetooth packet captures.

**Features**:
- Linux: Automated capture using btmon/hcidump
- Windows/macOS: Manual capture instructions
- Device discovery and MAC address identification
- Format conversion (PCAP to JSON)
- Timed captures with automatic stop

**Usage**:
```bash
# Scan for Bluetooth devices
python3 capture_helper.py --scan

# Start capture targeting specific device
python3 capture_helper.py --bluetooth --target AA:BB:CC:DD:EE:FF --output chigee_capture.pcap

# Convert existing capture to JSON
python3 capture_helper.py --convert capture.pcap
```

### 3. Compatibility Tester (`compatibility_tester.py`)
**Purpose**: Test ESP32 implementation against Chigee XR2 compatibility requirements.

**Features**:
- Basic AT command functionality testing
- Device fingerprinting resistance validation
- Security bypass mechanism verification
- OBD PID simulation accuracy assessment
- Overall compatibility scoring

**Usage**:
```bash
# Test via Bluetooth
python3 compatibility_tester.py --bluetooth AA:BB:CC:DD:EE:FF --output test_results.json

# Quick test via Serial (development)
python3 compatibility_tester.py --serial COM3 --quick

# Full test suite
python3 compatibility_tester.py --bluetooth AA:BB:CC:DD:EE:FF
```

## Workflow

### Phase 1: Data Collection
1. **Set up capture environment**:
   ```bash
   python3 capture_helper.py --scan
   ```

2. **Capture baseline traffic** (genuine OBD2 device + Chigee XR2):
   ```bash
   python3 capture_helper.py --bluetooth --target CHIGEE_MAC --output baseline.pcap
   ```

3. **Capture test device traffic** (our ESP32 + Chigee XR2):
   ```bash
   python3 capture_helper.py --bluetooth --target ESP32_MAC --output test.pcap
   ```

### Phase 2: Analysis
1. **Convert captures to JSON**:
   ```bash
   python3 capture_helper.py --convert baseline.pcap
   python3 capture_helper.py --convert test.pcap
   ```

2. **Analyze security mechanisms**:
   ```bash
   python3 traffic_analyzer.py baseline.json --output baseline_analysis.json
   python3 traffic_analyzer.py test.json --output test_analysis.json
   ```

3. **Compare device behaviors**:
   ```bash
   python3 traffic_analyzer.py baseline.json --compare GENUINE_MAC ESP32_MAC
   ```

### Phase 3: Validation
1. **Test current implementation**:
   ```bash
   python3 compatibility_tester.py --bluetooth ESP32_MAC --output compatibility.json
   ```

2. **Iterate based on findings**:
   - Update ESP32 code based on analysis recommendations
   - Re-test compatibility
   - Repeat until compatibility score > 80%

## Data Files

### Device Fingerprints Database (`../data/device_fingerprints.json`)
Contains known working OBD2 device characteristics:
- Bluetooth advertising data
- AT command response patterns
- Timing characteristics
- Error behaviors
- Security features

### Analysis Methodology (`../docs/TRAFFIC_ANALYSIS_METHODOLOGY.md`)
Comprehensive guide for:
- Hardware setup requirements
- Capture protocols and procedures
- Analysis frameworks and techniques
- Security bypass development
- Testing and validation methods

## Platform-Specific Setup

### Linux
```bash
# Install required tools
sudo apt-get install bluez-tools bluez-hcidump wireshark-common

# Give permissions for Bluetooth capture
sudo usermod -a -G wireshark $USER
# Log out and back in, or run with sudo
```

### Windows
1. Install Wireshark with Bluetooth support
2. Use external Bluetooth sniffer (Ubertooth One recommended)
3. Alternative: Use Android device with HCI snoop logging

### macOS
1. Install Xcode Additional Tools for PacketLogger
2. Use external Bluetooth sniffer
3. Alternative: Use Wireshark with USB Bluetooth adapter

## Security Considerations

⚠️ **Important**: These tools are for legitimate security research and device interoperability only.

- Only test with your own equipment
- Follow local laws regarding reverse engineering
- Consider responsible disclosure for security vulnerabilities
- Ensure motorcycle safety systems remain functional

## Expected Outputs

### Traffic Analysis Report
```json
{
  "analysis_date": "2025-09-14T...",
  "packet_count": 1234,
  "device_fingerprints": {...},
  "security_events": [...],
  "bypass_recommendations": [...],
  "statistics": {...}
}
```

### Compatibility Test Results
```json
{
  "test_timestamp": "2025-09-14T...",
  "compatibility_assessment": {
    "overall_score": 85.2,
    "chigee_ready": true,
    "basic_functionality": 90.0,
    "timing_realistic": true,
    "security_bypass": true,
    "simulation_quality": 88.5
  }
}
```

## Troubleshooting

### Common Issues

1. **"Permission denied" on Linux**:
   ```bash
   sudo python3 capture_helper.py ...
   ```

2. **"No Bluetooth adapter found"**:
   - Check if Bluetooth is enabled
   - Verify adapter compatibility
   - Try external USB Bluetooth adapter

3. **"Connection timeout"**:
   - Verify target device MAC address
   - Check if device is in pairing mode
   - Ensure devices are within range

4. **"Invalid capture format"**:
   - Use tshark to convert: `tshark -r input.pcap -T json > output.json`
   - Verify Wireshark installation

### Getting Help

1. Check tool help messages: `python3 tool_name.py --help`
2. Review methodology documentation
3. Verify hardware setup and permissions
4. Check tool output for specific error messages

---

**Note**: This toolkit is designed for security researchers and developers working on legitimate device interoperability. Always follow ethical guidelines and applicable laws when conducting security research.