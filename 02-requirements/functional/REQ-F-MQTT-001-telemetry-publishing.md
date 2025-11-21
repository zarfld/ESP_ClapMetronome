# REQ-F-MQTT-001: MQTT Telemetry Publishing

**Status**: Draft  
**Priority**: P2 (Medium)  
**Phase**: 02-requirements  
**Created**: 2025-11-21

---

## Requirement Statement

The system **SHALL** publish telemetry data to an MQTT broker at configurable intervals, including:
- Current BPM (beats per minute) with confidence level
- Lock status (LOCKED/UNLOCKING/UNLOCKED)
- Audio signal levels (current/min/max)
- System status (uptime, WiFi signal strength, free heap)
- Configuration changes

---

## Acceptance Criteria

**AC-MQTT-001**: System connects to MQTT broker on startup
- **Given** valid MQTT broker configuration (host, port, credentials)
- **When** system initializes network connectivity
- **Then** MQTT client establishes connection within 10 seconds
- **And** publishes `<device-id>/status/online` with payload `{"online":true}`

**AC-MQTT-002**: System publishes BPM telemetry
- **Given** MQTT client is connected
- **When** BPM value changes by ≥1.0 BPM OR 5 seconds elapsed since last publish
- **Then** system publishes to topic `<device-id>/telemetry/bpm`
- **And** payload contains: `{"bpm": 120.5, "confidence": 95, "lock_status": "LOCKED", "timestamp": 1700000000}`

**AC-MQTT-003**: System publishes audio signal levels
- **Given** MQTT client is connected
- **When** 5 seconds elapsed since last publish
- **Then** system publishes to topic `<device-id>/telemetry/audio`
- **And** payload contains: `{"current_level": 420, "max_level": 680, "min_level": 45, "threshold": 120, "gain": 1}`

**AC-MQTT-004**: System publishes system status
- **Given** MQTT client is connected
- **When** 60 seconds elapsed since last publish
- **Then** system publishes to topic `<device-id>/status/system`
- **And** payload contains: `{"uptime_sec": 7200, "free_heap": 180000, "wifi_rssi": -45, "wifi_ssid": "MyNetwork"}`

**AC-MQTT-005**: System publishes Last Will Testament (LWT)
- **Given** MQTT connection established
- **When** system unexpectedly disconnects (crash, power loss)
- **Then** broker automatically publishes to `<device-id>/status/online` with payload `{"online":false}`

**AC-MQTT-006**: System handles connection loss gracefully
- **Given** MQTT client was connected
- **When** broker becomes unreachable
- **Then** system continues normal operation (audio detection, web server)
- **And** attempts reconnection every 30 seconds
- **And** resumes publishing when reconnected

---

## Rationale

**Business Value**:
- Remote monitoring of multiple devices from central dashboard
- Historical BPM analysis (when paired with InfluxDB/Grafana)
- Integration with home automation systems (Home Assistant, Node-RED)
- Proactive alerting (e.g., BPM out of expected range)

**Technical Justification**:
- MQTT is lightweight (small packet overhead vs HTTP REST)
- Publish/subscribe model scales to many subscribers
- QoS levels ensure reliable delivery
- Standard protocol supported by all IoT platforms

---

## Constraints

- **Library**: Use `256dpi/MQTT @ ^2.5.2` (already in platformio.ini)
- **QoS**: QoS 0 (at most once) for telemetry (performance over reliability)
- **QoS**: QoS 1 (at least once) for status messages (reliability)
- **Payload**: JSON format (compact, human-readable)
- **Size**: Total payload <512 bytes (MQTT packet size limit)
- **Frequency**: Configurable via ConfigurationManager (default 5s for telemetry, 60s for status)
- **Non-blocking**: Must not block main loop >10ms

---

## Topic Structure

