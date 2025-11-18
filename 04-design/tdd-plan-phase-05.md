# Phase 05: Test-Driven Development (TDD) Plan

**Document ID**: TDD-PLAN-PHASE-05  
**Version**: 1.0.0  
**Date**: 2025-11-18  
**Status**: Approved for Phase 05  
**Standard**: ISO/IEC/IEEE 12207:2017 (Implementation Process), XP Test-First Programming  

---

## 1. Overview

This TDD plan enables systematic **Red-Green-Refactor** implementation for all Phase 04 designs. It provides:

- **Mock/stub interface specifications** for isolated unit testing
- **Acceptance criteria** derived from requirements and QA scenarios
- **Integration test scenarios** for component interactions
- **Test execution order** aligned with dependency graph
- **Continuous Integration (CI) strategy** for automated validation

---

## 2. XP Test-First Principles

### 2.1 Red-Green-Refactor Cycle

```
1. RED: Write a failing test that specifies desired behavior
   ↓
2. GREEN: Write minimal code to make the test pass
   ↓
3. REFACTOR: Improve design while keeping tests green
   ↓
4. REPEAT: Next smallest increment
```

**Rules**:
- ✅ Never write production code without a failing test
- ✅ Write only enough test code to demonstrate a failure
- ✅ Write only enough production code to pass the test
- ✅ Run all tests before committing (CI must be green)
- ✅ Refactor continuously; tests are the safety net

### 2.2 Test Pyramid

```
        /\
       /  \  E2E Tests (3 QA scenarios)
      /----\
     /      \  Integration Tests (26 scenarios)
    /--------\
   /          \  Unit Tests (74 tests)
  /____________\
```

**Distribution**:
- **Unit Tests**: 74 tests (64% coverage) - Fast, isolated, mock dependencies
- **Integration Tests**: 26 tests (23% coverage) - Component pairs, real dependencies
- **End-to-End (QA)**: 3 tests (3% coverage) - Full system, hardware-in-loop
- **Performance Tests**: 9 tests (8% coverage) - Timing, memory, CPU validation
- **Hardware Tests**: 3 tests (3% coverage) - Oscilloscope, protocol analyzer

---

## 3. Mock/Stub Interface Specifications

### 3.1 Foundation Layer Mocks

#### Mock: DES-I-001 (Timestamp Query)

**Purpose**: Enable testing without RTC3231 hardware

```cpp
/**
 * Mock Timing Manager for unit tests
 * Implements: DES-I-001, DES-I-002, DES-I-003
 * GitHub Issue: #44 (DES-C-005)
 */
class MockTimingManager {
public:
    // DES-I-001: Timestamp Query
    uint64_t getTimestampUs() { return mock_timestamp_us; }
    uint32_t getTimestampMs() { return mock_timestamp_us / 1000; }
    
    // DES-I-002: RTC Health Status
    bool rtcHealthy() { return mock_rtc_healthy; }
    float getRtcTemperature() { return mock_rtc_temp; }
    
    // DES-I-003: Time Synchronization
    bool syncRtc() { return mock_sync_success; }
    
    // Test control methods
    void setTimestamp(uint64_t ts) { mock_timestamp_us = ts; }
    void advanceTime(uint64_t delta_us) { mock_timestamp_us += delta_us; }
    void setRtcHealth(bool healthy) { mock_rtc_healthy = healthy; }
    void setSyncSuccess(bool success) { mock_sync_success = success; }
    
private:
    uint64_t mock_timestamp_us = 0;
    bool mock_rtc_healthy = true;
    float mock_rtc_temp = 25.0;
    bool mock_sync_success = true;
};
```

**Test Coverage**:
- ✅ Timestamp monotonicity (timestamps always increase)
- ✅ Timestamp precision (microsecond resolution)
- ✅ RTC fallback (healthy=false → use millis())
- ✅ Time synchronization success/failure

---

#### Mock: DES-I-008 (Configuration API)

**Purpose**: Enable testing without NVS flash writes

```cpp
/**
 * Mock Configuration Manager for unit tests
 * Implements: DES-I-008
 * GitHub Issue: #47 (DES-C-006)
 */
class MockConfigManager {
public:
    // DES-I-008: Configuration API
    AudioConfig getAudioConfig() { return audio_config; }
    BPMConfig getBPMConfig() { return bpm_config; }
    OutputConfig getOutputConfig() { return output_config; }
    MQTTConfig getMQTTConfig() { return mqtt_config; }
    WiFiConfig getWiFiConfig() { return wifi_config; }
    
    bool saveConfig() { 
        save_count++; 
        return mock_save_success; 
    }
    
    bool factoryReset() { 
        loadDefaults();
        return mock_reset_success; 
    }
    
    // Test control methods
    void setAudioConfig(AudioConfig cfg) { audio_config = cfg; }
    void setBPMConfig(BPMConfig cfg) { bpm_config = cfg; }
    void setOutputConfig(OutputConfig cfg) { output_config = cfg; }
    void setSaveSuccess(bool success) { mock_save_success = success; }
    int getSaveCount() { return save_count; }
    
private:
    AudioConfig audio_config;
    BPMConfig bpm_config;
    OutputConfig output_config;
    MQTTConfig mqtt_config;
    WiFiConfig wifi_config;
    bool mock_save_success = true;
    bool mock_reset_success = true;
    int save_count = 0;
    
    void loadDefaults() {
        audio_config = AudioConfig{};
        bpm_config = BPMConfig{};
        output_config = OutputConfig{};
    }
};
```

