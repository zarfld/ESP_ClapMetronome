# REQ-F-002: BPM Calculation from Tap Intervals

**Type**: Functional Requirement  
**Priority**: P0 (Critical)  
**Status**: Draft - Reverse Engineered  
**Source**: `ESP_ClapMetronome.ino` lines 476-641

---

## Description

System shall calculate tempo (BPM) from detected tap intervals, validate timing consistency, and adapt to tempo changes including half-tempo and double-tempo detection.

---

## Functional Requirements

### REQ-F-002.1: Tap Interval Recording

System shall maintain circular buffer of tap timestamps:

- **Buffer size**: 64 tap events
- **Timestamp precision**: Milliseconds (float)
- **Interval calculation**: Delta between consecutive taps
- **Minimum taps for BPM**: 4 taps (signature count)

### REQ-F-002.2: BPM Calculation Algorithm

System shall calculate BPM from tap intervals:

- **Formula**: `BPM = 60000 / average_interval_ms`
- **Valid range**: 50-205 BPM
- **Signature**: 4 beats per bar (configurable)
- **Consistency check**: Validate intervals within tolerance

### REQ-F-002.3: Timing Consistency Validation

System shall validate tap timing consistency:

- **Tolerance**: ±5% deviation from average interval
- **Validation**: Each tap interval compared to running average
- **Invalid detection**: If deviation > 5%, check for half/double tempo

### REQ-F-002.4: Half-Tempo and Double-Tempo Detection

System shall detect tempo multiplier changes:

- **Double-tempo**: If intervals consistently half of expected
- **Half-tempo**: If intervals consistently double of expected
- **Adaptation threshold**: Requires `tap_fail_limit` (5) consecutive matches
- **Automatic adjustment**: BPM multiplied or divided by 2

### REQ-F-002.5: Tempo Change Adaptation

System shall adapt to gradual tempo changes:

- **Change threshold**: ±5% of current BPM
- **Update condition**: New BPM outside tolerance range
- **Reset on change**: Clear tap buffer on significant tempo shift
- **MQTT notification**: Publish tempo changes

---

## Acceptance Criteria

```gherkin
Scenario: Calculate BPM from consistent taps
  Given tap buffer is empty
  When musician taps 4 times at 500ms intervals
  Then system calculates BPM as 120 (±2)
  And publishes new tempo via MQTT

Scenario: Reject inconsistent taps
  Given tap intervals vary by >5%
  When 4 taps recorded: 500ms, 520ms, 480ms, 600ms
  Then system returns BPM = 0 (invalid)
  And does not update current tempo

Scenario: Detect double-tempo transition
  Given system tracking 100 BPM (600ms intervals)
  When musician switches to 200 BPM (300ms intervals)
  And 5+ consecutive taps at 300ms intervals detected
  Then system updates to 200 BPM
  And resets tap buffer

Scenario: Detect half-tempo transition
  Given system tracking 140 BPM (428ms intervals)
  When musician switches to 70 BPM (857ms intervals)
  And 5+ consecutive taps match half-tempo
  Then system updates to 70 BPM
```

---

## Implementation Details

**Code Location**: `ESP_ClapMetronome.ino`

```cpp
/**
 * Implements: #N (REQ-F-002: BPM Calculation from Tap Intervals)
 * 
 * Main BPM calculation: bpms() function lines 476-641
 * Tap recording: lines 268-271
 * Tempo adaptation: lines 273-281, 308-317
 */

// BPM calculation (line 529)
return 60000/(total / (signiture - 1));

// Timing validation (lines 540-580)
float avg_interval = (total / cnt);
float t_diff = abs(float(avg_interval) - float(tap_intervals[tap]));
t_deviation = (t_diff / avg_interval) * 100;

if (tap_tolerance_perc < t_deviation) {
  // Check for half/double tempo
}

// Tempo change detection (lines 273-281)
if ((tmpBPM < (BPM - tap_tolerance_bpm)) || 
    (tmpBPM > (BPM + tap_tolerance_bpm))) {
  BPM = tmpBPM;
  mqtt.publish("/Proberaum/Metronome/Tempo", String(BPM));
}
```

---

## Validation Required

⚠️ **Reverse-engineered - Needs validation:**

- [ ] Verify 5% tolerance is appropriate for musical timing
- [ ] Test with various musical styles (steady vs. rubato)
- [ ] Confirm 4-tap minimum is sufficient for accuracy
- [ ] Validate half/double tempo detection doesn't false trigger
- [ ] Check behavior during accelerando/ritardando
- [ ] Test edge cases: very fast (>200 BPM) and slow (<60 BPM)

---

## Traces To

- **Parent**: #N (StR-001: Clap-Based Metronome System)
- **Depends on**: #N (REQ-F-001: Audio Clap Detection)
- **Related**: #N (REQ-F-003: MQTT Telemetry)
- **Verified by**: TEST-F-002 (to be created)
- **Architecture**: ADR-ALGO-001 (BPM Calculation Algorithm)

---

## Create GitHub Issue

```bash
gh issue create \
  --label "type:requirement:functional,phase:02-requirements,priority:p0" \
  --title "REQ-F-002: BPM Calculation from Tap Intervals" \
  --body-file REQ-F-002-bpm-calculation.md
```
