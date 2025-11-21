# TDD Plan: MQTT Telemetry Client (Wave 3.7)

**Phase**: 05-implementation  
**Component**: ARC-C-007 (MQTT Telemetry Client)  
**Requirement**: REQ-F-MQTT-001  
**Created**: 2025-11-21  
**Standard**: ISO/IEC/IEEE 12207:2017 (Implementation Process) + XP TDD Practices

---

## 🎯 Objectives

Implement MQTT telemetry publishing following Test-Driven Development (TDD):
1. **Red** → Write failing test
2. **Green** → Write minimal code to pass
3. **Refactor** → Improve design while keeping tests green

**Success Criteria**:
- ✅ All unit tests passing (test_mqtt_client/)
- ✅ All ESP32 integration tests passing (test_mqtt_integration/)
- ✅ Published telemetry viewable in MQTT Explorer
- ✅ Main loop overhead <10ms

---

## 📋 TDD Cycles Overview

| Cycle | Feature | Tests | Estimated Time |
|-------|---------|-------|----------------|
| 1 | MQTTClient class structure | test_mqtt_001_initialization | 30 min |
| 2 | Connection establishment | test_mqtt_002_connection | 45 min |
| 3 | BPM telemetry publishing | test_mqtt_003_bpm_publish | 45 min |
| 4 | Audio telemetry publishing | test_mqtt_004_audio_publish | 30 min |
| 5 | System status publishing | test_mqtt_005_status_publish | 30 min |
| 6 | Last Will Testament (LWT) | test_mqtt_006_lwt | 30 min |
| 7 | Reconnection logic | test_mqtt_007_reconnect | 45 min |
| 8 | Error handling | test_mqtt_008_error_handling | 30 min |
| 9 | ConfigurationManager integration | test_mqtt_009_config | 30 min |
| 10 | ESP32 integration test | test_mqtt_integration | 60 min |

**Total Estimated**: ~6 hours

---

## 🔄 TDD Cycle 01: Class Initialization

### 🔴 Red: Write Failing Test

**File**: `test/test_mqtt_client/test_mqtt_basic.cpp`

```cpp
#include <unity.h>
#include "mqtt/MQTTClient.h"
#include "config/ConfigurationManager.h"

void test_mqtt_001_initialization(void) {
    // Arrange
    ConfigurationManager config;
    MQTTConfig mqtt_config = config.getMQTTConfig();
    
    // Act
    MQTTClient mqtt_client(&mqtt_config);
    
    // Assert
    TEST_ASSERT_FALSE(mqtt_client.isConnected());
    TEST_ASSERT_EQUAL_STRING("", mqtt_client.getLastError());
}

void setUp(void) {
    // Setup runs before each test
}

void tearDown(void) {
    // Cleanup runs after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_001_initialization);
    UNITY_END();
    return 0;
}
```

**Expected**: ❌ Compilation fails - MQTTClient class doesn't exist

---

### 🟢 Green: Make Test Pass

**File**: `src/mqtt/MQTTClient.h`

```cpp
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include "config/ConfigurationManager.h"

class MQTTClient {
public:
    explicit MQTTClient(MQTTConfig* config);
    ~MQTTClient();
    
    bool isConnected() const { return connected_; }
    const char* getLastError() const { return last_error_.c_str(); }

private:
    MQTTConfig* config_;
    bool connected_;
    String last_error_;
};

#endif  // MQTT_CLIENT_H
```

**File**: `src/mqtt/MQTTClient.cpp`

```cpp
#include "mqtt/MQTTClient.h"

MQTTClient::MQTTClient(MQTTConfig* config) 
    : config_(config), connected_(false), last_error_("") {
}

MQTTClient::~MQTTClient() {
}
```

**Result**: ✅ Test passes

---

### 🔵 Refactor: Improve Design

- Add null pointer check for config
- Add documentation comments
- Consider dependency injection for WiFiClient

---

## 🔄 TDD Cycle 02: Connection Establishment