**Test Coverage**:
- ✅ Config load at boot (default values)
- ✅ Config save success/failure
- ✅ Factory reset restores defaults
- ✅ Save count tracking (verify writes)

---

### 3.2 Component Layer Mocks

#### Mock: DES-I-004 (Beat Event Interface)

**Purpose**: Test BPM calculation without audio hardware

```cpp
/**
 * Mock Audio Detection for BPM unit tests
 * Implements: DES-I-004, DES-I-005
 * GitHub Issue: #45 (DES-C-001)
 */
class MockAudioDetection {
public:
    // DES-I-004: Beat Event Interface
    void onBeat(std::function<void(const BeatEvent&)> callback) {
        beat_callback = callback;
    }
    
    // DES-I-005: Audio Telemetry Interface
    void onTelemetry(std::function<void(const AudioTelemetry&)> callback) {
        telemetry_callback = callback;
    }
    
    // Test control methods
    void simulateBeat(uint64_t timestamp, uint16_t amplitude, bool kick_only = false) {
        BeatEvent event;
        event.timestamp_us = timestamp;
        event.amplitude = amplitude;
        event.threshold = 2048;
        event.gain_level = 1;
        event.kick_only = kick_only;
        
        if (beat_callback) {
            beat_callback(event);
        }
    }
    
    void simulateBeats(int count, uint64_t interval_us, uint64_t start_time = 0) {
        for (int i = 0; i < count; i++) {
            simulateBeat(start_time + i * interval_us, 3000);
        }
    }
    
    void simulateTelemetry(uint16_t adc, uint16_t min, uint16_t max) {
        AudioTelemetry telem;
        telem.adc_value = adc;
        telem.min_value = min;
        telem.max_value = max;
        telem.threshold = (max - min) * 0.8 + min;
        telem.gain_level = 1;
        telem.clipping = (adc >= 4095);
        
        if (telemetry_callback) {
            telemetry_callback(telem);
        }
    }
    
private:
    std::function<void(const BeatEvent&)> beat_callback;
    std::function<void(const AudioTelemetry&)> telemetry_callback;
};
```

**Test Coverage**:
- ✅ Beat event callback registration
- ✅ Simulated beat sequences (regular intervals)
- ✅ Irregular beats (tempo changes)
- ✅ Kick-only mode filtering

---

#### Mock: DES-I-006 (BPM Update Interface)

**Purpose**: Test output scheduling without BPM engine

```cpp
/**
 * Mock BPM Calculation for Output unit tests
 * Implements: DES-I-006
 * GitHub Issue: #46 (DES-C-002)
 */
class MockBPMCalculation {
public:
    // DES-I-006: BPM Update Interface
    void onBPMUpdate(std::function<void(const BPMUpdate&)> callback) {
        bpm_callback = callback;
    }
    
    // Test control methods
    void simulateBPM(float bpm, bool stable = true, bool corrected = false) {
        BPMUpdate update;
        update.bpm = bpm;
        update.tap_count = 8;
        update.stable = stable;
        update.timestamp_us = mock_time;
        update.half_tempo_corrected = corrected;
        
        if (bpm_callback) {
            bpm_callback(update);
        }
        
        mock_time += 1000000; // Advance 1s
    }
    
    void simulateBPMChange(float start_bpm, float end_bpm, int steps) {
        float delta = (end_bpm - start_bpm) / steps;
        for (int i = 0; i <= steps; i++) {
            simulateBPM(start_bpm + i * delta, i >= 3);
        }
    }
    
private:
    std::function<void(const BPMUpdate&)> bpm_callback;
    uint64_t mock_time = 0;
};
```

**Test Coverage**:
- ✅ BPM update callback registration
- ✅ Stable BPM (consistent tempo)
- ✅ Unstable BPM (tempo changes)
- ✅ Half-tempo correction flag

---

### 3.3 Network Layer Stubs

#### Stub: DES-I-009 (MQTT Publish Interface)

**Purpose**: Test telemetry without MQTT broker