```
<device-id>/telemetry/bpm       # BPM and confidence
<device-id>/telemetry/audio     # Audio signal levels
<device-id>/status/system       # System health
<device-id>/status/online       # Online/offline status (LWT)
<device-id>/config/current      # Current configuration snapshot
```

**device-id**: Configurable unique identifier (default: `esp32-<chip-id>`)

---

## Configuration Parameters

Stored in `ConfigurationManager`:

```cpp
struct MQTTConfig {
    bool enabled;                    // Default: false
    String broker_host;              // Default: ""
    uint16_t broker_port;            // Default: 1883
    String username;                 // Default: ""
    String password;                 // Default: ""
    String device_id;                // Default: "esp32-<chip-id>"
    uint16_t telemetry_interval_ms;  // Default: 5000 (5s)
    uint16_t status_interval_ms;     // Default: 60000 (60s)
    uint8_t qos;                     // Default: 0
    bool use_tls;                    // Default: false (future)
};
```

---

## Non-Functional Requirements

| Attribute | Target | Verification |
|-----------|--------|--------------|
| **Latency** | Publish completes in <50ms | Measure `millis()` before/after publish() |
| **Reliability** | 99% message delivery (QoS 1 for status) | Test with broker downtime simulation |
| **Memory** | <10 KB RAM overhead | Measure heap before/after MQTT init |
| **CPU** | <5ms per publish | Profile with `xthal_get_ccount()` |
| **Reconnect** | Reconnect within 30s after disconnect | Test with broker restart |

---

## Dependencies

**Upstream (Required by this requirement)**:
- #REQ-F-004 (WiFi Connectivity) - MQTT requires network
- #REQ-F-002 (BPM Detection) - Source of telemetry data
- #ARC-C-006 (Configuration Manager) - Stores MQTT config

**Downstream (Depends on this requirement)**:
- None (MQTT is optional add-on)

---

## Test Strategy

**Unit Tests** (test_mqtt_client/):
- Connection establishment (mock broker)
- Publish message formatting (JSON validation)
- QoS level handling
- Error handling (broker unreachable)
- Reconnection logic

**Integration Tests** (test_mqtt_integration/):
- Connect to real MQTT broker (Mosquitto on localhost)
- Publish BPM telemetry every 5s
- Verify topic structure and payload format
- Simulate broker disconnect/reconnect
- Measure publish latency (<50ms)

**Manual Tests**:
- Subscribe to topics with `mosquitto_sub`
- Monitor telemetry stream in MQTT Explorer
- Test with Home Assistant MQTT integration

---

## Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| MQTT publish blocks main loop | Medium | High | Use non-blocking API, timeout after 50ms |
| Broker unreachable causes crash | Low | High | Graceful error handling, continue without MQTT |
| Credentials stored in plaintext | High | Medium | Warn users, recommend secure broker (future: TLS) |
| Memory exhaustion on ESP8266 | Medium | Medium | Disable MQTT on ESP8266 if RAM <30% |

---

## Future Enhancements

- REQ-F-MQTT-002: Subscribe to control topics (remote configuration)
- REQ-F-MQTT-003: TLS/SSL encryption
- REQ-F-MQTT-004: MQTT v5 support
- REQ-F-MQTT-005: OTA firmware updates via MQTT

---

## Traceability

**Satisfies Stakeholder Requirements**:
- #STR-002 (Remote Monitoring) - MQTT enables centralized monitoring

**Implements Architecture Components**:
- #ARC-C-007 (MQTT Telemetry Client) - Primary implementation

**Verified by Tests**:
- #TEST-MQTT-001 (Connection test)
- #TEST-MQTT-002 (Publish test)
- #TEST-MQTT-003 (Reconnection test)

**Referenced in Design**:
- `04-design/components/MQTTClient.md` (detailed design)
- `05-implementation/tdd-plan-mqtt.md` (TDD cycles)

---

**Author**: GitHub Copilot  
**Reviewed by**: [Pending]  
**Approved by**: [Pending]
