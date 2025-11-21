/**
 * @file MQTTClient.h
 * @brief MQTT Telemetry Client - Publishes system telemetry to MQTT broker
 * 
 * @component ARC-C-007: MQTT Telemetry Client
 * @implements REQ-F-MQTT-001 (MQTT Telemetry Publishing)
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Implementation)
 * @standard IEEE 1016-2009 (Software Design Documentation)
 * 
 * @description
 * Publishes BPM telemetry, audio levels, and system status to MQTT broker.
 * Supports QoS 0/1, Last Will Testament (LWT), and automatic reconnection.
 * 
 * Key Features:
 * - Non-blocking operation (<10ms per loop())
 * - QoS 0 for telemetry (at most once)
 * - QoS 1 for status messages (at least once)
 * - Last Will Testament for offline detection
 * - Automatic reconnection (30s interval)
 * - JSON payload format (compact, human-readable)
 * 
 * Topic Structure:
 *   <device-id>/telemetry/bpm       # BPM and confidence
 *   <device-id>/telemetry/audio     # Audio signal levels
 *   <device-id>/status/system       # System health
 *   <device-id>/status/online       # Online/offline (LWT)
 * 
 * @see GitHub Issue #TBD (ARC-C-007)
 * @see 02-requirements/functional/REQ-F-MQTT-001-telemetry-publishing.md
 * @date 2025-11-21
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <MQTT.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#endif

#include <cstdint>
#include <string>

namespace clap_metronome {

/**
 * @brief MQTT configuration structure
 * 
 * Extends NetworkConfig with MQTT-specific settings.
 * Stored in ConfigurationManager.
 */
struct MQTTConfig {
    bool enabled;                    ///< MQTT enabled, default: false
    std::string broker_host;         ///< Broker hostname/IP, default: ""
    uint16_t broker_port;            ///< Broker port, default: 1883
    std::string username;            ///< MQTT username, default: ""
    std::string password;            ///< MQTT password (stored encrypted), default: ""
    std::string device_id;           ///< Unique device identifier, default: "esp32-<chip-id>"
    uint16_t telemetry_interval_ms;  ///< Telemetry publish interval (ms), default: 5000
    uint16_t status_interval_ms;     ///< Status publish interval (ms), default: 60000
    uint8_t qos;                     ///< QoS level for telemetry (0-2), default: 0
    bool use_tls;                    ///< Use TLS/SSL encryption, default: false
    
    // Factory defaults
    static constexpr bool DEFAULT_ENABLED = false;
    static constexpr uint16_t DEFAULT_BROKER_PORT = 1883;
    static constexpr uint16_t DEFAULT_TELEMETRY_INTERVAL_MS = 5000;
    static constexpr uint16_t DEFAULT_STATUS_INTERVAL_MS = 60000;
    static constexpr uint8_t DEFAULT_QOS = 0;
    static constexpr bool DEFAULT_USE_TLS = false;
    
    /**
     * @brief Constructor with defaults
     */
    MQTTConfig()
        : enabled(DEFAULT_ENABLED),
          broker_host(""),
          broker_port(DEFAULT_BROKER_PORT),
          username(""),
          password(""),
          device_id(""),
          telemetry_interval_ms(DEFAULT_TELEMETRY_INTERVAL_MS),
          status_interval_ms(DEFAULT_STATUS_INTERVAL_MS),
          qos(DEFAULT_QOS),
          use_tls(DEFAULT_USE_TLS) {}
};

/**
 * @brief MQTT Telemetry Client
 * 
 * @implements ARC-C-007 (MQTT Telemetry Client)
 * 
 * Publishes system telemetry to MQTT broker with automatic reconnection.
 */
class MQTTClient {
public:
    /**
     * @brief Constructor
     * 
     * @param config Pointer to MQTT configuration
     */
    explicit MQTTClient(const MQTTConfig* config);
    
    /**
     * @brief Destructor
     */
    ~MQTTClient();
    
    // ========================================================================
    // Connection Management
    // ========================================================================
    
    /**
     * @brief Connect to MQTT broker
     * 
     * @return true if connection successful
     * @return false if connection failed
     * 
     * @implements AC-MQTT-001 (Connect within 10s)
     * 
     * Establishes connection with broker, sets Last Will Testament,
     * and publishes online status.
     */
    bool connect();
    