```cpp
/**
 * Stub MQTT Client for unit tests
 * Implements: DES-I-009
 * GitHub Issue: #48 (DES-C-007)
 */
class StubMQTTClient {
public:
    // DES-I-009: MQTT Publish Interface
    bool publishJSON(const char* topic, const char* json_payload) {
        if (!mock_connected) return false;
        
        publish_count++;
        last_topic = topic;
        last_payload = json_payload;
        return true;
    }
    
    bool publishValue(const char* topic, float value) {
        char payload[32];
        snprintf(payload, sizeof(payload), "%.2f", value);
        return publishJSON(topic, payload);
    }
    
    bool isConnected() { return mock_connected; }
    
    // Test control methods
    void setConnected(bool connected) { mock_connected = connected; }
    int getPublishCount() { return publish_count; }
    std::string getLastTopic() { return last_topic; }
    std::string getLastPayload() { return last_payload; }
    void reset() { 
        publish_count = 0; 
        last_topic = "";
        last_payload = "";
    }
    
private:
    bool mock_connected = true;
    int publish_count = 0;
    std::string last_topic;
    std::string last_payload;
};
```

**Test Coverage**:
- ✅ Publish when connected (success)
- ✅ Publish when disconnected (failure)
- ✅ JSON payload formatting
- ✅ Topic hierarchy validation

---

#### Stub: DES-I-011 (WebSocket Protocol)

**Purpose**: Test telemetry without browser clients

```cpp
/**
 * Stub WebSocket Server for unit tests
 * Implements: DES-I-011
 * GitHub Issue: #49 (DES-C-003)
 */
class StubWebSocket {
public:
    // DES-I-011: WebSocket Protocol
    void broadcastJSON(const char* json_message) {
        if (client_count == 0) return;
        
        broadcast_count++;
        last_message = json_message;
    }
    
    void sendJSON(uint32_t client_id, const char* json_message) {
        send_count++;
        last_client_id = client_id;
        last_message = json_message;
    }
    
    // Test control methods
    void setClientCount(int count) { client_count = count; }
    int getBroadcastCount() { return broadcast_count; }
    int getSendCount() { return send_count; }
    std::string getLastMessage() { return last_message; }
    void reset() {
        broadcast_count = 0;
        send_count = 0;
        last_message = "";
    }
    
private:
    int client_count = 0;
    int broadcast_count = 0;
    int send_count = 0;
    uint32_t last_client_id = 0;
    std::string last_message;
};
```

**Test Coverage**:
- ✅ Broadcast to multiple clients
- ✅ Send to specific client
- ✅ No broadcast when no clients
- ✅ JSON message formatting

---

## 4. Component Acceptance Criteria

### 4.1 DES-C-005: Timing Manager

**Requirement**: REQ-F-007 (RTC3231 I2C timing with fallback)  
**GitHub Issue**: #44  

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-TIME-001 | Timestamp monotonicity | Unit test | getTimestampUs() always increases |
| AC-TIME-002 | Microsecond precision | Unit test | Timestamps differ by <10µs on successive calls |
| AC-TIME-003 | RTC I2C communication | Integration test | I2C read success within 5ms |
| AC-TIME-004 | Automatic fallback | Unit test | Uses millis() when RTC unhealthy |
| AC-TIME-005 | Health monitoring | Unit test | rtcHealthy() returns false after 3 I2C failures |
| AC-TIME-006 | Temperature reading | Integration test | RTC temp = 20-30°C (room temp) |
| AC-TIME-007 | Time synchronization | Integration test | syncRtc() sets time accurately |
| AC-TIME-008 | Boot initialization | Integration test | <100ms to first timestamp |
| AC-TIME-009 | Continuous operation | Performance test | 24h run without timestamp rollback |

**Test Execution Order**:
1. Unit: AC-TIME-001, 002, 004, 005 (mock RTC)
2. Integration: AC-TIME-003, 006, 007, 008 (real RTC)
3. Performance: AC-TIME-009 (long-duration)

---

### 4.2 DES-C-001: Audio Detection Engine

**Requirement**: REQ-F-001 (Audio clap/kick detection), REQ-NF-001 (<20ms latency)  
**GitHub Issue**: #45  
**QA Scenario**: QA-SC-001 (100 kicks @ 140 BPM, >95% detection rate)

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-AUDIO-001 | Adaptive threshold | Unit test | threshold = 0.8 × (max - min) + min |
| AC-AUDIO-002 | Rising edge detection | Unit test | State: IDLE → RISING → TRIGGERED → DEBOUNCE |
| AC-AUDIO-003 | AGC level transitions | Unit test | AGC: 0 (40dB) → 1 (50dB) → 2 (60dB) on clipping |
| AC-AUDIO-004 | Beat event emission | Unit test | Callback fired with correct timestamp |
| AC-AUDIO-005 | Debounce period | Unit test | No beats within 50ms of previous beat |
| AC-AUDIO-006 | Kick-only filtering | Unit test | Rise time >4ms → kick_only = true |
| AC-AUDIO-007 | Telemetry updates | Integration test | Telemetry published every 500ms |
| AC-AUDIO-008 | Audio latency | QA-SC-001 | <20ms from mic input to beat event |
| AC-AUDIO-009 | Detection accuracy | QA-SC-001 | >95% of 100 kicks detected |
| AC-AUDIO-010 | CPU usage | Performance test | <45% average, <50% peak |
| AC-AUDIO-011 | Memory usage | Performance test | <400B RAM |
| AC-AUDIO-012 | Clipping prevention | Integration test | AGC increases gain when clipping |
| AC-AUDIO-013 | Noise rejection | Unit test | No detections below threshold |
| AC-AUDIO-014 | Window synchronization | Unit test | Dual buffers alternate correctly |