### 🔴 Red: Write Failing Test

```cpp
void test_mqtt_002_connection(void) {
    // Arrange
    ConfigurationManager config;
    MQTTConfig mqtt_config = config.getMQTTConfig();
    mqtt_config.enabled = true;
    mqtt_config.broker_host = "test.mosquitto.org";
    mqtt_config.broker_port = 1883;
    mqtt_config.device_id = "test-device-001";
    
    MQTTClient mqtt_client(&mqtt_config);
    
    // Act
    bool connected = mqtt_client.connect();
    
    // Assert
    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_TRUE(mqtt_client.isConnected());
    TEST_ASSERT_EQUAL_STRING("", mqtt_client.getLastError());
}
```

**Expected**: ❌ `connect()` method doesn't exist

---

### 🟢 Green: Make Test Pass

**File**: `src/mqtt/MQTTClient.h` (add):

```cpp
#include <MQTT.h>
#include <WiFiClient.h>

class MQTTClient {
public:
    bool connect();
    void disconnect();
    void loop();  // Call in main loop for keep-alive

private:
    WiFiClient wifi_client_;
    ::MQTTClient mqtt_;  // Use :: to disambiguate from class name
};
```

**File**: `src/mqtt/MQTTClient.cpp` (add):

```cpp
bool MQTTClient::connect() {
    if (!config_->enabled) {
        last_error_ = "MQTT disabled in config";
        return false;
    }
    
    if (config_->broker_host.isEmpty()) {
        last_error_ = "Broker host not configured";
        return false;
    }
    
    // Initialize MQTT client
    mqtt_.begin(config_->broker_host.c_str(), config_->broker_port, wifi_client_);
    
    // Set Last Will Testament
    String lwt_topic = String(config_->device_id) + "/status/online";
    mqtt_.setWill(lwt_topic.c_str(), "{\"online\":false}", true, config_->qos);
    
    // Attempt connection
    connected_ = mqtt_.connect(config_->device_id.c_str(), 
                                config_->username.c_str(), 
                                config_->password.c_str());
    
    if (!connected_) {
        last_error_ = "Connection failed";
        return false;
    }
    
    // Publish online status
    String online_msg = "{\"online\":true}";
    mqtt_.publish(lwt_topic.c_str(), online_msg.c_str(), true, config_->qos);
    
    last_error_ = "";
    return true;
}

void MQTTClient::disconnect() {
    if (connected_) {
        mqtt_.disconnect();
        connected_ = false;
    }
}

void MQTTClient::loop() {
    mqtt_.loop();
}
```

**Result**: ✅ Test passes (when WiFi connected and broker reachable)

---

## 🔄 TDD Cycle 03: BPM Telemetry Publishing

### 🔴 Red: Write Failing Test

```cpp
void test_mqtt_003_bpm_publish(void) {
    // Arrange
    ConfigurationManager config;
    MQTTConfig mqtt_config = config.getMQTTConfig();
    mqtt_config.enabled = true;
    mqtt_config.broker_host = "test.mosquitto.org";
    mqtt_config.broker_port = 1883;
    mqtt_config.device_id = "test-device-001";
    
    MQTTClient mqtt_client(&mqtt_config);
    mqtt_client.connect();
    
    // Act
    bool published = mqtt_client.publishBPM(120.5, 95, "LOCKED", 1700000000);
    
    // Assert
    TEST_ASSERT_TRUE(published);
    
    // Verify: Subscribe to topic and check message (manual verification)
    // Expected: {"bpm":120.5,"confidence":95,"lock_status":"LOCKED","timestamp":1700000000}
}
```

**Expected**: ❌ `publishBPM()` method doesn't exist

---

### 🟢 Green: Make Test Pass

**File**: `src/mqtt/MQTTClient.h` (add):

```cpp
#include <ArduinoJson.h>

class MQTTClient {
public:
    bool publishBPM(float bpm, uint8_t confidence, const char* lock_status, uint32_t timestamp);
};
```

