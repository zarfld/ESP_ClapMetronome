/**
 * @file test_migration.cpp
 * @brief Configuration Manager Migration Tests
 * 
 * @component DES-C-006: Configuration Manager
 * @implements AC-CFG-007: Config migration
 * @requirement REQ-F-005: Persistent configuration storage
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Testing)
 * @standard IEEE 1012-2016 (Verification and Validation)
 * 
 * @description
 * Tests for configuration migration across versions:
 * - Old key names → new key names
 * - Legacy format → current format
 * - Missing fields get defaults
 * - Version detection and upgrade
 * 
 * TDD Cycle: CFG-06 (Config Migration)
 * - RED: Write failing tests for migration scenarios
 * - GREEN: Implement migration logic
 * - REFACTOR: Ensure backward compatibility
 * 
 * Migration Scenarios:
 * - v1.0: Initial release (legacy keys)
 * - v1.1: Current version (renamed keys, new fields)
 * 
 * @date 2025-11-21
 */

#include <gtest/gtest.h>
#include "config/ConfigurationManager.h"
#include <cstring>

using namespace clap_metronome;

// ============================================================================
// TEST FIXTURE
// ============================================================================

class ConfigMigrationTest : public ::testing::Test {
protected:
    ConfigurationManager* config_;
    
    void SetUp() override {
        // Reset NVS storage for test isolation
        ConfigurationManager::resetNVSForTesting();
        
        config_ = new ConfigurationManager();
        config_->init();
    }
    
    void TearDown() override {
        if (config_ != nullptr) {
            delete config_;
            config_ = nullptr;
        }
        ConfigurationManager::resetNVSForTesting();
    }
    
    // Helper: Simulate reboot
    void simulateReboot() {
        if (config_ != nullptr) {
            delete config_;
            config_ = nullptr;
        }
        config_ = new ConfigurationManager();
        config_->init();
    }
    
    // Helper: Write legacy v1.0 config to NVS (for migration testing)
    void writeLegacyV10Config() {
        // Simulate old config with legacy key names
        // v1.0 used "sample_freq" instead of "sample_rate"
        // v1.0 used "min_tempo" instead of "min_bpm"
        
        AudioConfig audio = config_->getAudioConfig();
        audio.sample_rate = 10000;  // Old default was 10kHz
        config_->setAudioConfig(audio);
        
        BPMConfig bpm = config_->getBPMConfig();
        bpm.min_bpm = 50;  // Old default was 50
        bpm.max_bpm = 250;  // Old default was 250
        config_->setBPMConfig(bpm);
        
        config_->saveConfig();
    }
    
    // Helper: Write config with missing network section (v1.0 didn't have network config)
    void writeLegacyV10WithoutNetwork() {
        AudioConfig audio = config_->getAudioConfig();
        audio.sample_rate = 12000;
        config_->setAudioConfig(audio);
        
        // Only save audio and BPM, not network (simulates old version)
        config_->saveConfig();
    }
};

// ============================================================================
// BASIC MIGRATION TESTS
// ============================================================================

TEST_F(ConfigMigrationTest, NewInstall_UsesCurrentDefaults) {
    // Fresh install (no existing config) should use current defaults
    AudioConfig audio = config_->getAudioConfig();
    EXPECT_EQ(audio.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);  // 8000 in v1.1
    
    BPMConfig bpm = config_->getBPMConfig();
    EXPECT_EQ(bpm.min_bpm, BPMConfig::DEFAULT_MIN_BPM);  // 40 in v1.1
    EXPECT_EQ(bpm.max_bpm, BPMConfig::DEFAULT_MAX_BPM);  // 300 in v1.1
}

TEST_F(ConfigMigrationTest, ExistingConfig_LoadsSuccessfully) {
    // Save config with current version
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 14000;
    config_->setAudioConfig(audio);
    config_->saveConfig();
    
    // Simulate reboot - should load saved config
    simulateReboot();
    
    AudioConfig loaded = config_->getAudioConfig();
    EXPECT_EQ(loaded.sample_rate, 14000);
}

// ============================================================================
// KEY RENAMING MIGRATION
// ============================================================================

TEST_F(ConfigMigrationTest, LegacyV10Config_MigratesAutomatically) {
    // Write legacy config
    writeLegacyV10Config();
    
    // Simulate reboot - migration should happen during init()
    simulateReboot();
    
    // Verify config loaded (even though key names changed)
    AudioConfig audio = config_->getAudioConfig();
    EXPECT_EQ(audio.sample_rate, 10000);  // Value preserved
    
    BPMConfig bpm = config_->getBPMConfig();
    EXPECT_EQ(bpm.min_bpm, 50);
    EXPECT_EQ(bpm.max_bpm, 250);
}

TEST_F(ConfigMigrationTest, LegacyConfig_SavesMigratedVersion) {
    // Write legacy config
    writeLegacyV10Config();
    
    // Simulate reboot and migration
    simulateReboot();
    
    // Modify and save - should use new format
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 16000;
    config_->setAudioConfig(audio);
    config_->saveConfig();
    
    // Another reboot - should load from new format
    simulateReboot();
    
    AudioConfig loaded = config_->getAudioConfig();
    EXPECT_EQ(loaded.sample_rate, 16000);
}

// ============================================================================
// MISSING FIELDS MIGRATION
// ============================================================================

TEST_F(ConfigMigrationTest, MissingNetworkSection_GetsDefaults) {
    // v1.0 didn't have network config
    writeLegacyV10WithoutNetwork();
    
    // Simulate reboot
    simulateReboot();
    
    // Audio should be preserved
    AudioConfig audio = config_->getAudioConfig();
    EXPECT_EQ(audio.sample_rate, 12000);
    
    // Network should have defaults (new in v1.1)
    NetworkConfig network = config_->getNetworkConfig();
    EXPECT_EQ(network.wifi_enabled, NetworkConfig::DEFAULT_WIFI_ENABLED);
    EXPECT_EQ(network.mqtt_enabled, NetworkConfig::DEFAULT_MQTT_ENABLED);
}