**Test Execution Order**:
1. Unit: AC-AUDIO-001 to 006, 013, 014 (mock ADC)
2. Integration: AC-AUDIO-007, 012 (real MAX9814)
3. Performance: AC-AUDIO-010, 011 (CPU/memory profiling)
4. QA: AC-AUDIO-008, 009 (hardware-in-loop, oscilloscope)

---

### 4.3 DES-C-002: BPM Calculation Engine

**Requirement**: REQ-F-002 (BPM from tap timestamps)  
**GitHub Issue**: #46  
**QA Scenarios**: QA-SC-002 (140 BPM accuracy), QA-SC-003 (half-tempo correction)

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-BPM-001 | Circular buffer wraparound | Unit test | 64 taps → write_index = 0 |
| AC-BPM-002 | BPM calculation accuracy | QA-SC-002 | BPM = 140.0 ± 0.5 for 100 beats |
| AC-BPM-003 | Tempo stability detection | Unit test | CV < 5% → stable = true |
| AC-BPM-004 | Half-tempo correction | QA-SC-003 | 70 BPM corrected to 140 after 6th tap |
| AC-BPM-005 | Double-tempo correction | Unit test | 280 BPM corrected to 140 after 6th tap |
| AC-BPM-006 | Minimum taps requirement | Unit test | <4 taps → BPM invalid |
| AC-BPM-007 | Tap clearing | Unit test | clearTaps() → tap_count = 0 |
| AC-BPM-008 | BPM update emission | Integration test | Callback fired on new stable BPM |
| AC-BPM-009 | Calculation latency | Performance test | <5ms from addTap() to BPM update |
| AC-BPM-010 | CPU usage | Performance test | <2% average |
| AC-BPM-011 | Memory usage | Performance test | <600B RAM |
| AC-BPM-012 | Outlier rejection | Unit test | Taps >2× avg interval ignored |
| AC-BPM-013 | Tempo range validation | Unit test | 40-300 BPM accepted, others rejected |
| AC-BPM-014 | Timestamp overflow | Unit test | uint64_t rollover handled correctly |

**Test Execution Order**:
1. Unit: AC-BPM-001, 003, 005, 006, 007, 012, 013, 014 (mock beats)
2. Integration: AC-BPM-008 (real audio → BPM)
3. Performance: AC-BPM-009, 010, 011 (timing/profiling)
4. QA: AC-BPM-002, 004 (100-beat sequences)

---

### 4.4 DES-C-006: Configuration Manager

**Requirement**: REQ-F-005 (Persistent config), REQ-F-006 (Factory reset), REQ-NF-003 (Security)  
**GitHub Issue**: #47  

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-CFG-001 | Cold boot load | Integration test | Config loaded from NVS within 50ms |
| AC-CFG-002 | Default values | Unit test | Factory defaults match specification |
| AC-CFG-003 | Config save success | Integration test | saveConfig() writes to NVS, persists after reboot |
| AC-CFG-004 | Range validation | Unit test | Invalid values rejected (e.g., BPM <40 or >300) |
| AC-CFG-005 | Factory reset | Integration test | All values restored to defaults |
| AC-CFG-006 | Password encryption | Integration test | WiFi/MQTT passwords encrypted in NVS |
| AC-CFG-007 | Config migration | Unit test | v1.0 → v1.1 key renaming |
| AC-CFG-008 | Change notifications | Unit test | Callbacks fired when config changes |
| AC-CFG-009 | NVS partition size | Integration test | <16KB used (64KB partition) |

**Test Execution Order**:
1. Unit: AC-CFG-002, 004, 007, 008 (mock NVS)
2. Integration: AC-CFG-001, 003, 005, 006, 009 (real NVS flash)

---

### 4.5 DES-C-007: MQTT Telemetry Client

**Requirement**: REQ-F-004 (MQTT publish)  
**GitHub Issue**: #48  

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-MQTT-001 | QoS 0 publish | Unit test | Fire-and-forget, no ACK wait |
| AC-MQTT-002 | Auto-reconnect | Integration test | Reconnect after disconnect (exponential backoff) |
| AC-MQTT-003 | Rate limiting | Unit test | Max 10 beats/second (100ms min interval) |
| AC-MQTT-004 | Topic hierarchy | Unit test | metronome/bpm, metronome/audio/level |
| AC-MQTT-005 | JSON payload format | Unit test | Valid JSON with timestamp, value fields |
| AC-MQTT-006 | Connection status | Integration test | isConnected() accurate |
| AC-MQTT-007 | Publish when disconnected | Unit test | Returns false, drops message (no queue) |
| AC-MQTT-008 | CPU usage | Performance test | <2% average, <5% peak |
| AC-MQTT-009 | Memory usage | Performance test | <8KB RAM |