**File**: `src/mqtt/MQTTClient.cpp` (add):

```cpp
bool MQTTClient::publishBPM(float bpm, uint8_t confidence, const char* lock_status, uint32_t timestamp) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }
    
    // Build topic
    String topic = String(config_->device_id) + "/telemetry/bpm";
    
    // Build JSON payload
    StaticJsonDocument<256> doc;
    doc["bpm"] = bpm;
    doc["confidence"] = confidence;
    doc["lock_status"] = lock_status;
    doc["timestamp"] = timestamp;
    
    String payload;
    serializeJson(doc, payload);
    
    // Publish
    bool success = mqtt_.publish(topic.c_str(), payload.c_str(), false, config_->qos);
    
    if (!success) {
        last_error_ = "Publish failed";
    }
    
    return success;
}
```

**Result**: ✅ Test passes

---

## 🔄 TDD Cycle 04: Audio Telemetry Publishing

### 🔴 Red: Write Failing Test

```cpp
void test_mqtt_004_audio_publish(void) {
    // Arrange
    MQTTClient mqtt_client(&mqtt_config);
    mqtt_client.connect();
    
    // Act
    bool published = mqtt_client.publishAudioLevels(420, 680, 45, 120, 1);
    
    // Assert
    TEST_ASSERT_TRUE(published);
}
```

**Expected**: ❌ `publishAudioLevels()` doesn't exist

---

### 🟢 Green: Make Test Pass

```cpp
bool MQTTClient::publishAudioLevels(uint16_t current, uint16_t max, uint16_t min, 
                                     uint16_t threshold, uint8_t gain) {
    if (!connected_) return false;
    
    String topic = String(config_->device_id) + "/telemetry/audio";
    
    StaticJsonDocument<256> doc;
    doc["current_level"] = current;
    doc["max_level"] = max;
    doc["min_level"] = min;
    doc["threshold"] = threshold;
    doc["gain"] = gain;
    
    String payload;
    serializeJson(doc, payload);
    
    return mqtt_.publish(topic.c_str(), payload.c_str(), false, config_->qos);
}
```

---

## 🔄 TDD Cycle 05: System Status Publishing

### 🔴 Red: Write Failing Test

```cpp
void test_mqtt_005_status_publish(void) {
    // Arrange
    MQTTClient mqtt_client(&mqtt_config);
    mqtt_client.connect();
    
    // Act
    bool published = mqtt_client.publishSystemStatus(7200, 180000, -45, "TestNetwork");
    
    // Assert
    TEST_ASSERT_TRUE(published);
}
```

---

### 🟢 Green: Make Test Pass

```cpp
bool MQTTClient::publishSystemStatus(uint32_t uptime_sec, uint32_t free_heap, 
                                       int8_t wifi_rssi, const char* wifi_ssid) {
    if (!connected_) return false;
    
    String topic = String(config_->device_id) + "/status/system";
    
    StaticJsonDocument<256> doc;
    doc["uptime_sec"] = uptime_sec;
    doc["free_heap"] = free_heap;
    doc["wifi_rssi"] = wifi_rssi;
    doc["wifi_ssid"] = wifi_ssid;
    
    String payload;
    serializeJson(doc, payload);
    
    return mqtt_.publish(topic.c_str(), payload.c_str(), false, 1);  // QoS 1 for status
}
```

---

## 🔄 TDD Cycle 07: Reconnection Logic

### 🔴 Red: Write Failing Test

```cpp
void test_mqtt_007_reconnect(void) {
    // Arrange
    MQTTClient mqtt_client(&mqtt_config);
    mqtt_client.connect();
    TEST_ASSERT_TRUE(mqtt_client.isConnected());
    
    // Act: Simulate disconnect
    mqtt_client.disconnect();
    TEST_ASSERT_FALSE(mqtt_client.isConnected());
    
    // Reconnect
    bool reconnected = mqtt_client.connect();
    
    // Assert
    TEST_ASSERT_TRUE(reconnected);
    TEST_ASSERT_TRUE(mqtt_client.isConnected());
}
```