    /**
     * @brief Disconnect from MQTT broker
     * 
     * Gracefully disconnects and cleans up resources.
     */
    void disconnect();
    
    /**
     * @brief Check if connected to broker
     * 
     * @return true if connected
     * @return false if disconnected
     */
    bool isConnected() const;
    
    /**
     * @brief Call in main loop for keep-alive and reconnection
     * 
     * Must be called frequently (every loop iteration) to:
     * - Process incoming messages
     * - Send MQTT keep-alive pings
     * - Attempt reconnection if disconnected
     * 
     * @note Non-blocking, completes in <10ms
     */
    void loop();
    
    // ========================================================================
    // Telemetry Publishing
    // ========================================================================
    
    /**
     * @brief Publish BPM telemetry
     * 
     * @param bpm Current BPM value
     * @param confidence Confidence level (0-100%)
     * @param lock_status Lock status string ("LOCKED", "UNLOCKING", "UNLOCKED")
     * @param timestamp Unix timestamp (seconds since epoch)
     * @return true if publish successful
     * @return false if publish failed or not connected
     * 
     * @implements AC-MQTT-002 (BPM telemetry publishing)
     * 
     * Publishes to topic: <device-id>/telemetry/bpm
     * Payload: {"bpm":120.5,"confidence":95,"lock_status":"LOCKED","timestamp":1700000000}
     */
    bool publishBPM(float bpm, uint8_t confidence, const char* lock_status, uint32_t timestamp);
    
    /**
     * @brief Publish audio signal levels
     * 
     * @param current_level Current audio level (0-4095)
     * @param max_level Maximum level since last publish
     * @param min_level Minimum level since last publish
     * @param threshold Current threshold value
     * @param gain Current gain level (dB)
     * @return true if publish successful
     * @return false if publish failed or not connected
     * 
     * @implements AC-MQTT-003 (Audio telemetry publishing)
     * 
     * Publishes to topic: <device-id>/telemetry/audio
     * Payload: {"current_level":420,"max_level":680,"min_level":45,"threshold":120,"gain":1}
     */
    bool publishAudioLevels(uint16_t current_level, uint16_t max_level, uint16_t min_level, 
                             uint16_t threshold, uint8_t gain);
    
    /**
     * @brief Publish system status
     * 
     * @param uptime_sec System uptime in seconds
     * @param free_heap Free heap memory in bytes
     * @param wifi_rssi WiFi signal strength (dBm)
     * @param wifi_ssid WiFi network SSID
     * @return true if publish successful
     * @return false if publish failed or not connected
     * 
     * @implements AC-MQTT-004 (System status publishing)
     * 
     * Publishes to topic: <device-id>/status/system
     * Payload: {"uptime_sec":7200,"free_heap":180000,"wifi_rssi":-45,"wifi_ssid":"MyNetwork"}
     */
    bool publishSystemStatus(uint32_t uptime_sec, uint32_t free_heap, 
                             int8_t wifi_rssi, const char* wifi_ssid);
    
    // ========================================================================
    // Error Handling
    // ========================================================================
    
    /**
     * @brief Get last error message
     * 
     * @return const char* Error message string (empty if no error)
     */
    const char* getLastError() const;

private:
    const MQTTConfig* config_;       ///< Configuration reference
    bool connected_;                 ///< Connection status
    std::string last_error_;         ///< Last error message
    uint32_t last_reconnect_attempt_; ///< Last reconnection attempt timestamp (ms)
    
#ifndef NATIVE_BUILD
    WiFiClient wifi_client_;         ///< WiFi client for MQTT
    ::MQTTClient mqtt_;              ///< MQTT client instance (:: for namespace disambiguation)
#endif
    
    /**
     * @brief Set Last Will Testament
     * 
     * @implements AC-MQTT-005 (Last Will Testament)
     * 
     * Configures LWT message published by broker on unexpected disconnect.
     */
    void setLastWill();
    
    /**
     * @brief Publish online status
     * 
     * @param online true for online, false for offline
     * @return true if publish successful
     */
    bool publishOnlineStatus(bool online);
};

}  // namespace clap_metronome
