/**
 * Configuration Manager - Factory Default Values Tests
 * 
 * @file test_default_values.cpp
 * @component DES-C-006: Configuration Manager
 * @verifies AC-CFG-002: Factory defaults match specification
 * 
 * @test_type Unit Test
 * @phase Phase 05 - Implementation (TDD Cycle CFG-01)
 * @standards ISO/IEC/IEEE 12207:2017, XP Test-First
 * 
 * @description
 * Tests factory default configuration values.
 * 
 * Test Scenarios:
 * 1. AC-CFG-002: All factory defaults match specification
 * 2. Factory reset restores defaults
 * 3. Default values are within valid ranges
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/47
 * @date 2025-11-21
 */

#include <gtest/gtest.h>
#include "../../src/config/ConfigurationManager.h"

using namespace clap_metronome;

/**
 * Test Fixture for Configuration Manager Default Values
 */
class ConfigDefaultValuesTest : public ::testing::Test {
protected:
    ConfigurationManager* config_;
    
    void SetUp() override {
        config_ = new ConfigurationManager();
        config_->init();  // Loads defaults if no NVS data
    }
    
    void TearDown() override {
        delete config_;
    }
};

// ============================================================================
// AC-CFG-002: Audio Configuration Defaults
// ============================================================================

/**
 * @test AC-CFG-002: Audio configuration defaults match specification
 * 
 * @scenario Given ConfigurationManager just initialized
 *           When getAudioConfig() is called
 *           Then all audio values match factory defaults
 * 
 * @expected
 * - sample_rate = 8000 Hz
 * - threshold_margin = 80
 * - debounce_ms = 50
 * - gain_level = 50 dB
 * - kick_only_mode = false
 */
TEST_F(ConfigDefaultValuesTest, AudioConfig_HasCorrectDefaults) {
    // Act
    AudioConfig audio = config_->getAudioConfig();
    
    // Assert
    EXPECT_EQ(audio.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE)
        << "Sample rate should default to 8000 Hz";
    
    EXPECT_EQ(audio.threshold_margin, AudioConfig::DEFAULT_THRESHOLD_MARGIN)
        << "Threshold margin should default to 80";
    
    EXPECT_EQ(audio.debounce_ms, AudioConfig::DEFAULT_DEBOUNCE_MS)
        << "Debounce should default to 50ms";
    
    EXPECT_EQ(audio.gain_level, AudioConfig::DEFAULT_GAIN_LEVEL)
        << "Gain level should default to 50 dB";
    
    EXPECT_EQ(audio.kick_only_mode, AudioConfig::DEFAULT_KICK_ONLY_MODE)
        << "Kick-only mode should default to false";
}

// ============================================================================
// AC-CFG-002: BPM Configuration Defaults
// ============================================================================

/**
 * @test AC-CFG-002: BPM configuration defaults match specification
 * 
 * @scenario Given ConfigurationManager just initialized
 *           When getBPMConfig() is called
 *           Then all BPM values match factory defaults
 * 
 * @expected
 * - min_bpm = 40
 * - max_bpm = 300
 * - stability_threshold = 5%
 * - tempo_correction_enabled = true
 */
TEST_F(ConfigDefaultValuesTest, BPMConfig_HasCorrectDefaults) {
    // Act
    BPMConfig bpm = config_->getBPMConfig();
    
    // Assert
    EXPECT_EQ(bpm.min_bpm, BPMConfig::DEFAULT_MIN_BPM)
        << "Min BPM should default to 40";
    
    EXPECT_EQ(bpm.max_bpm, BPMConfig::DEFAULT_MAX_BPM)
        << "Max BPM should default to 300";
    
    EXPECT_EQ(bpm.stability_threshold, BPMConfig::DEFAULT_STABILITY_THRESHOLD)
        << "Stability threshold should default to 5%";
    
    EXPECT_EQ(bpm.tempo_correction_enabled, BPMConfig::DEFAULT_TEMPO_CORRECTION_ENABLED)
        << "Tempo correction should default to true";
}

// ============================================================================
// AC-CFG-002: Output Configuration Defaults
// ============================================================================

/**
 * @test AC-CFG-002: Output configuration defaults match specification
 * 
 * @scenario Given ConfigurationManager just initialized
 *           When getOutputConfig() is called
 *           Then all output values match factory defaults
 * 
 * @expected
 * - midi_enabled = true
 * - midi_channel = 10 (drum channel)
 * - midi_note = 60 (C4)
 * - midi_velocity = 100
 * - relay_enabled = false
 * - relay_pulse_ms = 50
 */