---

## 🔄 TDD Cycle 09: ConfigurationManager Integration

### 🔴 Red: Write Failing Test

```cpp
void test_mqtt_009_config(void) {
    // Arrange
    ConfigurationManager config;
    
    // Act: Update MQTT config
    MQTTConfig new_config;
    new_config.enabled = true;
    new_config.broker_host = "mqtt.example.com";
    new_config.broker_port = 1883;
    new_config.device_id = "esp32-test";
    
    bool saved = config.setMQTTConfig(new_config);
    TEST_ASSERT_TRUE(saved);
    
    // Verify
    MQTTConfig loaded = config.getMQTTConfig();
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_STRING("mqtt.example.com", loaded.broker_host.c_str());
}
```

**Expected**: ❌ `getMQTTConfig()` / `setMQTTConfig()` don't exist

---

### 🟢 Green: Make Test Pass

**File**: `src/config/ConfigurationManager.h` (add):

```cpp
struct MQTTConfig {
    bool enabled = false;
    String broker_host = "";
    uint16_t broker_port = 1883;
    String username = "";
    String password = "";
    String device_id = "";
    uint16_t telemetry_interval_ms = 5000;
    uint16_t status_interval_ms = 60000;
    uint8_t qos = 0;
    bool use_tls = false;
};

class ConfigurationManager {
public:
    MQTTConfig getMQTTConfig() const;
    bool setMQTTConfig(const MQTTConfig& config);

private:
    MQTTConfig mqtt_config_;
};
```

**File**: `src/config/ConfigurationManager.cpp` (add):

```cpp
MQTTConfig ConfigurationManager::getMQTTConfig() const {
    return mqtt_config_;
}

bool ConfigurationManager::setMQTTConfig(const MQTTConfig& config) {
    mqtt_config_ = config;
    notifyChange(ConfigChangeEvent::Section::MQTT);
    return true;  // Validation can be added later
}
```

---

## 🔄 TDD Cycle 10: ESP32 Integration Test

### Test File Structure

**File**: `test/test_mqtt_integration/test_mqtt_integration.cpp`