**Test Execution Order**:
1. Unit: AC-MQTT-001, 003, 004, 005, 007 (stub broker)
2. Integration: AC-MQTT-002, 006 (Mosquitto broker)
3. Performance: AC-MQTT-008, 009 (profiling)

---

### 4.6 DES-C-003: Web Server & WebSocket

**Requirement**: REQ-F-003 (Web UI display)  
**GitHub Issue**: #49  

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-WEB-001 | Static file serving | Integration test | index.html, app.js, style.css from LittleFS |
| AC-WEB-002 | REST API GET /api/config | Integration test | Returns all config as JSON |
| AC-WEB-003 | REST API POST /api/config | Integration test | Updates config, returns success |
| AC-WEB-004 | REST API POST /api/factory-reset | Integration test | Resets config, reboots |
| AC-WEB-005 | WebSocket connection | Integration test | Client connects successfully |
| AC-WEB-006 | WebSocket BPM updates | Integration test | BPM messages broadcast to 4 clients |
| AC-WEB-007 | WebSocket rate limiting | Unit test | BPM updates max 2Hz (500ms interval) |
| AC-WEB-008 | WebSocket error handling | Unit test | Disconnect on buffer overflow |
| AC-WEB-009 | Concurrent clients | Performance test | 4 clients simultaneous, <50ms broadcast |
| AC-WEB-010 | CPU usage | Performance test | <10% average, <15% peak |

**Test Execution Order**:
1. Unit: AC-WEB-007, 008 (mock WebSocket)
2. Integration: AC-WEB-001 to 006 (real HTTP/WebSocket)
3. Performance: AC-WEB-009, 010 (4 browser tabs)

---

### 4.7 DES-C-004: Output Controller

**Requirement**: REQ-F-008 (MIDI), REQ-F-009 (Relay), REQ-NF-001 (Real-time)  
**GitHub Issue**: #50  

**Acceptance Criteria**:

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-OUT-001 | MIDI note-on format | Unit test | 0x90 + (channel-1), note, velocity |
| AC-OUT-002 | MIDI note-off format | Unit test | 0x80 + (channel-1), note, 0x00 |
| AC-OUT-003 | MIDI baud rate | Hardware test | 31.25 kbaud (oscilloscope) |
| AC-OUT-004 | MIDI note duration | Unit test | 10ms ON, then OFF |
| AC-OUT-005 | Relay pulse duration | Unit test | Configurable 10-500ms (default 50ms) |
| AC-OUT-006 | Relay watchdog | Unit test | Force OFF if stuck >100ms |
| AC-OUT-007 | Output timing jitter | Performance test | <1ms jitter over 1000 outputs |
| AC-OUT-008 | ISR execution time | Performance test | <10µs (oscilloscope) |
| AC-OUT-009 | CPU usage | Performance test | <3% average |
| AC-OUT-010 | BPM synchronization | Integration test | Output interval matches BPM |
| AC-OUT-011 | MIDI channel selection | Unit test | Channels 1-16 configurable |
| AC-OUT-012 | Relay GPIO safety | Hardware test | GPIO LOW when disabled |
| AC-OUT-013 | State machine | Unit test | IDLE → ON → OFF → IDLE transitions |

**Test Execution Order**:
1. Unit: AC-OUT-001, 002, 004, 005, 006, 011, 013 (mock hardware)
2. Integration: AC-OUT-010 (real BPM → output)
3. Performance: AC-OUT-007, 008, 009 (timing/profiling)
4. Hardware: AC-OUT-003, 012 (oscilloscope, multimeter)

---

## 5. Integration Test Scenarios

### 5.1 Real-Time Pipeline Tests

#### INT-001: Audio → BPM Flow

**Components**: DES-C-001 (Audio), DES-C-002 (BPM)  
**Interface**: DES-I-004 (Beat Event)  
**GitHub Issues**: #45, #46  

**Scenario**:
```
Given: Audio detection engine running
 When: 10 claps at 140 BPM (428ms intervals)
 Then: BPM engine calculates 140.0 ± 0.5 BPM
  And: stable = true after 4th clap
```

**Assertions**:
- ✅ Beat events propagate to BPM engine
- ✅ Timestamps accurate (within 1ms)
- ✅ BPM calculation converges to 140 ± 0.5
- ✅ Stability flag set correctly

**Test Code** (PlatformIO):
```cpp
TEST_CASE("INT-001: Audio beats propagate to BPM engine", "[integration]") {
    TimingManager timing;
    AudioDetection audio(&timing);
    BPMCalculation bpm(&timing);
    
    // Connect audio → BPM
    audio.onBeat([&](const BeatEvent& event) {
        bpm.addTap(event.timestamp_us);
    });
    
    // Simulate 10 claps at 140 BPM (428ms intervals)
    uint64_t interval_us = 428571; // 60,000,000 / 140
    for (int i = 0; i < 10; i++) {
        audio.simulateBeat(i * interval_us, 3000);
    }
    
    // Verify BPM calculation
    REQUIRE(bpm.getBPM() == Approx(140.0).margin(0.5));
    REQUIRE(bpm.isStable() == true);
}
```

