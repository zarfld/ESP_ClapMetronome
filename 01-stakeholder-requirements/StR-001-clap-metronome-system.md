# StR-001: Clap-Based Metronome System with Remote Control

**Status**: Draft  
**Priority**: P0 (Critical)  
**Phase**: 01-stakeholder-requirements  
**Created**: 2025-11-21  
**Standard**: ISO/IEC/IEEE 29148:2018

---

## Stakeholder Information

### Primary Stakeholders

1. **Drummers** (Live & Rehearsal)
   - Need: Keep tempo tight without losing musical feel
   - Pain: Traditional metronomes require manual setup, don't follow live performance
   - Goal: Visual/audio guidance that follows their playing

2. **FOH Engineers** (Front of House)
   - Need: Monitor band tempo remotely from mixing console
   - Pain: Can't see tempo changes without being on stage
   - Goal: Remote dashboard showing real-time BPM and lock status

3. **Band Leaders / Musical Directors**
   - Need: Ensure entire band stays in sync
   - Pain: Individual click tracks can drift
   - Goal: Master tempo source synced across stage

4. **Lighting Technicians**
   - Need: Sync light shows to music tempo
   - Pain: Manual tap-tempo is inaccurate
   - Goal: Automated MIDI beat clock for DMX controllers

5. **DIY Makers / Hackers**
   - Need: Affordable, hackable stage tech
   - Pain: Commercial solutions cost 500+ EUR
   - Goal: Open-source firmware, <30 EUR BOM

---

## Business Context

### Problem Statement

**Current State**: Musicians use traditional metronomes that:
- Require manual BPM entry (no automatic detection)
- Don't adapt to live performance tempo changes
- Can't be monitored remotely
- Don't integrate with stage lighting or recording systems
- Are expensive (commercial solutions: 300-1000 EUR)

**Desired State**: A smart metronome that:
- Automatically detects tempo from acoustic signals (claps, drums, clicks)
- Follows tempo changes in real-time (smooth tempo-following)
- Provides visual feedback (LED beat indicators)
- Enables remote monitoring via web interface and MQTT
- Sends MIDI beat clock for synchronization
- Costs <30 EUR to build (ESP32 + microphone + LEDs)

**Impact**: 
- Reduced setup time (30s vs 5 minutes for manual entry)
- Improved performance quality (real-time tempo adaptation)
- Enhanced stage production (sync lights, visuals, recording)
- Democratized access (affordable for hobbyists)

---

## Product Vision

> **A real-time tempo detection system** that listens to acoustic signals (claps, drums, kicks) and provides synchronized visual/audio/MIDI feedback for musicians, enabling tight tempo control and integration with stage production systems.

**Core Value Propositions**:
1. **Automatic Tempo Detection** - No manual BPM entry
2. **Real-Time Adaptation** - Follows tempo changes smoothly
3. **Remote Monitoring** - Web UI + MQTT telemetry
4. **Universal Integration** - MIDI beat clock, RTP MIDI, MQTT
5. **Open & Affordable** - ESP32-based, <30 EUR BOM

---

## Needs & Goals

### Functional Needs

| Need | Description | Source Stakeholder |
|------|-------------|-------------------|
| **Tempo Detection** | Detect BPM from acoustic signals (20-500 BPM range) | All musicians |
| **Beat Synchronization** | Provide beat-accurate visual/audio cues | Drummers, Band Leaders |
| **Remote Monitoring** | Display tempo, lock status, audio levels via HTTP/MQTT | FOH Engineers, Remote Producers |
| **MIDI Integration** | Send MIDI beat clock and song position pointer | Lighting Tech, Recording Engineers |
| **Configuration** | Adjust sensitivity, thresholds, output modes via web GUI | All users |
| **Telemetry** | Log tempo history, beat accuracy, system health | FOH Engineers, Developers |

### Non-Functional Needs

| Attribute | Target | Source Stakeholder |
|-----------|--------|-------------------|
| **Latency** | <20ms beat detection | Drummers (feel-critical) |
| **Accuracy** | ±0.5 BPM at steady tempo | All musicians |
| **Reliability** | >99.9% uptime (no crashes during show) | FOH Engineers |
| **Usability** | Setup in <30 seconds | All users |
| **Cost** | <30 EUR BOM | DIY Makers |
| **Portability** | Battery-powered, <500g | Drummers (rehearsal rooms) |

---

## Success Criteria

### Scenario 1: Live Performance Tempo Following

```gherkin
Given drummer plays at 120 BPM
When drummer accelerates to 130 BPM over 8 bars
Then system smoothly follows tempo change
And LED beat indicators match drummer's feel
And MIDI clock updates without jarring jumps
And remote web UI displays "LOCKED" status at 130 BPM
```

**Measurable Success**: 
- Tempo lock achieved within 4 beats of steady tempo
- Tempo changes tracked with <1 BPM error
- <20ms latency from beat to LED flash

---

### Scenario 2: Remote FOH Monitoring

```gherkin
Given FOH engineer is 30 meters from stage
When band starts playing at 140 BPM
Then engineer opens http://157.247.3.96/ on tablet
And web UI shows real-time BPM: 140.2
And lock status indicator: "LOCKED"
And audio signal level: 420/680 (healthy)
And MQTT telemetry publishes every 5 seconds
```

**Measurable Success**:
- Web UI loads in <2 seconds
- Telemetry update rate: 5 seconds
- MQTT round-trip latency: <100ms

---

### Scenario 3: Lighting Sync via MIDI

