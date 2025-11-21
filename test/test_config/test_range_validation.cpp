/**
 * Configuration Manager Range Validation Tests
 * 
 * Tests: AC-CFG-004 (Range validation)
 * Requirements: REQ-F-005 (Persistent configuration)
 * Component: DES-C-006 (Configuration Manager)
 * 
 * Verifies that:
 * - Invalid audio settings are rejected
 * - Invalid BPM settings are rejected
 * - Invalid output settings are rejected
 * - Invalid network settings are rejected
 * - Config unchanged after rejected update
 * 
 * GitHub Issue: #47
 */

#include <gtest/gtest.h>
#include "../../src/config/ConfigurationManager.h"

using namespace clap_metronome;

class ConfigRangeValidationTest : public ::testing::Test {
protected:
    ConfigurationManager* config_;
    
    void SetUp() override {
        config_ = new ConfigurationManager();
        config_->init();
    }
    
    void TearDown() override {
        delete config_;
    }
};

// ============================================================================
// AUDIO CONFIG VALIDATION TESTS
// ============================================================================

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsSampleRateTooLow) {
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 7999;  // Below minimum (8000)
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    // Config should be unchanged
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsSampleRateTooHigh) {
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 16001;  // Above maximum (16000)
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_AcceptsValidSampleRateRange) {
    AudioConfig audio = config_->getAudioConfig();
    
    // Test minimum
    audio.sample_rate = 8000;
    EXPECT_TRUE(config_->setAudioConfig(audio));
    EXPECT_EQ(config_->getAudioConfig().sample_rate, 8000);
    
    // Test maximum
    audio.sample_rate = 16000;
    EXPECT_TRUE(config_->setAudioConfig(audio));
    EXPECT_EQ(config_->getAudioConfig().sample_rate, 16000);
    
    // Test mid-range
    audio.sample_rate = 12000;
    EXPECT_TRUE(config_->setAudioConfig(audio));
    EXPECT_EQ(config_->getAudioConfig().sample_rate, 12000);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsThresholdMarginTooLow) {
    AudioConfig audio = config_->getAudioConfig();
    audio.threshold_margin = 49;  // Below minimum (50)
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.threshold_margin, AudioConfig::DEFAULT_THRESHOLD_MARGIN);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsThresholdMarginTooHigh) {
    AudioConfig audio = config_->getAudioConfig();
    audio.threshold_margin = 201;  // Above maximum (200)
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.threshold_margin, AudioConfig::DEFAULT_THRESHOLD_MARGIN);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsDebounceTooLow) {
    AudioConfig audio = config_->getAudioConfig();
    audio.debounce_ms = 19;  // Below minimum (20)
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.debounce_ms, AudioConfig::DEFAULT_DEBOUNCE_MS);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsDebounceTooHigh) {
    AudioConfig audio = config_->getAudioConfig();
    audio.debounce_ms = 101;  // Above maximum (100)
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.debounce_ms, AudioConfig::DEFAULT_DEBOUNCE_MS);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_RejectsInvalidGainLevel) {
    AudioConfig audio = config_->getAudioConfig();
    
    // Test invalid values (only 40, 50, 60 are valid)
    audio.gain_level = 45;
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    audio.gain_level = 55;
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    audio.gain_level = 70;
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    // Config should be unchanged
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.gain_level, AudioConfig::DEFAULT_GAIN_LEVEL);
}

TEST_F(ConfigRangeValidationTest, AudioConfig_AcceptsValidGainLevels) {
    AudioConfig audio = config_->getAudioConfig();
    
    audio.gain_level = 40;
    EXPECT_TRUE(config_->setAudioConfig(audio));
    EXPECT_EQ(config_->getAudioConfig().gain_level, 40);
    
    audio.gain_level = 50;
    EXPECT_TRUE(config_->setAudioConfig(audio));
    EXPECT_EQ(config_->getAudioConfig().gain_level, 50);
    
    audio.gain_level = 60;
    EXPECT_TRUE(config_->setAudioConfig(audio));
    EXPECT_EQ(config_->getAudioConfig().gain_level, 60);
}

