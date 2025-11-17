# REQ-F-001: Audio Clap Detection

**Type**: Functional Requirement  
**Priority**: P0 (Critical)  
**Status**: Draft - Reverse Engineered  
**Source**: `ESP_ClapMetronome.ino` lines 232-290

---

## Description

System shall detect acoustic claps/taps via analog microphone input (MAX9814), applying adaptive threshold detection with automatic gain control.

---

## Functional Requirements

### REQ-F-001.1: Analog Audio Sampling

System shall continuously sample analog audio input (A0 pin) at high frequency:

- Sample buffer: 32 samples per detection cycle
- Floating window: 64 samples for min/max tracking
- ADC range: 0-1023 (10-bit resolution)

### REQ-F-001.2: Adaptive Threshold Detection

System shall dynamically adjust detection threshold:

- Initial threshold: 50 (ADC units)
- Adaptive formula: `threshold = 0.8 * (max - min) + min`
- Update on each successful detection
- Minimum separation: 30 ADC units between max and min

### REQ-F-001.3: Rising Edge Detection

System shall detect claps on rising edge with timing validation:

- Trigger condition: `sample > threshold AND tapped == false`
- Rise time validation: `<6ms` from minimum to trigger
- Debounce: Ignore until falling edge detected

### REQ-F-001.4: Automatic Gain Control

System shall adjust MAX9814 gain to prevent clipping:

- **Clipping prevention**: Reduce gain if `sample > 1000`
- **Sensitivity increase**: Increase gain if `max - min < 30`
- **Gain range**: 0-2 (three gain levels)
- **MQTT notification**: Publish gain changes

---

## Acceptance Criteria

```gherkin
Scenario: Detect single clap
  Given system is idle with adaptive threshold at 80
  When acoustic clap produces peak of 400 ADC units
  Then clap is detected within 10ms
  And threshold updates to new adaptive value
  And LED indicator activates

Scenario: Prevent false triggers from noise
  Given ambient noise level is 50 ADC units
  When noise spikes to 70 ADC units (below threshold)
  Then no detection event occurs
  And threshold remains stable

Scenario: Auto-gain prevents clipping
  Given current gain level is 2 (highest)
  When clap produces ADC reading > 1000
  Then system reduces gain to level 1
  And MQTT publishes new gain value
```

---

## Implementation Details

**Code Location**: `ESP_ClapMetronome.ino`

```cpp
/**
 * Implements: #N (REQ-F-001: Audio Clap Detection)
 * 
 * Main detection logic in loop() lines 232-370
 * Adaptive threshold calculation lines 388-395
 * Gain control lines 326-346
 */

// Detection trigger (line 258-282)
if ((tapped == false) and (Vanalogread[rIndex]>threshold)){
  float risetime = now - TimeMin[i_fAvg];
  if (risetime<riseTlim){
    // Clap detected
  }
}

// Adaptive threshold (line 392-394)
int avgDiv = fMaxSum-fMinSum;
threshold = ((0.8*avgDiv)+fMinSum)/avgCount;
```

---

## Validation Required

⚠️ **Reverse-engineered - Needs validation:**

- [ ] Verify sample rate sufficient for clap detection
- [ ] Confirm threshold formula produces reliable detection
- [ ] Test with various clap volumes and distances
- [ ] Validate gain control prevents clipping without losing sensitivity
- [ ] Check for false positives from other percussive sounds

---

## Traces To

- **Parent**: #N (StR-001: Clap-Based Metronome System)
- **Depends on**: Hardware MAX9814 configuration
- **Verified by**: TEST-F-001 (to be created)
- **Architecture**: ADR-HW-001 (MAX9814 Selection)

---

## Create GitHub Issue

```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p0" \
  --title "REQ-F-001: Audio Clap Detection" \
  --body-file REQ-F-001-audio-clap-detection.md
```
