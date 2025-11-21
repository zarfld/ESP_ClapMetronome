# Web Server Tests - Wave 3.6 (DES-C-003)

**Component**: Web Server & WebSocket  
**Requirements**: REQ-F-003 (Web UI display)  
**GitHub Issue**: #49  
**TDD Cycles**: WEB-01 (Native), WEB-02 (ESP32)

## Test Structure

### WEB-01: Native Unit Tests (CI/CD)

**File**: `test_web_logic.cpp`  
**Platform**: Native (Windows/Linux/macOS)  
**Framework**: GoogleTest  
**Purpose**: Test business logic without hardware dependencies

**Coverage**:
- ✅ AC-WEB-007: Rate limiting (max 2Hz = 500ms interval)
- ✅ AC-WEB-008: Error handling (buffer overflow, graceful degradation)

**Run**:
```bash
# Windows
cd test
cmake --build build --config Debug --target test_web_logic
.\build\Debug\test_web_logic.exe

# Linux/macOS
cd test
cmake --build build --target test_web_logic
./build/test_web_logic
```

**Test Count**: 9 tests
- Rate Limiting: 3 tests
- Error Handling: 4 tests
- Integration Scenarios: 2 tests

**Status**: ✅ All 9 tests passing

---

### WEB-02: ESP32 Integration Tests (Hardware)

**File**: `test_web_integration.cpp`  
**Platform**: ESP32 only  
**Framework**: Unity (PlatformIO)  
**Purpose**: Test complete Web Server functionality with real hardware

**Coverage**:
- ✅ AC-WEB-001: Static file serving (LittleFS)
- ✅ AC-WEB-002: REST API GET /api/config
- ✅ AC-WEB-003: REST API POST /api/config
- ✅ AC-WEB-004: REST API POST /api/factory-reset
- ✅ AC-WEB-005: WebSocket connection
- ✅ AC-WEB-006: WebSocket BPM broadcast (4 clients)
- ✅ AC-WEB-009: Concurrent client performance (<50ms)
- ✅ AC-WEB-010: CPU usage monitoring (<10% avg, <15% peak)

**Test Count**: 9 tests
- Static File Serving: 1 test
- REST API Endpoints: 3 tests
- WebSocket Connection: 2 tests
- Performance Tests: 2 tests
- Summary: 1 test

**Status**: ⏳ Implementation pending

---

## Hardware Setup (ESP32 Tests)

### Prerequisites

1. **ESP32 Development Board**
   - ESP32-DevKitC or compatible
   - USB cable for programming/power
   - Sufficient flash (4MB recommended)

2. **WiFi Access Point**
   - SSID and password configured
   - Stable connection required
   - Local network access

3. **PlatformIO**
   - Installed via VS Code extension or CLI
   - ESP32 platform configured

4. **LittleFS Filesystem**
   - Test files (index.html, app.js, style.css)
   - Auto-created by test if missing
   - ~100KB flash recommended

### WiFi Configuration

Create `test/test_config.h` (optional, defaults provided):

```cpp
#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

// WiFi credentials for ESP32 tests
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"

// Optional: Override defaults
#define TEST_SERVER_PORT 80
#define TEST_WEBSOCKET_PATH "/ws"
#define TEST_TIMEOUT_MS 5000

#endif
```

**Security Note**: Add `test/test_config.h` to `.gitignore` to avoid committing credentials.

### LittleFS Preparation

Option 1: **Auto-create** (recommended for tests)
- Tests automatically create minimal test files
- Files: `index.html`, `app.js`, `style.css`
- Sufficient for integration testing

Option 2: **Manual upload** (for full web UI)
- Create `data/` folder in project root
- Add complete web UI files
- Upload with: `pio run -e esp32dev -t uploadfs`

---

## Running ESP32 Tests

### Step 1: Connect ESP32

```bash
# Connect ESP32 via USB
# Verify connection
pio device list
```

### Step 2: Configure WiFi

Edit `test/test_config.h` or use defaults (TestAP/testpassword).

### Step 3: Upload and Test

```bash
# Build and upload test firmware
pio test -e esp32dev

# Or verbose output
pio test -e esp32dev -v

# Monitor output
pio device monitor --baud 115200
```