```gherkin
Given lighting controller listens to RTP MIDI on 192.168.1.100:5004
When system detects 128 BPM
Then MIDI beat clock sends 24 ticks per quarter note
And song position pointer (SPP) increments correctly
And DMX lighting syncs to beats
And light show stays in time for entire 3-minute song
```

**Measurable Success**:
- MIDI clock jitter: <1ms
- No missed beats over 1000 beats
- SPP accuracy: ±1 beat

---

## Constraints

### Technical Constraints

- **Hardware**: ESP32 (minimum 520KB RAM, WiFi)
- **Microphone**: MAX9814 AGC electret (or equivalent ADC input)
- **RTC**: DS3231 for accurate timestamps
- **Power**: USB 5V or battery (LiPo 3.7V, 18650 cell)
- **Storage**: microSD optional (for logging)

### Operational Constraints

- **Environment**: Stage noise levels (80-110 dBSPL)
- **Connectivity**: WiFi 2.4GHz (5GHz optional)
- **Setup Time**: <30 seconds from power-on to operational
- **Portability**: Fit in standard mic stand clip

### Regulatory Constraints

- **EMI**: Must not interfere with wireless microphones (FCC Part 15)
- **Safety**: Low voltage (<12V DC), no exposed mains
- **Open Source**: Firmware and schematics under MIT License

---

## Acceptance Criteria (High-Level)

- [ ] **AC-STR-001**: System detects tempo from acoustic signals (claps, drums, kicks) in range 50-200 BPM
- [ ] **AC-STR-002**: Beat detection latency <20ms (from signal peak to LED flash)
- [ ] **AC-STR-003**: Web UI accessible at http://<device-ip>/ showing real-time BPM, lock status, audio levels
- [ ] **AC-STR-004**: Configuration accessible at http://<device-ip>/api/config (JSON API)
- [ ] **AC-STR-005**: MQTT telemetry publishes to configurable broker every 5 seconds
- [ ] **AC-STR-006**: MIDI beat clock sent via RTP MIDI (port 5004) at 24 PPQ
- [ ] **AC-STR-007**: System operates for 4 hours on battery power
- [ ] **AC-STR-008**: Total BOM cost <30 EUR (excluding optional battery)
- [ ] **AC-STR-009**: Setup time <30 seconds (power-on to operational)
- [ ] **AC-STR-010**: Open-source firmware on GitHub with comprehensive documentation

---

## Risks & Assumptions

### Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| **False positives in noisy environment** | High | High | Multi-layer filtering (amplitude, frequency, timing) |
| **WiFi connectivity issues on stage** | Medium | Medium | Fall back to offline mode (LED-only), cache telemetry |
| **MIDI clock jitter** | Medium | High | Use RTC for precision timing, not millis() |
| **Battery life insufficient** | Low | Medium | Optimize deep sleep modes, USB power preferred |
| **Cost exceeds 30 EUR** | Low | Low | Use generic ESP32-WROOM module, bulk-buy components |

### Assumptions

- Stage environment has power outlets or users bring USB power banks
- WiFi router on stage (or users create hotspot)
- Users have basic electronics skills (soldering, flashing firmware)
- MQTT broker available (e.g., Mosquitto on Raspberry Pi, or cloud broker)

---

## Traceability

**Verified by Stakeholder Requirements**:
- This is the root stakeholder requirement (no parent)

**Refined by Functional Requirements**:
- #REQ-F-001 (Tempo Detection)
- #REQ-F-002 (BPM Calculation)
- #REQ-F-003 (Lock Status Detection)
- #REQ-F-004 (LED Beat Output)
- #REQ-F-005 (Web Server HTTP API)
- #REQ-F-006 (Web UI Dashboard)
- #REQ-F-007 (MQTT Telemetry Publishing)
- #REQ-F-008 (MIDI Beat Clock Output)
- #REQ-F-009 (RTP MIDI Support)
- #REQ-F-010 (Configuration Management)

**Implemented by Architecture Components**:
- #ARC-C-001 (Audio Detection Module)
- #ARC-C-002 (BPM Calculator)
- #ARC-C-003 (Timing Manager with RTC)
- #ARC-C-004 (Configuration Manager)
- #ARC-C-005 (Web Server)
- #ARC-C-006 (MQTT Client)
- #ARC-C-007 (MIDI Output Manager)

**Verified by Tests**:
- #TEST-AUDIO-001 (Beat detection accuracy)
- #TEST-BPM-001 (BPM calculation accuracy)
- #TEST-WEB-001 (HTTP API endpoints)
- #TEST-MQTT-001 (Telemetry publishing)
- #TEST-MIDI-001 (Beat clock accuracy)

---

## Notes

**Reverse Engineered From**: 
- Existing codebase: `ESP_ClapMetronome.ino`, `src/main.cpp`, `src/audio/AudioDetection.cpp`, etc.
- Hardware tests: 403/403 tests passing (Wave 1-3)
- Architecture documents: `03-architecture/ARCHITECTURE-SUMMARY.md`

**Next Steps**:
1. Create functional requirement issues (REQ-F-001 to REQ-F-010)
2. Create architecture decision records (ADR-001 to ADR-010)
3. Link existing tests to requirements
4. Implement missing features (Web UI, MQTT, MIDI)
5. Generate traceability matrix

---

**Author**: GitHub Copilot (reverse-engineered from existing project)  
**Reviewers**: [Pending]  
**Approved by**: [Pending]  
**Last Updated**: 2025-11-21
