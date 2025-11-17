# ESP_ClapMetronome Reverse Engineering Report

**Date**: 2025-11-17  
**Source**: `ESP_ClapMetronome.ino` (Arduino IDE legacy implementation)  
**Target**: PlatformIO migration with GitHub Issues traceability

---

## Executive Summary

Analyzed legacy Arduino clap metronome implementation and extracted:

- **1 Stakeholder Requirement** (StR-001)
- **4 Functional Requirements** (REQ-F-001 through REQ-F-004)
- **1 Non-Functional Requirement** (REQ-NF-001)
- **PlatformIO configuration** for ESP8266/ESP32 dual support

All requirements reverse-engineered from code and **require stakeholder validation** before becoming authoritative.

---

## Requirements Summary

### Stakeholder Requirements

| ID | Title | Priority | Description |
|----|-------|----------|-------------|
| StR-001 | Clap-Based Metronome System | P0 | Musicians need automated tempo detection from acoustic claps |

### Functional Requirements

| ID | Title | Priority | Key Features |
|----|-------|----------|--------------|
| REQ-F-001 | Audio Clap Detection | P0 | Analog sampling, adaptive threshold, auto-gain control |
| REQ-F-002 | BPM Calculation | P0 | Tap interval analysis, half/double tempo detection |
| REQ-F-003 | MQTT Telemetry | P1 | Publish tempo, threshold, gain to MQTT broker |
| REQ-F-004 | WiFi Connectivity | P1 | ESP8266/ESP32 WiFi management |

### Non-Functional Requirements

| ID | Title | Priority | Category |
|----|-------|----------|----------|
| REQ-NF-001 | Real-Time Performance | P0 | <50ms latency, watchdog timer, timing accuracy |

---

## Create GitHub Issues

### Step 1: Create Stakeholder Requirement

```bash
cd /d/Repos/ESP_ClapMetronome

gh issue create \
  --label "type:stakeholder-requirement,phase:01-stakeholder-requirements,priority:p0" \
  --title "StR-001: Clap-Based Metronome System" \
  --body-file docs/reverse-engineering/StR-001-clap-metronome.md
```

**Note**: Save the issue number (e.g., `#1`) for linking child requirements.

---

### Step 2: Create Functional Requirements

Replace `#N` with the StR-001 issue number from Step 1.

**REQ-F-001: Audio Clap Detection**
```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p0" \
  --title "REQ-F-001: Audio Clap Detection" \
  --body-file docs/reverse-engineering/REQ-F-001-audio-clap-detection.md

# After creation, link to parent:
gh issue comment <ISSUE_NUMBER> --body "**Traces to**: #N (StR-001)"
```

**REQ-F-002: BPM Calculation**
```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p0" \
  --title "REQ-F-002: BPM Calculation from Tap Intervals" \
  --body-file docs/reverse-engineering/REQ-F-002-bpm-calculation.md

gh issue comment <ISSUE_NUMBER> --body "**Traces to**: #N (StR-001)"
```

**REQ-F-003: MQTT Telemetry**
```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p1" \
  --title "REQ-F-003: MQTT Telemetry Publishing" \
  --body-file docs/reverse-engineering/REQ-F-003-mqtt-telemetry.md

gh issue comment <ISSUE_NUMBER> --body "**Traces to**: #N (StR-001)"
```

**REQ-F-004: WiFi Connectivity**
```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p1" \
  --title "REQ-F-004: WiFi Connectivity Management" \
  --body-file docs/reverse-engineering/REQ-F-004-wifi-connectivity.md

gh issue comment <ISSUE_NUMBER> --body "**Traces to**: #N (StR-001)"
```

---

### Step 3: Create Non-Functional Requirement

```bash
gh issue create \
  --label "type:requirement:non-functional,phase:02-requirements,priority:p0,category:performance" \
  --title "REQ-NF-001: Real-Time Performance Requirements" \
  --body-file docs/reverse-engineering/REQ-NF-001-realtime-performance.md

gh issue comment <ISSUE_NUMBER> --body "**Traces to**: #N (StR-001)"
```

---

## PlatformIO Migration

### Step 1: Initialize PlatformIO Project

```bash
# platformio.ini already created in repository root
# Verify configuration:
cat platformio.ini
```

### Step 2: Restructure Source Files

```bash
# Create PlatformIO directory structure
mkdir -p src lib include

# Move main sketch to src/
mv ESP_ClapMetronome.ino src/main.cpp

# Move pitches.h to include/
mv pitches.h include/

# Update #include in main.cpp:
# Change: #include "pitches.h"
# To:     #include <pitches.h>
```

### Step 3: Install Dependencies

```bash
# PlatformIO will auto-install libraries from platformio.ini:
pio pkg install

# Or manually install:
pio pkg install --library "adafruit/Adafruit MCP23017 Arduino Library@^2.3.2"
pio pkg install --library "adafruit/RTClib@^2.1.4"
pio pkg install --library "256dpi/MQTT@^2.5.2"
```

### Step 4: Build for Target Platform

```bash
# Build for ESP32:
pio run -e esp32dev

# Build for ESP8266:
pio run -e nodemcuv2

# Upload to ESP32:
pio run -e esp32dev -t upload

# Monitor serial output:
pio device monitor --baud 115200
```

---

## Missing Libraries to Resolve