### Step 4: Verify Results

Expected output:
```
========================================
ESP32 Web Server Integration Tests
TDD Cycle: WEB-02
========================================

[WiFi] Connecting to: YourSSID...
[WiFi] Connected! IP: 192.168.1.100

=== AC-WEB-001: Static File Serving ===
Testing URL: http://192.168.1.100/
Response code: 200
✓ AC-WEB-001: Static files served correctly

=== AC-WEB-002: REST API GET /api/config ===
GET: http://192.168.1.100/api/config
Response code: 200
✓ AC-WEB-002: GET /api/config returns complete JSON

...

========================================
9 Tests 0 Failures 0 Ignored
OK
========================================
```

---

## Test Categories

### Category 1: Static File Serving (1 test)

**Test**: `test_web_001_static_file_serving`

**Scenario**:
```
Given: Web server running with LittleFS mounted
 When: HTTP GET requests to /, /index.html, /app.js, /style.css
 Then: Files returned with correct content type and content
```

**Validation**:
- ✅ HTTP 200 response
- ✅ Correct Content-Type headers
- ✅ Expected content present

---

### Category 2: REST API Endpoints (3 tests)

#### Test 2: GET /api/config

**Test**: `test_web_002_rest_api_get_config`

**Scenario**:
```
Given: Web server running
 When: HTTP GET /api/config
 Then: Returns 200 with complete config JSON (audio, bpm, output, mqtt)
```

#### Test 3: POST /api/config

**Test**: `test_web_003_rest_api_post_config`

**Scenario**:
```
Given: Web server running
 When: HTTP POST /api/config with JSON body (min_bpm=80, max_bpm=200)
 Then: Config updated, verified with GET
```

#### Test 4: POST /api/factory-reset

**Test**: `test_web_004_rest_api_factory_reset`

**Scenario**:
```
Given: Modified configuration
 When: HTTP POST /api/factory-reset
 Then: Config reset to defaults (may reboot ESP32)
```

---

### Category 3: WebSocket Connection (2 tests)

#### Test 5: WebSocket Connection

**Test**: `test_web_005_websocket_connection`

**Scenario**:
```
Given: Web server running
 When: WebSocket client connects to ws://IP/ws
 Then: Connection established successfully
```

#### Test 6: WebSocket BPM Broadcast

**Test**: `test_web_006_websocket_bpm_broadcast`

**Scenario**:
```
Given: 4 WebSocket clients connected
 When: BPM update triggered (120.5 BPM, stable=true)
 Then: All 4 clients receive message within 500ms (rate limit)
```

**Validation**:
- ✅ All clients receive JSON: `{"bpm": 120.5, "stable": true}`
- ✅ Broadcast latency <500ms (2Hz rate limit)
- ✅ Message ordering preserved

---

### Category 4: Performance Tests (2 tests)

#### Test 7: Concurrent Clients Performance

**Test**: `test_web_009_concurrent_clients_performance`

**Scenario**:
```
Given: 4 WebSocket clients connected
 When: 10 BPM updates sent (respecting 500ms rate limit)
 Then: Average broadcast latency <50ms per update
```

**Metrics**:
- ✅ 10 broadcasts measured
- ✅ Average latency <50ms
- ✅ All clients receive all messages

#### Test 8: CPU Usage Monitoring

**Test**: `test_web_010_cpu_usage_monitoring`

**Scenario**:
```
Given: Web server running with 2 active WebSocket clients
 When: Monitoring CPU during normal operation (20 samples @ 50ms)
 Then: Average CPU <10%, peak <15%
```

**Measurement**: ESP32 cycle counting (xthal_get_ccount)

---

## Troubleshooting

### WiFi Connection Failed

**Symptoms**:
```
[WiFi] Connection FAILED
```

**Solutions**:
1. Verify SSID/password in `test/test_config.h`
2. Check WiFi signal strength (ESP32 near router)
3. Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
4. Check router MAC filtering/firewall

### LittleFS Mount Failed

**Symptoms**:
```
[LittleFS] Mount FAILED
```

**Solutions**:
1. Format LittleFS: Tests auto-format on mount fail
2. Check flash size: 4MB recommended, minimum 2MB
3. Verify partition table: `pio run -e esp32dev -t menuconfig`