// ============================================================================
// BPM CONFIG VALIDATION TESTS
// ============================================================================

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsMinBPMTooLow) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 29;  // Below minimum (30)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsMinBPMTooHigh) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 101;  // Above maximum (100)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsMaxBPMTooLow) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.max_bpm = 199;  // Below minimum (200)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.max_bpm, BPMConfig::DEFAULT_MAX_BPM);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsMaxBPMTooHigh) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.max_bpm = 601;  // Above maximum (600)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.max_bpm, BPMConfig::DEFAULT_MAX_BPM);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsMinGreaterThanMax) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 100;
    bpm.max_bpm = 80;  // Min > Max (invalid)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
    EXPECT_EQ(current.max_bpm, BPMConfig::DEFAULT_MAX_BPM);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsMinEqualToMax) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.min_bpm = 100;   // Max valid min
    bpm.max_bpm = 100;   // Below min valid max (200) - should be rejected
    
    // Min == Max is not valid (min must be < max)
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    // Config should remain unchanged
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.min_bpm, BPMConfig::DEFAULT_MIN_BPM);
    EXPECT_EQ(current.max_bpm, BPMConfig::DEFAULT_MAX_BPM);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsStabilityThresholdTooLow) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.stability_threshold = 0;  // Below minimum (1)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.stability_threshold, BPMConfig::DEFAULT_STABILITY_THRESHOLD);
}

TEST_F(ConfigRangeValidationTest, BPMConfig_RejectsStabilityThresholdTooHigh) {
    BPMConfig bpm = config_->getBPMConfig();
    bpm.stability_threshold = 11;  // Above maximum (10)
    
    EXPECT_FALSE(config_->setBPMConfig(bpm));
    
    BPMConfig current = config_->getBPMConfig();
    EXPECT_EQ(current.stability_threshold, BPMConfig::DEFAULT_STABILITY_THRESHOLD);
}

// ============================================================================
// OUTPUT CONFIG VALIDATION TESTS
// ============================================================================

TEST_F(ConfigRangeValidationTest, OutputConfig_RejectsMIDIChannelTooLow) {
    OutputConfig output = config_->getOutputConfig();
    output.midi_channel = 0;  // Below minimum (1)
    
    EXPECT_FALSE(config_->setOutputConfig(output));
    
    OutputConfig current = config_->getOutputConfig();
    EXPECT_EQ(current.midi_channel, OutputConfig::DEFAULT_MIDI_CHANNEL);
}

TEST_F(ConfigRangeValidationTest, OutputConfig_RejectsMIDIChannelTooHigh) {
    OutputConfig output = config_->getOutputConfig();
    output.midi_channel = 17;  // Above maximum (16)
    
    EXPECT_FALSE(config_->setOutputConfig(output));
    
    OutputConfig current = config_->getOutputConfig();
    EXPECT_EQ(current.midi_channel, OutputConfig::DEFAULT_MIDI_CHANNEL);
}

TEST_F(ConfigRangeValidationTest, OutputConfig_RejectsMIDINoteTooHigh) {
    OutputConfig output = config_->getOutputConfig();
    output.midi_note = 128;  // Above maximum (127)
    
    EXPECT_FALSE(config_->setOutputConfig(output));
    
    OutputConfig current = config_->getOutputConfig();
    EXPECT_EQ(current.midi_note, OutputConfig::DEFAULT_MIDI_NOTE);
}

TEST_F(ConfigRangeValidationTest, OutputConfig_RejectsMIDIVelocityTooHigh) {
    OutputConfig output = config_->getOutputConfig();
    output.midi_velocity = 128;  // Above maximum (127)
    
    EXPECT_FALSE(config_->setOutputConfig(output));
    
    OutputConfig current = config_->getOutputConfig();
    EXPECT_EQ(current.midi_velocity, OutputConfig::DEFAULT_MIDI_VELOCITY);
}

