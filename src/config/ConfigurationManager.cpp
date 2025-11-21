/**
 * @file ConfigurationManager.cpp
 * @brief Configuration Manager Implementation
 * 
 * @component DES-C-006: Configuration Manager
 * @implements REQ-F-005, REQ-F-006, REQ-NF-003
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Implementation)
 * 
 * @description
 * Implementation of Configuration Manager with NVS persistence,
 * factory defaults, validation, and change notifications.
 * 
 * @date 2025-11-21
 */

#include "ConfigurationManager.h"
#include <cstring>

#ifdef NATIVE_BUILD
// Native build: NVS functions are stubbed
#else
// ESP32 build: Include actual NVS library
#include <nvs_flash.h>
#include <nvs.h>
#endif

namespace clap_metronome {

ConfigurationManager::ConfigurationManager()
    : change_callback_(nullptr) {
    // Constructor: Initialize with factory defaults
    loadDefaults();
}

ConfigurationManager::~ConfigurationManager() {
    // Destructor
}

// ============================================================================
// Initialization
// ============================================================================

bool ConfigurationManager::init() {
    // Initialize NVS
    if (!nvsInit()) {
        // NVS init failed, use defaults
        loadDefaults();
        return false;
    }
    
    // Try to load from NVS
    if (!nvsLoad()) {
        // Load failed, use defaults
        loadDefaults();
        return false;
    }
    
    return true;
}

// ============================================================================
// Configuration Access (Getters)
// ============================================================================

AudioConfig ConfigurationManager::getAudioConfig() const {
    return audio_config_;
}

BPMConfig ConfigurationManager::getBPMConfig() const {
    return bpm_config_;
}

OutputConfig ConfigurationManager::getOutputConfig() const {
    return output_config_;
}

NetworkConfig ConfigurationManager::getNetworkConfig() const {
    return network_config_;
}

// ============================================================================
// Configuration Modification (Setters)
// ============================================================================

bool ConfigurationManager::setAudioConfig(const AudioConfig& config) {
    // Validate
    if (!validateAudioConfig(config)) {
        return false;
    }
    
    // Set
    audio_config_ = config;
    
    // Notify
    notifyChange(ConfigChangeEvent::Section::AUDIO);
    
    return true;
}

bool ConfigurationManager::setBPMConfig(const BPMConfig& config) {
    // Validate
    if (!validateBPMConfig(config)) {
        return false;
    }
    
    // Set
    bpm_config_ = config;
    
    // Notify
    notifyChange(ConfigChangeEvent::Section::BPM);
    
    return true;
}

bool ConfigurationManager::setOutputConfig(const OutputConfig& config) {
    // Validate
    if (!validateOutputConfig(config)) {
        return false;
    }
    
    // Set
    output_config_ = config;
    
    // Notify
    notifyChange(ConfigChangeEvent::Section::OUTPUT);
    
    return true;
}

bool ConfigurationManager::setNetworkConfig(const NetworkConfig& config) {
    // Validate
    if (!validateNetworkConfig(config)) {
        return false;
    }
    
    // Set
    network_config_ = config;
    
    // Notify
    notifyChange(ConfigChangeEvent::Section::NETWORK);
    
    return true;
}

// ============================================================================
// Persistence
// ============================================================================

bool ConfigurationManager::saveConfig() {
    return nvsSave();
}

bool ConfigurationManager::loadConfig() {
    if (!nvsLoad()) {
        loadDefaults();
        return false;
    }
    return true;
}

// ============================================================================
// Factory Reset
// ============================================================================

void ConfigurationManager::factoryReset() {
    // Erase NVS
    nvsErase();
    
    // Load factory defaults
    loadDefaults();
    
    // Notify of complete reset
    notifyChange(ConfigChangeEvent::Section::ALL);
}

// ============================================================================
// Change Notifications
// ============================================================================

void ConfigurationManager::onConfigChange(std::function<void(const ConfigChangeEvent&)> callback) {
    change_callback_ = callback;
}

// ============================================================================
// Internal Helpers
// ============================================================================

void ConfigurationManager::loadDefaults() {
    // Audio defaults
    audio_config_.sample_rate = AudioConfig::DEFAULT_SAMPLE_RATE;
    audio_config_.threshold_margin = AudioConfig::DEFAULT_THRESHOLD_MARGIN;
    audio_config_.debounce_ms = AudioConfig::DEFAULT_DEBOUNCE_MS;
    audio_config_.gain_level = AudioConfig::DEFAULT_GAIN_LEVEL;
    audio_config_.kick_only_mode = AudioConfig::DEFAULT_KICK_ONLY_MODE;
    
    // BPM defaults
    bpm_config_.min_bpm = BPMConfig::DEFAULT_MIN_BPM;
    bpm_config_.max_bpm = BPMConfig::DEFAULT_MAX_BPM;
    bpm_config_.stability_threshold = BPMConfig::DEFAULT_STABILITY_THRESHOLD;
    bpm_config_.tempo_correction_enabled = BPMConfig::DEFAULT_TEMPO_CORRECTION_ENABLED;
    
    // Output defaults
    output_config_.midi_enabled = OutputConfig::DEFAULT_MIDI_ENABLED;
    output_config_.midi_channel = OutputConfig::DEFAULT_MIDI_CHANNEL;
    output_config_.midi_note = OutputConfig::DEFAULT_MIDI_NOTE;
    output_config_.midi_velocity = OutputConfig::DEFAULT_MIDI_VELOCITY;
    output_config_.relay_enabled = OutputConfig::DEFAULT_RELAY_ENABLED;
    output_config_.relay_pulse_ms = OutputConfig::DEFAULT_RELAY_PULSE_MS;
    
    // Network defaults
    network_config_.wifi_enabled = NetworkConfig::DEFAULT_WIFI_ENABLED;
    network_config_.wifi_ssid = "";
    network_config_.wifi_password = "";
    network_config_.mqtt_enabled = NetworkConfig::DEFAULT_MQTT_ENABLED;
    network_config_.mqtt_broker = "";
    network_config_.mqtt_port = NetworkConfig::DEFAULT_MQTT_PORT;
    network_config_.mqtt_username = "";
    network_config_.mqtt_password = "";
    network_config_.websocket_enabled = NetworkConfig::DEFAULT_WEBSOCKET_ENABLED;
}

bool ConfigurationManager::validateAudioConfig(const AudioConfig& config) const {
    // Sample rate: 8000-16000
    if (config.sample_rate < 8000 || config.sample_rate > 16000) {
        return false;
    }
    
    // Threshold margin: 50-200
    if (config.threshold_margin < 50 || config.threshold_margin > 200) {
        return false;
    }
    
    // Debounce: 20-100ms
    if (config.debounce_ms < 20 || config.debounce_ms > 100) {
        return false;
    }
    
    // Gain level: 40, 50, or 60 dB only
    if (config.gain_level != 40 && config.gain_level != 50 && config.gain_level != 60) {
        return false;
    }
    
    return true;
}

bool ConfigurationManager::validateBPMConfig(const BPMConfig& config) const {
    // Min BPM: 30-100
    if (config.min_bpm < 30 || config.min_bpm > 100) {
        return false;
    }
    
    // Max BPM: 200-600
    if (config.max_bpm < 200 || config.max_bpm > 600) {
        return false;
    }
    
    // Min must be less than max
    if (config.min_bpm >= config.max_bpm) {
        return false;
    }
    
    // Stability threshold: 1-10%
    if (config.stability_threshold < 1 || config.stability_threshold > 10) {
        return false;
    }
    
    return true;
}

bool ConfigurationManager::validateOutputConfig(const OutputConfig& config) const {
    // MIDI channel: 1-16
    if (config.midi_channel < 1 || config.midi_channel > 16) {
        return false;
    }
    
    // MIDI note: 0-127
    if (config.midi_note > 127) {
        return false;
    }
    
    // MIDI velocity: 0-127
    if (config.midi_velocity > 127) {
        return false;
    }
    
    // Relay pulse: 10-500ms
    if (config.relay_pulse_ms < 10 || config.relay_pulse_ms > 500) {
        return false;
    }
    
    return true;
}

bool ConfigurationManager::validateNetworkConfig(const NetworkConfig& config) const {
    // SSID max 32 chars
    if (config.wifi_ssid.length() > 32) {
        return false;
    }
    
    // WiFi password max 64 chars
    if (config.wifi_password.length() > 64) {
        return false;
    }
    
    // MQTT broker max 64 chars
    if (config.mqtt_broker.length() > 64) {
        return false;
    }
    
    // MQTT port: 1-65535
    if (config.mqtt_port < 1) {
        return false;
    }
    
    // MQTT username max 32 chars
    if (config.mqtt_username.length() > 32) {
        return false;
    }
    
    // MQTT password max 64 chars
    if (config.mqtt_password.length() > 64) {
        return false;
    }
    
    return true;
}

void ConfigurationManager::notifyChange(ConfigChangeEvent::Section section) {
    if (change_callback_) {
        ConfigChangeEvent event;
        event.section = section;
        event.timestamp_us = 0;  // TODO: Get actual timestamp from timing provider
        change_callback_(event);
    }
}

// ============================================================================
// NVS Helpers (Platform-Specific)
// ============================================================================

#ifdef NATIVE_BUILD
// Native build: Stub NVS functions for unit testing

bool ConfigurationManager::nvsInit() {
    // Stub: Always succeed
    return true;
}

bool ConfigurationManager::nvsLoad() {
    // Stub: Return false to trigger defaults loading
    return false;
}

bool ConfigurationManager::nvsSave() {
    // Stub: Always succeed
    return true;
}

bool ConfigurationManager::nvsErase() {
    // Stub: Always succeed
    return true;
}

#else
// ESP32 build: Actual NVS implementation

bool ConfigurationManager::nvsInit() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated, erase and retry
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return (err == ESP_OK);
}

bool ConfigurationManager::nvsLoad() {
    // TODO: Implement NVS load
    // For now, return false to use defaults
    return false;
}

bool ConfigurationManager::nvsSave() {
    // TODO: Implement NVS save
    return true;
}

bool ConfigurationManager::nvsErase() {
    // TODO: Implement NVS erase
    return true;
}

#endif

} // namespace clap_metronome