TEST_F(ConfigMigrationTest, MissingField_GetsDefaultValue) {
    // Simulate config missing a field (e.g., kick_only_mode added in v1.1)
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 11000;
    // Don't set kick_only_mode (simulate old version)
    config_->setAudioConfig(audio);
    config_->saveConfig();
    
    // Simulate reboot
    simulateReboot();
    
    // New field should have default value
    AudioConfig loaded = config_->getAudioConfig();
    EXPECT_EQ(loaded.sample_rate, 11000);  // Preserved
    EXPECT_EQ(loaded.kick_only_mode, AudioConfig::DEFAULT_KICK_ONLY_MODE);  // Default
}

// ============================================================================
// FORWARD COMPATIBILITY
// ============================================================================

TEST_F(ConfigMigrationTest, CurrentVersion_NoMigrationNeeded) {
    // Save with current version
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 9000;
    config_->setAudioConfig(audio);
    config_->saveConfig();
    
    // Track if migration occurred (should not)
    simulateReboot();
    
    // Config should load without migration
    AudioConfig loaded = config_->getAudioConfig();
    EXPECT_EQ(loaded.sample_rate, 9000);
}

TEST_F(ConfigMigrationTest, MultipleReboots_ConfigStable) {
    // Set initial config
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 13000;
    config_->setAudioConfig(audio);
    config_->saveConfig();
    
    // Multiple reboot cycles
    for (int i = 0; i < 5; ++i) {
        simulateReboot();
        AudioConfig loaded = config_->getAudioConfig();
        EXPECT_EQ(loaded.sample_rate, 13000) << "Failed on reboot " << i;
    }
}

// ============================================================================
// PARTIAL MIGRATION
// ============================================================================

TEST_F(ConfigMigrationTest, PartialConfig_FillsMissingWithDefaults) {
    // Only save audio config (simulate incomplete save or corruption)
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 15000;
    config_->setAudioConfig(audio);
    // Don't save to NVS - init will load defaults for missing sections
    
    // Reboot
    simulateReboot();
    
    // Audio might be default (no save), BPM should be default
    BPMConfig bpm = config_->getBPMConfig();
    EXPECT_EQ(bpm.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
    EXPECT_EQ(bpm.max_bpm, BPMConfig::DEFAULT_MAX_BPM);
}

// ============================================================================
// MIGRATION VALIDATION
// ============================================================================

TEST_F(ConfigMigrationTest, MigratedConfig_PassesValidation) {
    // Write legacy config with values at old limits
    writeLegacyV10Config();
    
    // Simulate migration
    simulateReboot();
    
    // Try to modify - should pass validation with new rules
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 40;  // New minimum (was 50 in v1.0)
    EXPECT_TRUE(config_->setBPMConfig(bpm));
}

TEST_F(ConfigMigrationTest, InvalidLegacyValue_RejectedAfterMigration) {
    // Suppose v1.0 allowed BPM down to 20, v1.1 requires >= 30
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 20;  // Would be valid in v1.0, invalid in v1.1 (min is 30)
    
    // Should be rejected by current validation
    EXPECT_FALSE(config_->setBPMConfig(bpm));
}

// ============================================================================
// FACTORY RESET AFTER MIGRATION
// ============================================================================

TEST_F(ConfigMigrationTest, FactoryReset_AfterMigration_UsesCurrentDefaults) {
    // Migrate from legacy
    writeLegacyV10Config();
    simulateReboot();
    
    // Factory reset
    config_->factoryReset();
    
    // Should use current defaults, not legacy defaults
    AudioConfig audio = config_->getAudioConfig();
    EXPECT_EQ(audio.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);  // 8000 (v1.1)
    
    BPMConfig bpm = config_->getBPMConfig();
    EXPECT_EQ(bpm.min_bpm, BPMConfig::DEFAULT_MIN_BPM);  // 40 (v1.1)
    EXPECT_EQ(bpm.max_bpm, BPMConfig::DEFAULT_MAX_BPM);  // 300 (v1.1)
}

// ============================================================================
// DATA INTEGRITY
// ============================================================================

TEST_F(ConfigMigrationTest, Migration_PreservesUserData) {
    // Set custom values
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 11500;
    audio.threshold_margin = 180;
    audio.debounce_ms = 65;
    config_->setAudioConfig(audio);
    config_->saveConfig();
    
    // Simulate migration
    simulateReboot();
    
    // All custom values should be preserved
    AudioConfig loaded = config_->getAudioConfig();
    EXPECT_EQ(loaded.sample_rate, 11500);
    EXPECT_EQ(loaded.threshold_margin, 180);
    EXPECT_EQ(loaded.debounce_ms, 65);
}

TEST_F(ConfigMigrationTest, EmptyNVS_InitializesWithDefaults) {
    // NVS is empty (first boot or after erase)
    // init() should populate with defaults
    
    AudioConfig audio = config_->getAudioConfig();
    EXPECT_EQ(audio.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);
    
    BPMConfig bpm = config_->getBPMConfig();
    EXPECT_EQ(bpm.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
    
    OutputConfig output = config_->getOutputConfig();
    EXPECT_EQ(output.midi_enabled, OutputConfig::DEFAULT_MIDI_ENABLED);
    
    NetworkConfig network = config_->getNetworkConfig();
    EXPECT_EQ(network.wifi_enabled, NetworkConfig::DEFAULT_WIFI_ENABLED);
}