TEST_F(ConfigRangeValidationTest, OutputConfig_RejectsRelayPulseTooLow) {
    OutputConfig output = config_->getOutputConfig();
    output.relay_pulse_ms = 9;  // Below minimum (10)
    
    EXPECT_FALSE(config_->setOutputConfig(output));
    
    OutputConfig current = config_->getOutputConfig();
    EXPECT_EQ(current.relay_pulse_ms, OutputConfig::DEFAULT_RELAY_PULSE_MS);
}

TEST_F(ConfigRangeValidationTest, OutputConfig_RejectsRelayPulseTooHigh) {
    OutputConfig output = config_->getOutputConfig();
    output.relay_pulse_ms = 501;  // Above maximum (500)
    
    EXPECT_FALSE(config_->setOutputConfig(output));
    
    OutputConfig current = config_->getOutputConfig();
    EXPECT_EQ(current.relay_pulse_ms, OutputConfig::DEFAULT_RELAY_PULSE_MS);
}

// ============================================================================
// NETWORK CONFIG VALIDATION TESTS
// ============================================================================

TEST_F(ConfigRangeValidationTest, NetworkConfig_RejectsSSIDTooLong) {
    NetworkConfig network = config_->getNetworkConfig();
    network.wifi_ssid = std::string(33, 'X');  // 33 chars, max is 32
    
    EXPECT_FALSE(config_->setNetworkConfig(network));
    
    NetworkConfig current = config_->getNetworkConfig();
    EXPECT_EQ(current.wifi_ssid, "");  // Should remain default (empty)
}

TEST_F(ConfigRangeValidationTest, NetworkConfig_AcceptsSSIDAtMaxLength) {
    NetworkConfig network = config_->getNetworkConfig();
    network.wifi_ssid = std::string(32, 'X');  // 32 chars (max valid)
    
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_EQ(config_->getNetworkConfig().wifi_ssid.length(), 32);
}

TEST_F(ConfigRangeValidationTest, NetworkConfig_RejectsPasswordTooLong) {
    NetworkConfig network = config_->getNetworkConfig();
    network.wifi_password = std::string(65, 'Y');  // 65 chars, max is 64
    
    EXPECT_FALSE(config_->setNetworkConfig(network));
    
    NetworkConfig current = config_->getNetworkConfig();
    EXPECT_EQ(current.wifi_password, "");
}

TEST_F(ConfigRangeValidationTest, NetworkConfig_RejectsMQTTUsernameTooLong) {
    NetworkConfig network = config_->getNetworkConfig();
    network.mqtt_username = std::string(33, 'Z');  // 33 chars, max is 32
    
    EXPECT_FALSE(config_->setNetworkConfig(network));
    
    NetworkConfig current = config_->getNetworkConfig();
    EXPECT_EQ(current.mqtt_username, "");
}

// ============================================================================
// COMPOUND VALIDATION TESTS
// ============================================================================

TEST_F(ConfigRangeValidationTest, MultipleInvalidFieldsAllRejected) {
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 7999;  // Invalid
    audio.gain_level = 45;      // Invalid
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    // All fields should remain at defaults
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);
    EXPECT_EQ(current.gain_level, AudioConfig::DEFAULT_GAIN_LEVEL);
}

TEST_F(ConfigRangeValidationTest, OneInvalidFieldRejectsEntireConfig) {
    AudioConfig audio = config_->getAudioConfig();
    audio.sample_rate = 12000;      // Valid
    audio.threshold_margin = 100;   // Valid
    audio.debounce_ms = 50;         // Valid
    audio.gain_level = 45;          // Invalid - should reject entire update
    
    EXPECT_FALSE(config_->setAudioConfig(audio));
    
    // No fields should be updated
    AudioConfig current = config_->getAudioConfig();
    EXPECT_EQ(current.sample_rate, AudioConfig::DEFAULT_SAMPLE_RATE);
    EXPECT_EQ(current.threshold_margin, AudioConfig::DEFAULT_THRESHOLD_MARGIN);
}