---

#### INT-002: BPM → Output Synchronization

**Components**: DES-C-002 (BPM), DES-C-004 (Output)  
**Interface**: DES-I-006 (BPM Update)  
**GitHub Issues**: #46, #50  

**Scenario**:
```
Given: Output controller configured for MIDI
 When: BPM updates to 120 BPM
 Then: Output interval = 500ms (60,000 / 120)
  And: MIDI note-on every 500ms ± 1ms
```

**Assertions**:
- ✅ Output interval recalculated on BPM change
- ✅ MIDI timing matches BPM (±1ms jitter)
- ✅ Note-on followed by note-off after 10ms

**Test Code**:
```cpp
TEST_CASE("INT-002: Output synchronized to BPM", "[integration]") {
    TimingManager timing;
    BPMCalculation bpm(&timing);
    OutputController output(&timing);
    
    // Connect BPM → Output
    bpm.onBPMUpdate([&](const BPMUpdate& update) {
        output.setBPM(update.bpm);
    });
    
    // Simulate BPM update to 120
    bpm.simulateBPM(120.0, true);
    
    // Measure output intervals
    std::vector<uint64_t> intervals;
    for (int i = 0; i < 10; i++) {
        uint64_t t1 = output.getLastTriggerTime();
        output.trigger();
        delay(500); // Wait for next BPM beat
        uint64_t t2 = output.getLastTriggerTime();
        intervals.push_back(t2 - t1);
    }
    
    // Verify 500ms ± 1ms
    for (auto interval : intervals) {
        REQUIRE(interval == Approx(500000).margin(1000)); // µs
    }
}
```

---

### 5.2 Telemetry Pipeline Tests

#### INT-003: BPM → WebSocket Broadcast

**Components**: DES-C-002 (BPM), DES-C-003 (Web)  
**Interface**: DES-I-006 (BPM Update), DES-I-011 (WebSocket)  
**GitHub Issues**: #46, #49  

**Scenario**:
```
Given: 4 WebSocket clients connected
 When: BPM updates to 140
 Then: All clients receive {"type": "bpm", "value": 140.0}
  And: Broadcast latency <50ms
```

**Assertions**:
- ✅ All 4 clients receive BPM update
- ✅ JSON format correct
- ✅ Broadcast latency <50ms
- ✅ Rate limiting enforced (max 2Hz)

---

#### INT-004: BPM → MQTT Publish

**Components**: DES-C-002 (BPM), DES-C-007 (MQTT)  
**Interface**: DES-I-006 (BPM Update), DES-I-009 (MQTT Publish)  
**GitHub Issues**: #46, #48  

**Scenario**:
```
Given: MQTT connected to broker
 When: BPM updates to 140
 Then: Publish to "metronome/bpm" with payload "140.0"
  And: Publish latency <10ms
```

**Assertions**:
- ✅ MQTT message published
- ✅ Topic hierarchy correct
- ✅ Payload format correct
- ✅ QoS 0 (fire-and-forget)

---

### 5.3 Configuration Flow Tests

#### INT-005: Config → Audio Reload

**Components**: DES-C-006 (Config), DES-C-001 (Audio)  
**Interface**: DES-I-008 (Config API)  
**GitHub Issues**: #47, #45  

**Scenario**:
```
Given: Audio detection running with threshold = 2048
 When: User changes threshold to 3000 via Web UI
 Then: Config manager saves threshold = 3000
  And: Audio detection reloads config
  And: New beats use threshold = 3000
```

**Assertions**:
- ✅ Config change persists to NVS
- ✅ Audio engine notified of change
- ✅ New threshold applied immediately

---

#### INT-006: Config → Output Reload

**Components**: DES-C-006 (Config), DES-C-004 (Output)  
**Interface**: DES-I-008 (Config API)  
**GitHub Issues**: #47, #50  

**Scenario**:
```
Given: Output controller using MIDI channel 1
 When: User changes MIDI channel to 10 via Web UI
 Then: Config manager saves channel = 10
  And: Next MIDI note-on uses 0x90 + (10-1) = 0x99
```

**Assertions**:
- ✅ MIDI channel update applied
- ✅ No output during channel change
- ✅ Status byte correct after reload

---

### 5.4 End-to-End Tests

#### INT-007: Web UI → Config → NVS

**Components**: DES-C-003 (Web), DES-C-006 (Config)  
**Interface**: DES-I-012 (REST API), DES-I-008 (Config API)  
**GitHub Issues**: #49, #47  

**Scenario**:
```
Given: Web UI loaded in browser
 When: User submits config form (POST /api/config)
 Then: Config manager validates and saves to NVS
  And: Device reboots with new config
  And: Web UI shows updated config
```

