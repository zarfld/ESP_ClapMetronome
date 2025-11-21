/**
 * @file MQTTClient.cpp
 * @brief MQTT Telemetry Client Implementation
 * 
 * @component ARC-C-007: MQTT Telemetry Client
 * @implements REQ-F-MQTT-001 (MQTT Telemetry Publishing)
 * 
 * @date 2025-11-21
 */

#include "mqtt/MQTTClient.h"

#ifndef NATIVE_BUILD
#include <WiFi.h>
#endif

namespace clap_metronome {

// ============================================================================
// Constructor / Destructor
// ============================================================================

MQTTClient::MQTTClient(const MQTTConfig* config)
    : config_(config),
      connected_(false),
      last_error_(""),
      last_reconnect_attempt_(0)
#ifndef NATIVE_BUILD
      , mqtt_(256)  // Buffer size for MQTT messages
#endif
{
    if (!config_) {
        last_error_ = "Config pointer is null";
    }
}

MQTTClient::~MQTTClient() {
    disconnect();
}

// ============================================================================
// Connection Management
// ============================================================================

bool MQTTClient::connect() {
#ifdef NATIVE_BUILD
    // Native build - mock connection for unit tests
    if (!config_ || !config_->enabled) {
        last_error_ = "MQTT disabled in config";
        return false;
    }
    
    if (config_->broker_host.empty()) {
        last_error_ = "Broker host not configured";
        return false;
    }
    
    connected_ = true;
    last_error_ = "";
    return true;
#else
    // ESP32/ESP8266 build - real MQTT connection
    if (!config_ || !config_->enabled) {
        last_error_ = "MQTT disabled in config";
        return false;
    }
    
    if (config_->broker_host.empty()) {
        last_error_ = "Broker host not configured";
        return false;
    }
    
    // Check WiFi connectivity
    if (WiFi.status() != WL_CONNECTED) {
        last_error_ = "WiFi not connected";
        return false;
    }
    
    // Initialize MQTT client
    mqtt_.begin(config_->broker_host.c_str(), config_->broker_port, wifi_client_);
    
    // Set Last Will Testament
    setLastWill();
    
    // Attempt connection
    bool success = false;
    if (config_->username.empty()) {
        // Connect without authentication
        success = mqtt_.connect(config_->device_id.c_str());
    } else {
        // Connect with authentication
        success = mqtt_.connect(config_->device_id.c_str(),
                                 config_->username.c_str(),
                                 config_->password.c_str());
    }
    
    if (!success) {
        last_error_ = "Connection failed - broker unreachable or credentials invalid";
        connected_ = false;
        return false;
    }
    
    connected_ = true;
    last_error_ = "";
    
    // Publish online status
    publishOnlineStatus(true);
    
    return true;
#endif
}

void MQTTClient::disconnect() {
#ifndef NATIVE_BUILD
    if (connected_) {
        // Publish offline status before disconnecting
        publishOnlineStatus(false);
        mqtt_.disconnect();
    }
#endif
    connected_ = false;
}

bool MQTTClient::isConnected() const {
    return connected_;
}

void MQTTClient::loop() {
#ifndef NATIVE_BUILD
    if (!config_ || !config_->enabled) {
        return;
    }
    
    mqtt_.loop();
    
    // Check if still connected
    if (!mqtt_.connected()) {
        connected_ = false;
        
        // Attempt reconnection every 30 seconds
        uint32_t now = millis();
        if (now - last_reconnect_attempt_ > 30000) {
            last_reconnect_attempt_ = now;
            Serial.println("[MQTT] Attempting reconnection...");
            connect();
        }
    } else {
        connected_ = true;
    }
#endif
}

// ============================================================================
// Telemetry Publishing
// ============================================================================

bool MQTTClient::publishBPM(float bpm, uint8_t confidence, const char* lock_status, uint32_t timestamp) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }
    
#ifdef NATIVE_BUILD
    // Native build - mock publish for unit tests
    return true;
#else
    // Build topic
    std::string topic = std::string(config_->device_id) + "/telemetry/bpm";
    
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
#endif
}

bool MQTTClient::publishAudioLevels(uint16_t current_level, uint16_t max_level, uint16_t min_level,
                                     uint16_t threshold, uint8_t gain) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }
    
#ifdef NATIVE_BUILD
    return true;
#else
    std::string topic = std::string(config_->device_id) + "/telemetry/audio";
    
    StaticJsonDocument<256> doc;
    doc["current_level"] = current_level;
    doc["max_level"] = max_level;
    doc["min_level"] = min_level;
    doc["threshold"] = threshold;
    doc["gain"] = gain;
    
    String payload;
    serializeJson(doc, payload);
    
    return mqtt_.publish(topic.c_str(), payload.c_str(), false, config_->qos);
#endif
}

bool MQTTClient::publishSystemStatus(uint32_t uptime_sec, uint32_t free_heap,
                                       int8_t wifi_rssi, const char* wifi_ssid) {
    if (!connected_) {
        last_error_ = "Not connected";
        return false;
    }
    
#ifdef NATIVE_BUILD
    return true;
#else
    std::string topic = std::string(config_->device_id) + "/status/system";
    
    StaticJsonDocument<256> doc;
    doc["uptime_sec"] = uptime_sec;
    doc["free_heap"] = free_heap;
    doc["wifi_rssi"] = wifi_rssi;
    doc["wifi_ssid"] = wifi_ssid;
    
    String payload;
    serializeJson(doc, payload);
    
    // Use QoS 1 for status messages (at least once delivery)
    return mqtt_.publish(topic.c_str(), payload.c_str(), false, 1);
#endif
}

// ============================================================================
// Error Handling
// ============================================================================

const char* MQTTClient::getLastError() const {
    return last_error_.c_str();
}

// ============================================================================
// Private Methods
// ============================================================================

void MQTTClient::setLastWill() {
#ifndef NATIVE_BUILD
    std::string lwt_topic = std::string(config_->device_id) + "/status/online";
    std::string lwt_payload = "{\"online\":false}";
    mqtt_.setWill(lwt_topic.c_str(), lwt_payload.c_str(), true, config_->qos);
#endif
}

bool MQTTClient::publishOnlineStatus(bool online) {
#ifdef NATIVE_BUILD
    return true;
#else
    std::string topic = std::string(config_->device_id) + "/status/online";
    
    StaticJsonDocument<64> doc;
    doc["online"] = online;
    
    String payload;
    serializeJson(doc, payload);
    
    // Use retained message so subscribers get status immediately
    return mqtt_.publish(topic.c_str(), payload.c_str(), true, config_->qos);
#endif
}

}  // namespace clap_metronome