TEST_F(ConfigDefaultValuesTest, OutputConfig_HasCorrectDefaults) {
    // Act
    OutputConfig output = config_->getOutputConfig();
    
    // Assert
    EXPECT_EQ(output.midi_enabled, OutputConfig::DEFAULT_MIDI_ENABLED)
        << "MIDI should be enabled by default";
    
    EXPECT_EQ(output.midi_channel, OutputConfig::DEFAULT_MIDI_CHANNEL)
        << "MIDI channel should default to 10 (drums)";
    
    EXPECT_EQ(output.midi_note, OutputConfig::DEFAULT_MIDI_NOTE)
        << "MIDI note should default to 60 (C4)";
    
    EXPECT_EQ(output.midi_velocity, OutputConfig::DEFAULT_MIDI_VELOCITY)
        << "MIDI velocity should default to 100";
    
    EXPECT_EQ(output.relay_enabled, OutputConfig::DEFAULT_RELAY_ENABLED)
        << "Relay should be disabled by default";
    
    EXPECT_EQ(output.relay_pulse_ms, OutputConfig::DEFAULT_RELAY_PULSE_MS)
        << "Relay pulse should default to 50ms";
}

// ============================================================================
// AC-CFG-002: Network Configuration Defaults
// ============================================================================

/**
 * @test AC-CFG-002: Network configuration defaults match specification
 * 
 * @scenario Given ConfigurationManager just initialized
 *           When getNetworkConfig() is called
 *           Then all network values match factory defaults
 * 
 * @expected
 * - wifi_enabled = true
 * - wifi_ssid = "" (empty)
 * - wifi_password = "" (empty)
 * - mqtt_enabled = false
 * - mqtt_broker = "" (empty)
 * - mqtt_port = 1883
 * - mqtt_username = "" (empty)
 * - mqtt_password = "" (empty)
 * - websocket_enabled = true
 */
TEST_F(ConfigDefaultValuesTest, NetworkConfig_HasCorrectDefaults) {
    // Act
    NetworkConfig network = config_->getNetworkConfig();
    
    // Assert
    EXPECT_EQ(network.wifi_enabled, NetworkConfig::DEFAULT_WIFI_ENABLED)
        << "WiFi should be enabled by default";
    
    EXPECT_TRUE(network.wifi_ssid.empty())
        << "WiFi SSID should be empty by default";
    
    EXPECT_TRUE(network.wifi_password.empty())
        << "WiFi password should be empty by default";
    
    EXPECT_EQ(network.mqtt_enabled, NetworkConfig::DEFAULT_MQTT_ENABLED)
        << "MQTT should be disabled by default";
    
    EXPECT_TRUE(network.mqtt_broker.empty())
        << "MQTT broker should be empty by default";
    
    EXPECT_EQ(network.mqtt_port, NetworkConfig::DEFAULT_MQTT_PORT)
        << "MQTT port should default to 1883";
    
    EXPECT_TRUE(network.mqtt_username.empty())
        << "MQTT username should be empty by default";
    
    EXPECT_TRUE(network.mqtt_password.empty())
        << "MQTT password should be empty by default";
    
    EXPECT_EQ(network.websocket_enabled, NetworkConfig::DEFAULT_WEBSOCKET_ENABLED)
        << "WebSocket should be enabled by default";
}

// ============================================================================
// AC-CFG-005: Factory Reset Restores Defaults
// ============================================================================

/**
 * @test AC-CFG-005: Factory reset restores all defaults
 * 
 * @scenario Given configuration has been modified
 *           When factoryReset() is called
 *           Then all values restored to factory defaults
 */
TEST_F(ConfigDefaultValuesTest, FactoryReset_RestoresAllDefaults) {
    // Arrange: Modify configuration
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 16000;  // Change from default 8000
    audio.gain_level = 40;      // Change from default 50
    config_->setAudioConfig(audio);
    
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 60;           // Change from default 40
    config_->setBPMConfig(bpm);
    
    // Act: Factory reset
    config_->factoryReset();
    
    // Assert: All defaults restored
    AudioConfig audio_after = config_->getAudioConfig();
    EXPECT_EQ(audio_after.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE)
        << "Sample rate should be restored to default";
    EXPECT_EQ(audio_after.gain_level, AudioConfig::DEFAULT_GAIN_LEVEL)
        << "Gain level should be restored to default";
    
    BPMConfig bpm_after = config_->getBPMConfig();
    EXPECT_EQ(bpm_after.min_bpm, BPMConfig::DEFAULT_MIN_BPM)
        << "Min BPM should be restored to default";
}

// ============================================================================
// Edge Cases: Multiple Resets
// ============================================================================

/**
 * @test Edge case: Multiple factory resets are safe
 * 
 * @scenario Given configuration already at defaults
 *           When factoryReset() called multiple times
 *           Then defaults remain unchanged (idempotent)
 */
TEST_F(ConfigDefaultValuesTest, FactoryReset_Idempotent_SafeMultipleCalls) {
    // Act: Multiple resets
    config_->factoryReset();
    config_->factoryReset();
    config_->factoryReset();
    
    // Assert: Still at defaults
    AudioConfig audio = config_->getAudioConfig();
    EXPECT_EQ(audio.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);
    EXPECT_EQ(audio.gain_level, AudioConfig::DEFAULT_GAIN_LEVEL);
    
    BPMConfig bpm = config_->getBPMConfig();
    EXPECT_EQ(bpm.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
    EXPECT_EQ(bpm.max_bpm, BPMConfig::DEFAULT_MAX_BPM);
}
