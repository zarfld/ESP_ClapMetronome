# StR-001: Clap-Based Metronome System

**Type**: Stakeholder Requirement  
**Priority**: P0 (Critical)  
**Status**: Draft - Reverse Engineered from Code  
**Source**: `ESP_ClapMetronome.ino` (legacy implementation)

---

## Description

Musicians need a metronome system that automatically detects tempo from acoustic claps or taps, synchronizes to detected rhythm, and provides remote monitoring capabilities via MQTT protocol.

**Business Context**: Rehearsal room automation requiring hands-free tempo synchronization.

---

## Stakeholder Needs

### Primary Stakeholders
- **Musicians/Performers**: Need hands-free tempo detection and synchronization
- **Sound Engineers**: Require remote monitoring of timing metrics
- **Rehearsal Room Operators**: Need automated tempo tracking system

### Key Requirements
1. Detect acoustic claps/taps via microphone
2. Calculate tempo (BPM) from detected rhythm patterns
3. Adapt to tempo changes in real-time
4. Provide visual feedback (LED, optional LCD)
5. Publish timing metrics remotely (MQTT)
6. Operate continuously with watchdog protection

---

## Business Constraints

- **Hardware**: ESP8266 or ESP32 microcontroller
- **Audio Input**: MAX9814 microphone amplifier with automatic gain control
- **Connectivity**: WiFi with MQTT broker access
- **Tempo Range**: 50-205 BPM
- **Real-time**: Sub-100ms detection latency required

---

## Success Criteria

```gherkin
Scenario: Musician establishes tempo
  Given system is powered and connected to WiFi
  When musician claps 4 times at 120 BPM
  Then system detects tempo within ±5 BPM
  And LED blinks at detected tempo
  And MQTT publishes BPM value

Scenario: Tempo change adaptation
  Given system is tracking 100 BPM
  When musician changes to 140 BPM over 4 claps
  Then system adapts to new tempo within 2 seconds
  And MQTT updates reflect new BPM
```

---

## Validation Required

⚠️ **This requirement was reverse-engineered from code:**
- [ ] Verify with stakeholders: Is this the correct business need?
- [ ] Confirm operational context (rehearsal room vs. stage)
- [ ] Validate tempo range (50-205 BPM sufficient?)
- [ ] Check connectivity requirements (MQTT mandatory or optional?)
- [ ] Confirm hardware constraints still valid

---

## Traces To (GitHub Issues)

**Create GitHub Issue**:
```bash
gh issue create \
  --label "type:stakeholder-requirement,phase:01-stakeholder-requirements,priority:p0" \
  --title "StR-001: Clap-Based Metronome System" \
  --body-file StR-001-clap-metronome.md
```

**Child Requirements** (to be created):
- REQ-F-001: Audio Clap Detection
- REQ-F-002: BPM Calculation from Taps
- REQ-F-003: MQTT Telemetry Publishing
- REQ-F-004: WiFi Connectivity Management
- REQ-NF-001: Real-Time Performance Requirements