**Assertions**:
- ✅ REST API accepts POST
- ✅ Config persists after reboot
- ✅ GET /api/config returns updated values

---

#### INT-008: MQTT Config Commands

**Components**: DES-C-007 (MQTT), DES-C-006 (Config)  
**Interface**: DES-I-009 (MQTT Publish), DES-I-008 (Config API)  
**GitHub Issues**: #48, #47  

**Scenario**:
```
Given: MQTT subscribed to "metronome/config/set"
 When: Broker publishes {"key": "audio.threshold", "value": 3000}
 Then: Config manager updates threshold = 3000
  And: MQTT publishes ACK to "metronome/config/ack"
```

**Assertions**:
- ✅ MQTT subscription active
- ✅ Config updated remotely
- ✅ ACK published

---

#### INT-009: Timing → All Components

**Components**: DES-C-005 (Timing), All DES-C-*  
**Interface**: DES-I-001 (Timestamp Query)  
**GitHub Issues**: #44, all  

**Scenario**:
```
Given: All components initialized
 When: Each component requests timestamps
 Then: Timestamps monotonically increase
  And: No timestamp goes backward
  And: Precision <10µs between calls
```

**Assertions**:
- ✅ Timestamps from all components increase
- ✅ No timestamp rollback
- ✅ Microsecond precision maintained

---

## 6. Test Execution Order

### 6.1 Dependency-Based Order

**Phase 1: Foundation Components** (no dependencies)
```
1. DES-C-005 (Timing Manager) - 9 unit tests
2. DES-C-006 (Config Manager) - 9 unit tests
   Total: 18 tests, ~2 hours
```

**Phase 2: Real-Time Pipeline** (depends on Timing, Config)
```
3. DES-C-001 (Audio Detection) - 14 unit tests
4. DES-C-002 (BPM Calculation) - 14 unit tests
5. INT-001 (Audio → BPM) - 1 integration test
   Total: 29 tests, ~4 hours
```

**Phase 3: Output Components** (depends on BPM)
```
6. DES-C-004 (Output Controller) - 9 unit tests
7. INT-002 (BPM → Output) - 1 integration test
   Total: 10 tests, ~2 hours
```

**Phase 4: Network Components** (depends on BPM, Config)
```
8. DES-C-007 (MQTT Client) - 9 unit tests
9. DES-C-003 (Web Server) - 10 unit tests
10. INT-003 (BPM → WebSocket) - 1 integration test
11. INT-004 (BPM → MQTT) - 1 integration test
    Total: 21 tests, ~3 hours
```

**Phase 5: Configuration Flow** (depends on all)
```
12. INT-005 (Config → Audio) - 1 integration test
13. INT-006 (Config → Output) - 1 integration test
14. INT-007 (Web → Config) - 1 integration test
15. INT-008 (MQTT → Config) - 1 integration test
16. INT-009 (Timing → All) - 1 integration test
    Total: 5 tests, ~2 hours
```

**Phase 6: Performance & QA** (system-wide)
```
17. Performance tests (9 tests) - ~3 hours
18. QA scenarios (3 tests) - ~2 hours
19. Hardware validation (3 tests) - ~2 hours
    Total: 15 tests, ~7 hours
```

**Total Estimated Time**: 18 + 29 + 10 + 21 + 5 + 15 = **98 tests, ~20 hours**

---

### 6.2 Daily Sprint Plan (5 days × 4 hours/day)

**Day 1**: Foundation + Real-Time Pipeline (Phases 1-2)
- Timing Manager unit tests (2h)
- Config Manager unit tests (2h)

**Day 2**: Real-Time Pipeline (Phase 2)
- Audio Detection unit tests (2h)
- BPM Calculation unit tests (2h)

**Day 3**: Outputs + Network (Phases 3-4)
- Output Controller unit tests (2h)
- MQTT + Web Server unit tests (2h)

**Day 4**: Integration + Config Flow (Phases 4-5)
- Network integration tests (2h)
- Config flow integration tests (2h)

**Day 5**: Performance + QA (Phase 6)
- Performance tests (2h)
- QA scenarios + hardware validation (2h)

---

## 7. Continuous Integration (CI) Strategy

### 7.1 CI Pipeline Configuration

**File**: `.github/workflows/platformio-ci.yml`

```yaml
name: PlatformIO CI

on:
  push:
    branches: [ master, develop ]
  pull_request:
    branches: [ master ]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      
      - name: Install PlatformIO
        run: |
          python -m pip install --upgrade pip
          pip install platformio
      
      - name: Run Unit Tests (Native)
        run: pio test -e native
      
      - name: Build for ESP32
        run: pio run -e esp32dev
      
      - name: Upload Coverage
        uses: codecov/codecov-action@v3
        with:
          files: .pio/build/native/coverage.xml
```

### 7.2 Test Environments

**platformio.ini** configuration:

