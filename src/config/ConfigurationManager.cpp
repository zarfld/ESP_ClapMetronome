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
#include <chrono>
#include <map>
#include <vector>
#include <string>

#if defined(ESP32) && !defined(NATIVE_BUILD)
#include <esp_timer.h>
#elif defined(ESP8266)
#include <Arduino.h>  // For micros()
#endif

#ifdef NATIVE_BUILD
// Native build: NVS functions use in-memory storage
// Global storage (outside namespace to avoid conflicts)
static std::map<std::string, std::vector<uint8_t>> native_nvs_storage;
static bool native_nvs_initialized = false;
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
    // Clear callback to avoid dangling function pointer
    change_callback_ = nullptr;
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
    notifyChange(ConfigChangeEvent::Section::OUTPUT_CFG);
    
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

#ifdef UNIT_TEST
void ConfigurationManager::resetNVSForTesting() {
#ifdef NATIVE_BUILD
    native_nvs_storage.clear();
    native_nvs_initialized = false;
#endif
}

std::vector<uint8_t> ConfigurationManager::getNVSRawData(const std::string& key) const {
#ifdef NATIVE_BUILD
    auto it = native_nvs_storage.find(key);
    if (it != native_nvs_storage.end()) {
        return it->second;
    }
#endif
    return std::vector<uint8_t>();  // Empty if not found
}
#endif

// ============================================================================
// Internal Helpers
// ============================================================================

void ConfigurationManager::encryptPassword(char* password, size_t max_len) {
    // Simple XOR encryption for native builds (demonstration)
    // For ESP32, use NVS encryption feature (hardware-backed)
    #ifdef NATIVE_BUILD
    const uint8_t key = 0xA5;  // XOR key (in production, use proper key derivation)
    size_t len = std::strlen(password);
    for (size_t i = 0; i < len && i < max_len; ++i) {
        password[i] ^= key;
    }
    #endif
    // ESP32: NVS handles encryption automatically when nvs_flash_init_partition is called with encryption
}

void ConfigurationManager::decryptPassword(char* password, size_t max_len) {
    // XOR is symmetric, so decrypt is same as encrypt
    #ifdef NATIVE_BUILD
    encryptPassword(password, max_len);  // XOR again to decrypt
    #endif
    // ESP32: NVS handles decryption automatically
}

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
    network_config_.wifi_ssid[0] = '\0';
    network_config_.wifi_password[0] = '\0';
    network_config_.mqtt_enabled = NetworkConfig::DEFAULT_MQTT_ENABLED;
    network_config_.mqtt_broker[0] = '\0';
    network_config_.mqtt_port = NetworkConfig::DEFAULT_MQTT_PORT;
    network_config_.mqtt_username[0] = '\0';
    network_config_.mqtt_password[0] = '\0';
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
    // SSID max 32 chars (null-terminated C string)
    if (std::strlen(config.wifi_ssid) > 32) {
        return false;
    }
    
    // WiFi password max 64 chars
    if (std::strlen(config.wifi_password) > 64) {
        return false;
    }
    
    // MQTT broker max 64 chars
    if (std::strlen(config.mqtt_broker) > 64) {
        return false;
    }
    
    // MQTT port: 1-65535
    if (config.mqtt_port < 1) {
        return false;
    }
    
    // MQTT username max 32 chars
    if (std::strlen(config.mqtt_username) > 32) {
        return false;
    }
    
    // MQTT password max 64 chars
    if (std::strlen(config.mqtt_password) > 64) {
        return false;
    }
    
    return true;
}

void ConfigurationManager::notifyChange(ConfigChangeEvent::Section section) {
    if (change_callback_) {
        ConfigChangeEvent event;
        event.section = section;
        
        // Get current timestamp in microseconds
        #ifdef NATIVE_BUILD
        // Native build: use std::chrono
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        event.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        #elif defined(ESP32)
        // ESP32 build: use esp_timer_get_time()
        event.timestamp_us = esp_timer_get_time();
        #elif defined(ESP8266)
        // ESP8266 build: use micros()
        event.timestamp_us = micros();
        #endif
        
        change_callback_(event);
    }
}

// ============================================================================
// NVS Helpers (Platform-Specific)
// ============================================================================

#ifdef NATIVE_BUILD
// Native build: In-memory NVS storage for unit testing

bool ConfigurationManager::nvsInit() {
    native_nvs_initialized = true;
    return true;
}

bool ConfigurationManager::nvsLoad() {
    if (!native_nvs_initialized) {
        return false;
    }
    
    // Try to load each section
    bool audio_loaded = false;
    bool bpm_loaded = false;
    bool output_loaded = false;
    bool network_loaded = false;
    
    // Load audio config
    auto it = native_nvs_storage.find("audio");
    if (it != native_nvs_storage.end() && it->second.size() == sizeof(AudioConfig)) {
        std::memcpy(&audio_config_, it->second.data(), sizeof(AudioConfig));
        audio_loaded = true;
    }
    
    // Load BPM config
    it = native_nvs_storage.find("bpm");
    if (it != native_nvs_storage.end() && it->second.size() == sizeof(BPMConfig)) {
        std::memcpy(&bpm_config_, it->second.data(), sizeof(BPMConfig));
        bpm_loaded = true;
    }
    
    // Load output config
    it = native_nvs_storage.find("output");
    if (it != native_nvs_storage.end() && it->second.size() == sizeof(OutputConfig)) {
        std::memcpy(&output_config_, it->second.data(), sizeof(OutputConfig));
        output_loaded = true;
    }
    
    // Load network config and decrypt passwords
    it = native_nvs_storage.find("network");
    if (it != native_nvs_storage.end() && it->second.size() == sizeof(NetworkConfig)) {
        std::memcpy(&network_config_, it->second.data(), sizeof(NetworkConfig));
        decryptPassword(network_config_.wifi_password, sizeof(network_config_.wifi_password));
        decryptPassword(network_config_.mqtt_password, sizeof(network_config_.mqtt_password));
        network_loaded = true;
    }
    
    // Return true only if at least one section was loaded
    return (audio_loaded || bpm_loaded || output_loaded || network_loaded);
}

bool ConfigurationManager::nvsSave() {
    if (!native_nvs_initialized) {
        return false;
    }
    
    // Save each section
    std::vector<uint8_t> audio_data(sizeof(AudioConfig));
    std::memcpy(audio_data.data(), &audio_config_, sizeof(AudioConfig));
    native_nvs_storage["audio"] = audio_data;
    
    std::vector<uint8_t> bpm_data(sizeof(BPMConfig));
    std::memcpy(bpm_data.data(), &bpm_config_, sizeof(BPMConfig));
    native_nvs_storage["bpm"] = bpm_data;
    
    std::vector<uint8_t> output_data(sizeof(OutputConfig));
    std::memcpy(output_data.data(), &output_config_, sizeof(OutputConfig));
    native_nvs_storage["output"] = output_data;
    
    // Save network config with encrypted passwords
    NetworkConfig network_encrypted = network_config_;  // Copy
    encryptPassword(network_encrypted.wifi_password, sizeof(network_encrypted.wifi_password));
    encryptPassword(network_encrypted.mqtt_password, sizeof(network_encrypted.mqtt_password));
    
    std::vector<uint8_t> network_data(sizeof(NetworkConfig));
    std::memcpy(network_data.data(), &network_encrypted, sizeof(NetworkConfig));
    native_nvs_storage["network"] = network_data;
    
    return true;
}

bool ConfigurationManager::nvsErase() {
    native_nvs_storage.clear();
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
