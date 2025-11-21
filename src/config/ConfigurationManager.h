/**
 * @file ConfigurationManager.h
 * @brief Configuration Manager - Persistent storage for system configuration
 * 
 * @component DES-C-006: Configuration Manager
 * @implements REQ-F-005 (Persistent configuration storage)
 * @implements REQ-F-006 (Factory reset capability)
 * @implements REQ-NF-003 (Security - credential encryption)
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Implementation)
 * @standard IEEE 1016-2009 (Software Design Documentation)
 * 
 * @description
 * Manages system configuration with NVS (Non-Volatile Storage) persistence.
 * Provides factory defaults, range validation, change notifications, and
 * secure credential storage.
 * 
 * Key Features:
 * - NVS persistence (survives power cycles)
 * - Factory default values
 * - Range validation (e.g., BPM 40-300)
 * - Change notification callbacks
 * - Credential encryption (WiFi/MQTT passwords)
 * - Configuration migration (version upgrades)
 * 
 * @see GitHub Issue #47
 * @date 2025-11-21
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace clap_metronome {

/**
 * @brief Audio configuration structure
 * 
 * @implements DES-D-004 (NVS Configuration Schema - Audio section)
 */
struct AudioConfig {
    uint16_t sample_rate;           ///< ADC sample rate (Hz), valid: 8000-16000, default: 8000
    uint16_t threshold_margin;      ///< Threshold crossing margin, valid: 50-200, default: 80
    uint16_t debounce_ms;           ///< Debounce period (ms), valid: 20-100, default: 50
    uint8_t gain_level;             ///< AGC gain level (dB), valid: 40/50/60, default: 50
    bool kick_only_mode;            ///< Kick-only filtering enabled, default: false
    
    // Factory defaults
    static constexpr uint16_t DEFAULT_SAMPLE_RATE = 8000;
    static constexpr uint16_t DEFAULT_THRESHOLD_MARGIN = 80;
    static constexpr uint16_t DEFAULT_DEBOUNCE_MS = 50;
    static constexpr uint8_t DEFAULT_GAIN_LEVEL = 50;
    static constexpr bool DEFAULT_KICK_ONLY_MODE = false;
};

/**
 * @brief BPM calculation configuration structure
 * 
 * @implements DES-D-004 (NVS Configuration Schema - BPM section)
 */
struct BPMConfig {
    uint16_t min_bpm;               ///< Minimum valid BPM, valid: 30-100, default: 40
    uint16_t max_bpm;               ///< Maximum valid BPM, valid: 200-600, default: 300
    uint8_t stability_threshold;    ///< CV threshold for stability (%), valid: 1-10, default: 5
    bool tempo_correction_enabled;  ///< Half/double tempo correction, default: true
    
    // Factory defaults
    static constexpr uint16_t DEFAULT_MIN_BPM = 40;
    static constexpr uint16_t DEFAULT_MAX_BPM = 300;
    static constexpr uint8_t DEFAULT_STABILITY_THRESHOLD = 5;
    static constexpr bool DEFAULT_TEMPO_CORRECTION_ENABLED = true;
};

/**
 * @brief Output configuration structure
 * 
 * @implements DES-D-004 (NVS Configuration Schema - Output section)
 */
struct OutputConfig {
    bool midi_enabled;              ///< MIDI output enabled, default: true
    uint8_t midi_channel;           ///< MIDI channel (1-16), default: 10 (drums)
    uint8_t midi_note;              ///< MIDI note number (0-127), default: 60 (C4)
    uint8_t midi_velocity;          ///< MIDI velocity (0-127), default: 100
    
    bool relay_enabled;             ///< Relay output enabled, default: false
    uint16_t relay_pulse_ms;        ///< Relay pulse duration (ms), valid: 10-500, default: 50
    
    // Factory defaults
    static constexpr bool DEFAULT_MIDI_ENABLED = true;
    static constexpr uint8_t DEFAULT_MIDI_CHANNEL = 10;
    static constexpr uint8_t DEFAULT_MIDI_NOTE = 60;
    static constexpr uint8_t DEFAULT_MIDI_VELOCITY = 100;
    static constexpr bool DEFAULT_RELAY_ENABLED = false;
    static constexpr uint16_t DEFAULT_RELAY_PULSE_MS = 50;
};

/**
 * @brief Network configuration structure
 * 
 * @implements DES-D-004 (NVS Configuration Schema - Network section)
 */
struct NetworkConfig {
    bool wifi_enabled;              ///< WiFi enabled, default: true
    char wifi_ssid[33];             ///< WiFi SSID, max 32 chars + null terminator
    char wifi_password[65];         ///< WiFi password (encrypted in NVS), max 64 chars + null
    
    bool mqtt_enabled;              ///< MQTT enabled, default: false
    char mqtt_broker[65];           ///< MQTT broker hostname/IP, max 64 chars + null
    uint16_t mqtt_port;             ///< MQTT port, valid: 1-65535, default: 1883
    char mqtt_username[33];         ///< MQTT username, max 32 chars + null
    char mqtt_password[65];         ///< MQTT password (encrypted in NVS), max 64 chars + null
    
    bool websocket_enabled;         ///< WebSocket enabled, default: true
    
    // Factory defaults
    static constexpr bool DEFAULT_WIFI_ENABLED = true;
    static constexpr bool DEFAULT_MQTT_ENABLED = false;
    static constexpr uint16_t DEFAULT_MQTT_PORT = 1883;
    static constexpr bool DEFAULT_WEBSOCKET_ENABLED = true;
};

/**
 * @brief Configuration change event
 */
struct ConfigChangeEvent {
    enum class Section {
        AUDIO,
        BPM,
        OUTPUT,
        NETWORK,
        ALL  ///< Factory reset or full reload
    };
    