```ini
[env:native]
platform = native
test_framework = unity
test_build_src = yes
build_flags = 
    -std=c++17
    -DUNIT_TEST
    -fprofile-arcs
    -ftest-coverage
lib_deps =
    bblanchon/ArduinoJson@^6.21.3

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
test_framework = unity
upload_speed = 921600
monitor_speed = 115200
lib_deps =
    ottowinter/ESPAsyncWebServer-esphome@^3.0.0
    marvinroger/AsyncMqttClient@^0.9.0
    adafruit/RTClib@^2.1.1
    bblanchon/ArduinoJson@^6.21.3
```

### 7.3 CI Quality Gates

**PR Merge Requirements**:
- ✅ All unit tests pass (74/74)
- ✅ All integration tests pass (26/26)
- ✅ Code coverage >80% (target: 90%)
- ✅ No compiler warnings
- ✅ Memory budget <420KB RAM
- ✅ Build size <4MB Flash
- ✅ At least 1 approved review

**Automated Checks**:
1. **Traceability**: Every test links to requirement/design issue
2. **Coverage**: Line coverage >80%, branch coverage >70%
3. **Performance**: No regressions (CPU, memory, latency)
4. **Style**: clang-format, cpplint pass

---

## 8. Test Data & Fixtures

### 8.1 Audio Test Fixtures

**File**: `test/fixtures/audio_samples.h`

```cpp
// 100 claps at 140 BPM (428ms intervals)
const uint64_t CLAPS_140BPM[] = {
    0,       428571,  857142,  1285713, 1714284,  // 0-4
    2142855, 2571426, 2999997, 3428568, 3857139,  // 5-9
    // ... 90 more entries
};

// Irregular beats (tempo changes)
const uint64_t IRREGULAR_BEATS[] = {
    0, 500000, 900000, 1250000, 1700000, // 120 → 133 → 150 → 130 BPM
};

// Kick-only samples (rise time >4ms)
const uint16_t KICK_WAVEFORM[] = {
    512, 600, 800, 1200, 1800, 2600, 3400, 4095, 3800, 3200, // 10ms rise
};
```

### 8.2 BPM Test Fixtures

**File**: `test/fixtures/bpm_sequences.h`

```cpp
// Exact 140 BPM (for QA-SC-002)
const float EXACT_140BPM_INTERVALS[] = {
    428571.4285, // 60,000,000 µs / 140 BPM
};

// Half-tempo sequence (for QA-SC-003)
const uint64_t HALF_TEMPO_TAPS[] = {
    0, 857142, 1714284, 2571426, 3428568, 4285710, // 70 BPM
    // After 6th tap, engine should detect and correct to 140 BPM
};
```

---

## 9. Mock Library Utilities

### 9.1 FreeRTOS Mocks (for Native Testing)

**File**: `test/mocks/freertos_mock.h`

```cpp
#ifdef UNIT_TEST
// Mock FreeRTOS functions for native testing
#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms) (ms)
#define vTaskDelay(ticks) usleep((ticks) * 1000)
#define xTaskGetTickCount() (millis())

inline void IRAM_ATTR mockISR() { /* No-op for native */ }
#endif
```

### 9.2 Arduino Mocks

**File**: `test/mocks/arduino_mock.cpp`

```cpp
#ifdef UNIT_TEST
#include <chrono>

static auto start_time = std::chrono::steady_clock::now();

unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    return duration.count();
}

unsigned long micros() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
    return duration.count();
}

void delay(unsigned long ms) {
    usleep(ms * 1000);
}
#endif
```

---

## 10. Verification Checklist

**Phase 04 → Phase 05 Transition**:

- ✅ All 7 component designs documented
- ✅ All 13 interfaces specified
- ✅ All 7 data models defined
- ✅ Traceability matrix complete
- ✅ TDD plan created (this document)
- ✅ Mock/stub interfaces defined
- ✅ Acceptance criteria specified (98 criteria)
- ✅ Integration test scenarios defined (26 tests)
- ✅ Test execution order planned (6 phases)
- ✅ CI/CD pipeline configured
- ⏳ Design review scheduled
- ⏳ XP pair programming assignments made

**Ready for Phase 05 Implementation**: ✅

---

## 11. References

**GitHub Issues**:
- #44: DES-C-005 (Timing Manager)
- #45: DES-C-001 (Audio Detection)
- #46: DES-C-002 (BPM Calculation)
- #47: DES-C-006 (Configuration)
- #48: DES-C-007 (MQTT)
- #49: DES-C-003 (Web Server)
- #50: DES-C-004 (Output Controller)

**Standards**:
- ISO/IEC/IEEE 12207:2017 - Implementation Process
- XP Test-First Programming - Kent Beck
- IEEE 1012-2016 - Verification & Validation

**Related Documents**:
- `04-design/phase-04-traceability-matrix.md` - Traceability
- `.github/copilot-instructions.md` - XP practices
- `docs/xp-practices.md` - TDD guidelines

---

**End of TDD Plan**
