# TDD Cycle OUT-02: RED Phase - RTP-MIDI Network Transport

**Wave**: 3.2 (Output Controller)  
**Cycle**: OUT-02 (Network Transport Layer)  
**Phase**: RED (Write Failing Tests)  
**Date**: 2025-01-18  
**Status**: 🔴 RED Phase In Progress

## Objective

Implement UDP network transport for RTP-MIDI protocol (RFC 6295).

## Acceptance Criteria

- **AC-OUT-003**: RTP-MIDI network transport (UDP over WiFi)
- **AC-OUT-008**: Network latency handling (<10ms compensation)

## Test Plan (12 tests)

### 1. UDP Socket Creation
- Test UDP socket opens on port 5004 (control)
- Test UDP socket opens on port 5005 (data)
- Test socket creation failure handling

### 2. RTP-MIDI Packet Format (RFC 6295)
- Test RTP header format (V=2, PT=97)
- Test MIDI command section encoding
- Test single-byte System Real-Time encapsulation
- Test timestamp field (microseconds)

### 3. Network Transmission
- Test send single 0xF8 message
- Test send START/STOP messages
- Test packet size (minimal overhead)

### 4. Error Handling
- Test network unavailable handling
- Test send timeout
- Test buffer overflow protection

## Implementation Scope

**Files to modify**:
- `src/output/OutputController.cpp` - Replace stub `sendMIDIRealTime()`
- `test/test_output/test_network_transport.cpp` - 12 new tests

**Dependencies**:
- WiFi library (ESP32/ESP8266)
- UDP socket API
- RTP-MIDI packet builder

## Expected Failures

All 12 tests will fail because `sendMIDIRealTime()` currently does nothing.

---

**Next**: Implement tests and document RED phase completion