### WebSocket Connection Timeout

**Symptoms**:
```
WebSocket connection failed
```

**Solutions**:
1. Verify server IP matches WiFi IP
2. Check firewall blocking port 80
3. Increase TEST_TIMEOUT_MS if network slow
4. Monitor server logs for errors

### CPU Usage Test Fails

**Symptoms**:
```
Average CPU usage exceeded 10%
```

**Solutions**:
1. Close other tasks (Bluetooth, etc.)
2. Check for background WiFi activity
3. Optimize WebServer loop() frequency
4. Profile with ESP32 tools: `pio run -e esp32dev-debug`

### Factory Reset Reboots ESP32

**Expected Behavior**: Some implementations reboot after factory reset.

**Handling**:
- Test allows for reboot (checks if GET fails after POST)
- Re-upload test firmware if needed: `pio test -e esp32dev`

---

## CI/CD Integration

### GitHub Actions Workflow

**Native Tests** (Run on every commit):
```yaml
- name: Run Native Web Tests
  run: |
    cd test
    cmake -B build
    cmake --build build --target test_web_logic
    ./build/test_web_logic
```

**ESP32 Tests** (Manual or scheduled):
- Requires self-hosted runner with ESP32
- Or skip in CI (mark as manual verification)

```yaml
- name: Skip ESP32 Tests (Hardware Required)
  run: echo "ESP32 tests require hardware - run locally with 'pio test -e esp32dev'"
```

---

## Test Coverage Summary

| AC ID | Description | Test File | Status |
|-------|-------------|-----------|--------|
| AC-WEB-001 | Static file serving | test_web_integration.cpp | ⏳ |
| AC-WEB-002 | REST GET /api/config | test_web_integration.cpp | ⏳ |
| AC-WEB-003 | REST POST /api/config | test_web_integration.cpp | ⏳ |
| AC-WEB-004 | REST POST /api/factory-reset | test_web_integration.cpp | ⏳ |
| AC-WEB-005 | WebSocket connection | test_web_integration.cpp | ⏳ |
| AC-WEB-006 | WebSocket BPM broadcast | test_web_integration.cpp | ⏳ |
| AC-WEB-007 | Rate limiting (2Hz) | test_web_logic.cpp | ✅ |
| AC-WEB-008 | Error handling | test_web_logic.cpp | ✅ |
| AC-WEB-009 | Concurrent performance | test_web_integration.cpp | ⏳ |
| AC-WEB-010 | CPU usage monitoring | test_web_integration.cpp | ⏳ |

**Legend**:
- ✅ Implemented and passing
- ⏳ Implementation pending (tests ready)

---

## Dependencies

### Native Tests (test_web_logic.cpp)

- GoogleTest ^1.14.0
- Standard C++17 library
- No ESP32 dependencies

### ESP32 Tests (test_web_integration.cpp)

**Libraries** (via PlatformIO):
```ini
lib_deps = 
    ESP Async WebServer @ ^1.2.3
    ESP AsyncTCP @ ^1.1.1
    WebSockets @ ^2.4.0
    ArduinoJson @ ^6.21.0
    LittleFS @ ^2.0.0
```

**Frameworks**:
- Arduino (ESP32)
- Unity (test framework)

---

## Next Steps

1. ✅ **WEB-01 Complete**: Native unit tests passing (9/9)
2. ⏳ **WEB-02 Pending**: Implement WebServer class
   - Create `src/web/WebServer.h`
   - Create `src/web/WebServer.cpp`
   - Integrate with ConfigManager
3. ⏳ **Hardware Testing**: Run ESP32 integration tests
4. ⏳ **Wave 3.7**: MQTT Telemetry Client (DES-C-004)

---

## Standards Compliance

- **ISO/IEC/IEEE 12207:2017**: Integration Process (Phase 05)
- **IEEE 1012-2016**: Verification and Validation
- **XP Practices**: Test-Driven Development (TDD RED-GREEN-REFACTOR)

---

**Last Updated**: 2025-11-21  
**TDD Cycle**: WEB-01 ✅ | WEB-02 ⏳  
**Total Tests**: 18 (9 native ✅ + 9 ESP32 ⏳)
