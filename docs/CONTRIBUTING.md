# Contributing to Chigee XR2 OBD2 Module

Thank you for your interest in contributing to this project! This guide will help you get started with contributing safely and effectively.

## ??? Safety First

**CRITICAL: This project interfaces with vehicle electronics. All contributions must prioritize safety.**

- All code changes must be tested in **isolation mode** first
- Never submit code that bypasses safety checks
- Document any changes that affect vehicle interaction
- Include safety warnings in pull requests when appropriate

## ?? Getting Started

### Prerequisites
- Arduino IDE with ESP32 support
- Basic understanding of CAN bus protocols
- Git and GitHub account
- Hardware for testing (ESP32 + MCP2515)

### Development Environment Setup
1. Fork the repository
2. Clone your fork locally
3. Install required Arduino libraries:
   ```
   - BluetoothSerial (built-in)
   - SPI (built-in) 
   - mcp_can by Coryjfowler
   ```
4. Verify compilation with `TEST_MODE = true`

## ?? Types of Contributions

### ?? Bug Reports
- Use the GitHub Issues template
- Include complete error logs
- Specify hardware configuration
- Provide steps to reproduce
- Test in isolation mode first

### ?? Feature Requests  
- Check existing issues first
- Describe the use case clearly
- Consider safety implications
- Propose implementation approach

### ?? Code Contributions
- Bug fixes
- New OBD2 PID support
- CAN database improvements
- Documentation updates
- Test coverage improvements

### ?? Documentation
- Installation guides
- Troubleshooting help
- Hardware configuration guides
- Translation to other languages

## ?? Development Workflow

### 1. Issue Discussion
- Create or comment on relevant issue
- Discuss approach before coding
- Get feedback from maintainers

### 2. Development
```bash
# Create feature branch
git checkout -b feature/your-feature-name

# Make changes with safety in mind
# Always test in isolation mode first

# Commit with descriptive messages
git commit -m "feat: add support for PID 0x15 intake manifold pressure

- Implement PID 0x15 handler in handleOBDCommand()
- Add simulation data for intake pressure
- Update documentation with new PID
- Tested in isolation mode with Chigee XR2

Safety: No vehicle connection required for this change"
```

### 3. Testing Requirements
All contributions must include appropriate testing:

- **Compilation Test**: Code must compile without errors
- **Isolation Mode Test**: Functionality tested with `TEST_MODE = true`
- **Unit Tests**: Add tests for new functions where applicable
- **Documentation Test**: Verify all examples work as documented

### 4. Pull Request Process
1. **Update Documentation**: Include any necessary docs updates
2. **Test Report**: Provide evidence of testing
3. **Safety Review**: Highlight any safety considerations
4. **Phase Impact**: Note which project phases are affected

## ?? Code Standards

### C++ Style Guidelines
- Use descriptive variable names
- Comment complex CAN parsing logic
- Follow Arduino naming conventions
- Include safety checks in vehicle interface code

### Specific Requirements
```cpp
// Good: Clear safety check
if (!TEST_MODE && !performSafetyChecks()) {
    Serial.println("SAFETY: Aborting CAN operation");
    return false;
}

// Good: Descriptive CAN parsing
// Parse RPM from KTM Duke 390 CAN frame 0x280
// Bytes 0-1: Raw RPM value (no division needed)
uint16_t rawRpm = (canData[0] << 8) | canData[1];

// Bad: No safety consideration
CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
```

### Documentation Standards
- Include safety warnings where appropriate
- Provide working code examples
- Document CAN ID sources and confidence levels
- Update phase management documentation

## ?? Testing Guidelines

### Required Tests
1. **Compilation**: Must compile with no errors/warnings
2. **Isolation Mode**: All features work with `TEST_MODE = true`
3. **Memory Usage**: No memory leaks or excessive usage
4. **Bluetooth**: Compatible with ELM327 expectations

### Hardware Testing
- Use development hardware first
- Never test on vehicle without isolation validation
- Document test configuration and results
- Include oscilloscope captures for CAN changes

### Test Documentation
```cpp
/*
 * TEST REPORT
 * Feature: Added PID 0x15 support
 * Hardware: ESP32-DevKitC + MCP2515
 * Test Mode: ISOLATION (TEST_MODE = true)
 * Bluetooth Client: ELM327 Terminal
 * Result: PASS - Correct response format
 * Safety: No vehicle connection required
 */
```

## ?? CAN Database Contributions

### Adding New CAN IDs
When contributing CAN database information:

1. **Source Documentation**: Cite source of CAN ID information
2. **Confidence Level**: Rate confidence (LOW/MEDIUM/HIGH)
3. **Verification**: Include testing methodology
4. **Multiple Sources**: Prefer multiple confirmations

Example:
```cpp
// CAN ID 0x2C5 - Fuel Level (MEDIUM confidence)
// Source: KTM Duke 390 forum post by user XYZ
// Verification: Tested on 2020 Svartpilen 401
// Notes: May vary by model year
```

## ?? Phase Management

### Project Phase Impact
When contributing, consider which phases are affected:

- **Phase 2 (Testing)**: Simulation improvements, test coverage
- **Phase 3 (CAN Integration)**: CAN parsing, vehicle protocols  
- **Phase 4 (Chigee Pairing)**: Bluetooth, OBD2 compatibility
- **Phase 5+**: Real-world testing, optimization

Update phase documentation when:
- Adding new requirements
- Changing validation criteria
- Affecting phase progression

## ?? Review Process

### What Reviewers Look For
1. **Safety**: No bypass of safety systems
2. **Testing**: Adequate test coverage
3. **Documentation**: Clear and complete
4. **Compatibility**: Works with existing features
5. **Code Quality**: Readable and maintainable

### Review Timeline
- Initial response: Within 48 hours
- Full review: Within 1 week
- Complex features: May require multiple review cycles

## ?? Security Considerations

### Vehicle Security
- Never include real vehicle VIN or personal info
- Be cautious about publishing CAN captures
- Consider privacy implications of data logging
- Respect reverse engineering ethical guidelines

### Code Security  
- No hardcoded passwords or keys
- Validate all inputs from Bluetooth
- Implement proper error handling
- Use secure coding practices

## ?? Getting Help

### Before Contributing
- Read all documentation in `docs/` folder
- Check existing issues and discussions
- Test your development environment
- Understand the phase management system

### Communication Channels
- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions and general discussion
- **Pull Request Comments**: Code-specific discussions

### Response Expectations
- Issues: Response within 48 hours
- Pull Requests: Initial review within 1 week
- Discussions: Best effort, community-driven

## ?? Contribution Ideas

### Good First Contributions
- Documentation improvements
- Additional PID support (with simulation)
- Test coverage improvements
- Code comments and cleanup

### Advanced Contributions
- New vehicle model support
- Performance optimizations
- Advanced diagnostics features
- PCB design improvements

### Long-term Projects
- Mobile app development
- Web-based configuration tool
- Advanced CAN reverse engineering tools
- Community testing framework

## ?? Code of Conduct

### Our Standards
- Safety-first mindset in all contributions
- Respectful and constructive communication
- Focus on technical merit and safety
- Help newcomers learn safely

### Unacceptable Behavior
- Promoting unsafe practices
- Sharing unverified CAN data as confirmed
- Bypassing or removing safety features
- Disrespectful communication

## ?? Recognition

Contributors will be recognized in:
- README.md acknowledgments
- Release notes for significant contributions
- Documentation credits
- Project history

Thank you for helping make this project safer and better for the motorcycle community!

---

**Remember: Safety first, test thoroughly, document clearly, and have fun! ???**