    Section section;                ///< Which section changed
    uint64_t timestamp_us;          ///< When the change occurred
};

/**
 * @brief Configuration Manager
 * 
 * @implements DES-I-008 (Configuration API)
 * 
 * Manages system configuration with NVS persistence, factory defaults,
 * validation, and change notifications.
 */
class ConfigurationManager {
public:
    /**
     * @brief Constructor
     */
    ConfigurationManager();
    
    /**
     * @brief Destructor
     */
    ~ConfigurationManager();
    
    // ========================================================================
    // Initialization
    // ========================================================================
    
    /**
     * @brief Initialize configuration manager
     * 
     * @return true if initialization successful
     * @return false if initialization failed
     * 
     * @implements AC-CFG-001 (Cold boot load <50ms)
     * 
     * Loads configuration from NVS. If NVS is empty or corrupt,
     * loads factory defaults.
     */
    bool init();
    
    // ========================================================================
    // Configuration Access (Getters)
    // ========================================================================
    
    /**
     * @brief Get audio configuration
     * 
     * @return AudioConfig Current audio configuration
     */
    AudioConfig getAudioConfig() const;
    
    /**
     * @brief Get BPM configuration
     * 
     * @return BPMConfig Current BPM configuration
     */
    BPMConfig getBPMConfig() const;
    
    /**
     * @brief Get output configuration
     * 
     * @return OutputConfig Current output configuration
     */
    OutputConfig getOutputConfig() const;
    
    /**
     * @brief Get network configuration
     * 
     * @return NetworkConfig Current network configuration
     */
    NetworkConfig getNetworkConfig() const;
    
    // ========================================================================
    // Configuration Modification (Setters)
    // ========================================================================
    
    /**
     * @brief Set audio configuration
     * 
     * @param config New audio configuration
     * @return true if configuration valid and set
     * @return false if configuration invalid (out of range)
     * 
     * @implements AC-CFG-004 (Range validation)
     * @implements AC-CFG-008 (Change notifications)
     * 
     * Validates ranges before setting. Fires change callback if successful.
     */
    bool setAudioConfig(const AudioConfig& config);
    
    /**
     * @brief Set BPM configuration
     * 
     * @param config New BPM configuration
     * @return true if configuration valid and set
     * @return false if configuration invalid
     * 
     * @implements AC-CFG-004 (Range validation)
     * @implements AC-CFG-008 (Change notifications)
     */
    bool setBPMConfig(const BPMConfig& config);
    
    /**
     * @brief Set output configuration
     * 
     * @param config New output configuration
     * @return true if configuration valid and set
     * @return false if configuration invalid
     * 
     * @implements AC-CFG-004 (Range validation)
     * @implements AC-CFG-008 (Change notifications)
     */
    bool setOutputConfig(const OutputConfig& config);
    
    /**
     * @brief Set network configuration
     * 
     * @param config New network configuration
     * @return true if configuration valid and set
     * @return false if configuration invalid
     * 
     * @implements AC-CFG-004 (Range validation)
     * @implements AC-CFG-008 (Change notifications)
     */
    bool setNetworkConfig(const NetworkConfig& config);
    
    // ========================================================================
    // Persistence
    // ========================================================================
    
    /**
     * @brief Save configuration to NVS
     * 
     * @return true if save successful
     * @return false if save failed
     * 
     * @implements AC-CFG-003 (Config save success)
     * 
     * Writes current configuration to NVS flash. Configuration persists
     * across power cycles.
     */
    bool saveConfig();
    
    /**
     * @brief Load configuration from NVS
     * 
     * @return true if load successful
     * @return false if load failed (loads defaults instead)
     * 
     * @implements AC-CFG-001 (Cold boot load)
     * 
     * Reads configuration from NVS. If NVS is empty or corrupt,
     * loads factory defaults.
     */
    bool loadConfig();
    
    // ========================================================================
    // Factory Reset
    // ========================================================================
    
    /**
     * @brief Restore factory default configuration
     * 
     * @implements AC-CFG-002 (Default values)
     * @implements AC-CFG-005 (Factory reset)
     * @implements AC-CFG-008 (Change notifications)
     * 
     * Resets all configuration to factory defaults and erases NVS.
     * Fires change callback with Section::ALL.
     */
    void factoryReset();
    
    // ========================================================================
    // Change Notifications
    // ========================================================================
    
    /**
     * Register callback for configuration changes
     * 
     * @param callback Function to call when configuration changes
     * 
     * @implements AC-CFG-008 (Change notifications)
     * 
     * Callback is fired after any successful configuration change.
     */
    void onConfigChange(std::function<void(const ConfigChangeEvent&)> callback);
    
#ifdef UNIT_TEST
    /**
     * Reset NVS storage (test-only helper)
     * Clears all stored configuration data for test isolation.
     */
    static void resetNVSForTesting();
#endif
    
private:
    // Configuration state
    AudioConfig audio_config_;
    BPMConfig bpm_config_;
    OutputConfig output_config_;
    NetworkConfig network_config_;
    
    // Change notification callback
    std::function<void(const ConfigChangeEvent&)> change_callback_;
    
    // Internal helpers
    void loadDefaults();
    bool validateAudioConfig(const AudioConfig& config) const;
    bool validateBPMConfig(const BPMConfig& config) const;
    bool validateOutputConfig(const OutputConfig& config) const;
    bool validateNetworkConfig(const NetworkConfig& config) const;
    void notifyChange(ConfigChangeEvent::Section section);
    
    // NVS helpers (platform-specific, stubbed for native build)
    bool nvsInit();
    bool nvsLoad();
    bool nvsSave();
    bool nvsErase();
};

} // namespace clap_metronome