```cpp
#include <unity.h>
#include <WiFi.h>
#include "mqtt/MQTTClient.h"
#include "config/ConfigurationManager.h"
#include "credentials.h"  // Contains WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER

ConfigurationManager* config_manager = nullptr;
MQTTClient* mqtt_client = nullptr;

void setUp(void) {
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(100);
    }
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    
    // Configure MQTT
    config_manager = new ConfigurationManager();
    MQTTConfig mqtt_config;
    mqtt_config.enabled = true;
    mqtt_config.broker_host = MQTT_BROKER;  // e.g., "test.mosquitto.org"
    mqtt_config.broker_port = 1883;
    mqtt_config.device_id = "esp32-test-" + String(ESP.getEfuseMac(), HEX);
    config_manager->setMQTTConfig(mqtt_config);
    
    // Create MQTT client
    mqtt_client = new MQTTClient(config_manager->getMQTTConfig());
    delay(1000);
}

void tearDown(void) {
    if (mqtt_client) {
        mqtt_client->disconnect();
        delete mqtt_client;
    }
    if (config_manager) {
        delete config_manager;
    }
}

void test_mqtt_int_001_connection(void) {
    TEST_ASSERT_TRUE_MESSAGE(mqtt_client->connect(), "Failed to connect to MQTT broker");
    TEST_ASSERT_TRUE(mqtt_client->isConnected());
}

void test_mqtt_int_002_bpm_publish(void) {
    mqtt_client->connect();
    TEST_ASSERT_TRUE(mqtt_client->isConnected());
    
    bool published = mqtt_client->publishBPM(120.5, 95, "LOCKED", millis() / 1000);
    TEST_ASSERT_TRUE_MESSAGE(published, "Failed to publish BPM telemetry");
}

void test_mqtt_int_003_audio_publish(void) {
    mqtt_client->connect();
    
    bool published = mqtt_client->publishAudioLevels(420, 680, 45, 120, 1);
    TEST_ASSERT_TRUE_MESSAGE(published, "Failed to publish audio levels");
}

void test_mqtt_int_004_status_publish(void) {
    mqtt_client->connect();
    
    bool published = mqtt_client->publishSystemStatus(
        millis() / 1000,
        ESP.getFreeHeap(),
        WiFi.RSSI(),
        WiFi.SSID().c_str()
    );
    TEST_ASSERT_TRUE_MESSAGE(published, "Failed to publish system status");
}

void test_mqtt_int_005_reconnect(void) {
    mqtt_client->connect();
    TEST_ASSERT_TRUE(mqtt_client->isConnected());
    
    mqtt_client->disconnect();
    TEST_ASSERT_FALSE(mqtt_client->isConnected());
    
    delay(1000);
    
    bool reconnected = mqtt_client->connect();
    TEST_ASSERT_TRUE_MESSAGE(reconnected, "Failed to reconnect");
}

void test_mqtt_int_summary(void) {
    Serial.println("\n=== MQTT Integration Test Summary ===");
    Serial.println("✓ All MQTT integration tests passed");
    Serial.println("✓ Connection, publish, and reconnect working");
    TEST_PASS();
}

void setup() {
    delay(2000);  // Allow serial monitor to open
    UNITY_BEGIN();
    
    RUN_TEST(test_mqtt_int_001_connection);
    RUN_TEST(test_mqtt_int_002_bpm_publish);
    RUN_TEST(test_mqtt_int_003_audio_publish);
    RUN_TEST(test_mqtt_int_004_status_publish);
    RUN_TEST(test_mqtt_int_005_reconnect);
    RUN_TEST(test_mqtt_int_summary);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
```

---

## 📊 Test Coverage Goals

| Component | Target Coverage | Critical Paths |
|-----------|----------------|----------------|
| MQTTClient.cpp | 85% | connect(), publish*(), loop() |
| ConfigurationManager (MQTT) | 90% | get/setMQTTConfig() |
| Integration | 100% | All publish paths |

---

## 🚀 Implementation Order

**Session 1** (2 hours):
1. ✅ Create requirements document (REQ-F-MQTT-001)
2. ✅ Create TDD plan (this document)
3. 🔄 TDD Cycle 01: Class initialization
4. 🔄 TDD Cycle 02: Connection

**Session 2** (2 hours):
5. TDD Cycle 03: BPM publishing
6. TDD Cycle 04: Audio publishing
7. TDD Cycle 05: System status

**Session 3** (2 hours):
8. TDD Cycle 07: Reconnection
9. TDD Cycle 09: Config integration
10. TDD Cycle 10: ESP32 integration test

---

## 🧪 Testing Strategy

### Unit Tests (Native)
- Run on CI without hardware
- Mock WiFiClient if needed
- Test JSON serialization
- Test error conditions

### Integration Tests (ESP32)
- Requires real MQTT broker (test.mosquitto.org or local Mosquitto)
- Requires WiFi connectivity
- Manual verification: `mosquitto_sub -h test.mosquitto.org -t 'esp32-test-+/telemetry/#' -v`

---

## 📝 Completion Criteria

**Wave 3.7 Complete When**:
- [ ] All unit tests passing (test_mqtt_client/)
- [ ] All ESP32 integration tests passing (5/5)
- [ ] MQTTClient integrated into main.cpp
- [ ] Telemetry published every 5s (BPM, audio)
- [ ] System status published every 60s
- [ ] Main loop overhead <10ms
- [ ] Documentation updated (README, API docs)
- [ ] Manual verification: Telemetry visible in MQTT Explorer

---

**Created**: 2025-11-21  
**Author**: GitHub Copilot  
**Phase**: 05-implementation (Wave 3.7)
