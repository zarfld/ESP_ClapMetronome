# REQ-F-003: MQTT Telemetry Publishing

**Type**: Functional Requirement  
**Priority**: P1 (High)  
**Status**: Draft - Reverse Engineered  
**Source**: `ESP_ClapMetronome.ino` lines 24-28, 173, 278, 337, 393-395, 401

---

## Description

System shall publish telemetry data to MQTT broker for remote monitoring of tempo, detection threshold, gain levels, and signal statistics.

---

## Functional Requirements

### REQ-F-003.1: MQTT Connection Configuration

System shall connect to MQTT broker:

- **Broker host**: Configurable IP address (default: 157.247.1.102)
- **Protocol**: MQTT (standard port)
- **Client**: WiFiClient + MQTTClient library
- **Connection**: Established during setup, maintained during operation

### REQ-F-003.2: Tempo Publishing

System shall publish BPM changes:

- **Topic**: `/Proberaum/Metronome/Tempo`
- **Payload**: Integer BPM value as string
- **Trigger**: When BPM changes outside tolerance (±5%)
- **Frequency**: On-change only

### REQ-F-003.3: Threshold Telemetry

System shall publish adaptive threshold updates:

- **Topic**: `/Proberaum/Metronome/threshold`
- **Payload**: Integer threshold value (ADC units)
- **Trigger**: After each clap detection and threshold recalculation
- **Frequency**: On clap detection

### REQ-F-003.4: Signal Statistics Publishing

System shall publish min/max audio levels:

- **Topics**: 
  - `/Proberaum/Metronome/Max`
  - `/Proberaum/Metronome/Min`
- **Payload**: Integer ADC average values
- **Trigger**: With threshold updates (on clap)
- **Purpose**: Monitor audio signal quality

### REQ-F-003.5: Gain Control Telemetry

System shall publish gain level changes:

- **Topic**: `/Proberaum/Metronome/Gain` (also `/Proberaum/Metronome/gain`)
- **Payload**: Integer gain level (0-2)
- **Trigger**: When automatic gain control adjusts gain
- **Frequency**: On gain change only

---

## Acceptance Criteria

```gherkin
Scenario: Publish tempo change
  Given system detects new BPM of 135
  And previous BPM was 120
  When tempo update occurs
  Then MQTT message published to "/Proberaum/Metronome/Tempo"
  And payload is "135"
  And message is retained on broker

Scenario: Publish detection threshold
  Given clap detected with adaptive threshold update
  When threshold recalculated to 87
  Then MQTT publishes to "/Proberaum/Metronome/threshold"
  And payload is "87"
  And Max/Min statistics also published

Scenario: Publish gain adjustment
  Given current gain is 2
  When audio signal clips (>1000 ADC)
  Then gain reduced to 1
  And MQTT publishes to "/Proberaum/Metronome/Gain"
  And payload is "1"
```

---

## MQTT Topic Structure

| Topic | Payload Type | Update Frequency | Purpose |
|-------|-------------|------------------|---------|
| `/Proberaum/Metronome/Tempo` | Integer (BPM) | On change | Monitor tempo |
| `/Proberaum/Metronome/threshold` | Integer (ADC) | Per clap | Detection sensitivity |
| `/Proberaum/Metronome/Max` | Integer (ADC avg) | Per clap | Signal peak level |
| `/Proberaum/Metronome/Min` | Integer (ADC avg) | Per clap | Signal noise floor |
| `/Proberaum/Metronome/Gain` | Integer (0-2) | On change | Amplifier gain |

---

## Implementation Details

**Code Location**: `ESP_ClapMetronome.ino`

```cpp
/**
 * Implements: #N (REQ-F-003: MQTT Telemetry Publishing)
 * 
 * MQTT initialization: lines 24-28, 173
 * Tempo publishing: line 278
 * Threshold publishing: lines 393-395
 * Gain publishing: lines 337, 401
 */

// Connection setup (line 173)
mqtt.begin(host, net);

// Tempo update (line 278)
mqtt.publish("/Proberaum/Metronome/Tempo", String(BPM));

// Threshold and statistics (lines 393-395)
mqtt.publish("/Proberaum/Metronome/threshold", String(threshold));
mqtt.publish("/Proberaum/Metronome/Max", String(fMaxSum/avgCount));
mqtt.publish("/Proberaum/Metronome/Min", String(fMinSum/avgCount));

// Gain updates (line 337, 401)
mqtt.publish("/Proberaum/Metronome/Gain", String(gain));
```

---

## Validation Required

⚠️ **Reverse-engineered - Needs validation:**

- [ ] Confirm MQTT broker address and port
- [ ] Verify topic naming convention (camelCase vs snake_case)
- [ ] Check if QoS level and retain flags needed
- [ ] Validate WiFi reconnection handling
- [ ] Test MQTT reconnection on broker failure
- [ ] Consider adding authentication (username/password)
- [ ] Add status/availability topic (birth/LWT messages)

---

## Security Considerations

⚠️ **Hardcoded credentials in code:**
- MQTT broker IP: `yourIP`
- WiFi SSID: `"yourSSID"`
- WiFi password: `"yourPassword"`

**Action Required**: Move to configuration file or environment variables

---

## Traces To

- **Parent**: #N (StR-001: Clap-Based Metronome System)
- **Depends on**: #N (REQ-F-004: WiFi Connectivity)
- **Related**: #N (REQ-F-001: Audio Clap Detection)
- **Related**: #N (REQ-F-002: BPM Calculation)
- **Verified by**: TEST-F-003 (to be created)
- **Architecture**: ADR-COMM-001 (MQTT Protocol Selection)

---

## Create GitHub Issue

```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p1" \
  --title "REQ-F-003: MQTT Telemetry Publishing" \
  --body-file REQ-F-003-mqtt-telemetry.md
```