**Note**: Two libraries from the original code need location/replacement:

1. **ICUsingMCP23017** - Not found in PlatformIO registry
   - Action: Search GitHub or implement locally
   - Fallback: May not be critical if MCP23017 direct control works

2. **Alislahish_MAX9814** - Custom library for MAX9814 gain control
   - Action: Locate source or reimplement gain control pins
   - Source: Appears to be custom wrapper for MCP23017 GPIO control

**Recommendation**: Create `lib/` folder with local copies of these libraries.

---

## Code Traceability Updates

After creating GitHub issues, add traceability comments to code:

```cpp
// In src/main.cpp (formerly ESP_ClapMetronome.ino)

/**
 * ESP Clap Metronome - Automatic Tempo Detection
 * 
 * Implements: #1 (StR-001: Clap-Based Metronome System)
 * 
 * Functional Requirements:
 *   - #2 (REQ-F-001: Audio Clap Detection)
 *   - #3 (REQ-F-002: BPM Calculation)
 *   - #4 (REQ-F-003: MQTT Telemetry)
 *   - #5 (REQ-F-004: WiFi Connectivity)
 * 
 * Non-Functional Requirements:
 *   - #6 (REQ-NF-001: Real-Time Performance)
 * 
 * Repository: https://github.com/zarfld/ESP_ClapMetronome
 */
```

Add function-level traceability:

```cpp
/**
 * Calculate BPM from tap intervals
 * 
 * Implements: #3 (REQ-F-002: BPM Calculation)
 * 
 * @return BPM value or 0 if invalid
 */
int bpms() {
  // ... existing implementation
}

/**
 * Main loop - audio sampling and clap detection
 * 
 * Implements: #2 (REQ-F-001: Audio Clap Detection)
 * Related: #6 (REQ-NF-001: Real-Time Performance)
 */
void loop() {
  // ... existing implementation
}
```

---

## Validation Checklist

Before treating these requirements as authoritative:

### Stakeholder Validation
- [ ] Confirm business need with musicians/users
- [ ] Validate tempo range (50-205 BPM) sufficient
- [ ] Verify MQTT requirement (could be optional)
- [ ] Check hardware constraints still valid

### Technical Validation
- [ ] Test adaptive threshold algorithm with real claps
- [ ] Measure actual detection latency (<50ms target)
- [ ] Validate BPM calculation accuracy
- [ ] Test half/double tempo detection reliability
- [ ] Verify WiFi stability over extended operation
- [ ] Benchmark watchdog timer behavior

### Security Review
- [ ] Remove hardcoded WiFi credentials
- [ ] Add MQTT authentication
- [ ] Implement secure credential storage
- [ ] Consider OTA update security

---

## Next Steps

### Phase 02: Requirements Refinement

1. **Create missing requirements**:
   - REQ-F-005: LED Visual Feedback
   - REQ-F-006: LCD Display (optional)
   - REQ-F-007: Tap Output Synchronization
   - REQ-NF-002: Reliability (Watchdog details)
   - REQ-NF-003: Network Resilience

2. **Create test issues** (Phase 07):
   - TEST-F-001: Audio Detection Tests
   - TEST-F-002: BPM Calculation Tests
   - TEST-F-003: MQTT Integration Tests
   - TEST-NF-001: Performance Benchmarks

### Phase 03: Architecture Design

Create ADRs for key decisions:
- ADR-HW-001: MAX9814 Microphone Amplifier Selection
- ADR-ALGO-001: BPM Calculation Algorithm
- ADR-COMM-001: MQTT Protocol Selection
- ADR-COMM-002: WiFi Configuration Strategy
- ADR-ARCH-001: Real-Time Processing Architecture

### Phase 04: Detailed Design

Document component designs:
- Audio sampling module design
- BPM calculation state machine
- MQTT telemetry manager
- Adaptive threshold controller

### Phase 05: Implementation (PlatformIO)

- Migrate to PlatformIO structure
- Refactor monolithic .ino into modules
- Implement missing libraries
- Add unit tests (PlatformIO test framework)
- Configure CI/CD pipeline

---

## Files Created

```
docs/reverse-engineering/
├── StR-001-clap-metronome.md
├── REQ-F-001-audio-clap-detection.md
├── REQ-F-002-bpm-calculation.md
├── REQ-F-003-mqtt-telemetry.md
├── REQ-F-004-wifi-connectivity.md
└── REQ-NF-001-realtime-performance.md

platformio.ini (root)
```

---

## Summary

✅ **Completed**:
- Analyzed 650+ line legacy Arduino sketch
- Extracted 6 requirements with full traceability
- Created PlatformIO configuration for dual-platform support
- Documented all reverse-engineered functionality
- Provided GitHub Issue creation commands

⚠️ **Critical Actions Required**:
1. Validate all requirements with stakeholders
2. Create GitHub Issues using provided commands
3. Remove hardcoded credentials from code
4. Locate/implement missing libraries (ICUsingMCP23017, Alislahish_MAX9814)
5. Migrate code to PlatformIO structure

🎯 **Goal**: Transform undocumented Arduino sketch into standards-compliant, traceable PlatformIO project ready for enhancement.

---

**Generated**: 2025-11-17  
**Tool**: code-to-requirements.prompt.md  
**AI Agent Guardrails**: Requirements validated against implementation-based assumptions per guardrail guidelines.
