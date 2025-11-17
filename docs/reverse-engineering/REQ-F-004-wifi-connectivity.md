# REQ-F-004: WiFi Connectivity Management

**Type**: Functional Requirement  
**Priority**: P1 (High)  
**Status**: Draft - Reverse Engineered  
**Source**: `ESP_ClapMetronome.ino` lines 10-16, 24-28, 169-171

---

## Description

System shall establish and maintain WiFi connectivity to enable MQTT communication with remote monitoring services.

---

## Functional Requirements

### REQ-F-004.1: WiFi Configuration

System shall support WiFi configuration:

- **SSID**: Configurable network name
- **Password**: WPA/WPA2 authentication
- **Mode**: Station + Access Point (WIFI_AP_STA)
- **Platform support**: ESP8266 and ESP32

### REQ-F-004.2: Connection Establishment

System shall connect to WiFi during initialization:

- **Timing**: During `setup()` before MQTT initialization
- **Blocking**: Non-blocking connection attempt
- **Retry**: Implementation-dependent (not explicitly defined in code)

### REQ-F-004.3: Platform Abstraction

System shall abstract WiFi API differences:

```cpp
#if defined(ESP8266)
   #include <ESP8266WiFi.h>
   #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
#endif
```

- **ESP8266**: Uses ESP8266WiFi library
- **ESP32**: Uses standard WiFi library
- **Conditional compilation**: Automatic platform detection

---

## Acceptance Criteria

```gherkin
Scenario: Successful WiFi connection (ESP32)
  Given ESP32 device is powered on
  And WiFi credentials are configured
  When setup() executes WiFi.begin()
  Then device connects to configured SSID
  And IP address is assigned via DHCP
  And MQTT can establish connection

Scenario: Successful WiFi connection (ESP8266)
  Given ESP8266 device is powered on
  And WiFi mode is WIFI_AP_STA
  When setup() executes WiFi.begin()
  Then device connects to configured SSID
  And can function as station and AP simultaneously

Scenario: WiFi connection failure
  Given WiFi credentials are incorrect
  Or network is unavailable
  When WiFi.begin() is called
  Then system continues execution (no blocking)
  And MQTT operations will fail until connected
```

---

## Current Implementation Limitations

⚠️ **Identified issues in current code:**

1. **No connection status check**: Code doesn't verify WiFi connected before MQTT
2. **No reconnection logic**: If WiFi drops, no automatic reconnect
3. **Hardcoded credentials**: SSID and password in source code
4. **No connection timeout**: Could block indefinitely
5. **No status feedback**: LED or serial output for connection state

---

## Implementation Details

**Code Location**: `ESP_ClapMetronome.ino`

```cpp
/**
 * Implements: #N (REQ-F-004: WiFi Connectivity Management)
 * 
 * Platform detection: lines 10-16
 * Configuration: lines 24-28
 * Connection: lines 169-171
 */

// Platform-specific includes (lines 10-16)
#if defined(ESP8266)
   #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

// WiFi configuration (lines 25-27)
const char* ssid = "yourSSID";
const char* password = "yourPassword";

// Connection establishment (lines 169-171)
WiFi.mode(WIFI_AP_STA);
WiFi.begin(ssid, password);
```

---

## Validation Required

⚠️ **Reverse-engineered - Needs validation:**

- [ ] Verify WIFI_AP_STA mode is required (why AP mode?)
- [ ] Confirm no reconnection logic is intentional
- [ ] Test connection stability over extended operation
- [ ] Validate behavior on WiFi dropout
- [ ] Check if watchdog timer handles WiFi hangs
- [ ] Consider adding WiFi status LED indicator
- [ ] Evaluate need for WiFiManager or similar config library

---

## Security Considerations

⚠️ **Critical security issue:**

Hardcoded credentials in source code:

```cpp
const char* ssid = "yourSSID";
const char* password = "yourPassword";
```

**Recommendations**:

1. Move to `config.h` (git-ignored)
2. Use EEPROM/Flash storage
3. Implement WiFiManager for web-based configuration
4. Use WPS or SmartConfig for initial setup

---

## Traces To

- **Parent**: #N (StR-001: Clap-Based Metronome System)
- **Required by**: #N (REQ-F-003: MQTT Telemetry)
- **Verified by**: TEST-F-004 (to be created)
- **Architecture**: ADR-COMM-002 (WiFi Configuration Strategy)
- **Related**: REQ-NF-003 (Network Reliability - to be created)

---

## Create GitHub Issue

```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p1" \
  --title "REQ-F-004: WiFi Connectivity Management" \
  --body-file REQ-F-004-wifi-connectivity.md
```
