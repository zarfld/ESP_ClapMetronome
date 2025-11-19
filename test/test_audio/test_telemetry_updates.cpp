/**
 * TDD Cycle 7 - AC-AUDIO-007: Telemetry Updates (RED Phase)
 * 
 * Validates that audio telemetry is published every 500ms with all required data.
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Tests: AC-AUDIO-007 (Telemetry updates - 500ms interval)
 * Interface: DES-I-005 (IAudioTelemetry)
 * Requirement: REQ-NF-004 (Telemetry and monitoring)
 * Standards: ISO/IEC/IEEE 12207:2017 (TDD), IEEE 1012-2016 (V&V)
 * 
 * Test Strategy:
 * 1. Verify 500ms interval constant
 * 2. Verify callback registration works
 * 3. Verify telemetry published at correct intervals
 * 4. Verify all telemetry fields populated correctly
 * 5. Verify no telemetry when no callback registered
 * 6. Verify multiple updates over time
 * 
 * Coverage: Integration test for AC-AUDIO-007
 * Dependencies: MockTimingProvider, AudioTelemetry struct
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../../src/audio/AudioDetectionState.h"
#include "../mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * Test Fixture: Telemetry Updates
 * 
 * Sets up AudioDetection with mock timing for telemetry validation.
 */
class TelemetryUpdateTest : public ::testing::Test {
protected:
    MockTimingProvider timing_;
    AudioDetection* detector_;
    
    // Telemetry capture
    bool telemetry_received_ = false;
    AudioTelemetry last_telemetry_;
    int telemetry_count_ = 0;
    
    void SetUp() override {
        detector_ = new AudioDetection(&timing_);
        detector_->init();
        timing_.setTimestamp(0);  // Start at T=0
    }
    
    void TearDown() override {
        delete detector_;
    }
    
    /**
     * Register telemetry callback that captures data
     */
    void registerTelemetryCallback() {
        detector_->onTelemetry([this](const AudioTelemetry& telem) {
            telemetry_received_ = true;
            last_telemetry_ = telem;
            telemetry_count_++;
        });
    }
    
    /**
     * Process samples to advance time
     * @param samples Number of samples to process
     * @param adc_value ADC value for all samples (default: baseline)
     */
    void processSamples(int samples, uint16_t adc_value = 2048) {
        for (int i = 0; i < samples; ++i) {
            timing_.advanceTime(1);  // Advance time FIRST so processSample sees new time
            detector_->processSample(adc_value);
        }
    }
};

// ===== Test 1: Telemetry Interval Constant =====

/**
 * Verify TELEMETRY_INTERVAL_US constant is 500ms (500000 microseconds)
 * 
 * Expected: Constant defined and equals 500000us
 */
TEST_F(TelemetryUpdateTest, TelemetryIntervalIs500ms) {
    constexpr uint64_t EXPECTED_INTERVAL_US = 500000;  // 500ms
    
    EXPECT_EQ(AudioDetectionState::TELEMETRY_INTERVAL_US, EXPECTED_INTERVAL_US)
        << "Telemetry interval should be 500ms (500000 microseconds)";
}

// ===== Test 2: Callback Registration =====

/**
 * Verify telemetry callback can be registered
 * 
 * Expected: hasTelemetryCallback() returns true after registration
 */
TEST_F(TelemetryUpdateTest, TelemetryCallbackRegistered) {
    EXPECT_FALSE(detector_->hasTelemetryCallback())
        << "No callback should be registered initially";
    
    registerTelemetryCallback();
    
    EXPECT_TRUE(detector_->hasTelemetryCallback())
        << "Callback should be registered after onTelemetry() call";
}

// ===== Test 3: No Telemetry Without Callback =====

/**
 * Verify no telemetry published when no callback registered
 * 
 * Expected: No crash, no telemetry callback invoked
 */
TEST_F(TelemetryUpdateTest, NoTelemetryWhenNoCallbackRegistered) {
    // Do NOT register callback
    EXPECT_FALSE(detector_->hasTelemetryCallback());
    
    // Process samples past 500ms interval
    processSamples(600000);  // 600ms worth of samples
    
    // Verify no telemetry received (no crash)
    EXPECT_FALSE(telemetry_received_)
        << "No telemetry should be published without callback";
    EXPECT_EQ(telemetry_count_, 0)
        << "Telemetry count should remain zero";
}

// ===== Test 4: First Telemetry at 500ms =====

/**
 * Verify first telemetry published at 500ms mark
 * 
 * Expected: No telemetry before 500ms, telemetry at/after 500ms
 */
TEST_F(TelemetryUpdateTest, FirstTelemetryPublishedAt500ms) {
    registerTelemetryCallback();
    
    // Process samples up to 499ms (just before 500ms)
    processSamples(499000);
    EXPECT_FALSE(telemetry_received_)
        << "No telemetry should be published before 500ms";
    
    // Process one more sample to reach 500ms
    processSamples(1000);  // Now at 500ms
    
    EXPECT_TRUE(telemetry_received_)
        << "Telemetry should be published at 500ms mark";
    EXPECT_EQ(telemetry_count_, 1)
        << "Exactly one telemetry update expected at 500ms";
}

// ===== Test 5: Telemetry Timestamp Correct =====

/**
 * Verify telemetry timestamp matches current sample time
 * 
 * Expected: AudioTelemetry.timestamp_us equals current timing provider timestamp
 */
TEST_F(TelemetryUpdateTest, TelemetryTimestampCorrect) {
    registerTelemetryCallback();
    
    // Process to 500ms
    processSamples(500000);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // Timestamp should be at or near 500ms mark
    EXPECT_GE(last_telemetry_.timestamp_us, 500000)
        << "Telemetry timestamp should be >= 500ms";
    EXPECT_LE(last_telemetry_.timestamp_us, 505000)
        << "Telemetry timestamp should be within a few ms of 500ms";
}

// ===== Test 6: Telemetry Contains Current ADC =====

/**
 * Verify telemetry includes current ADC value
 * 
 * Expected: AudioTelemetry.adc_value matches last processed sample
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsCurrentADC) {
    registerTelemetryCallback();
    
    constexpr uint16_t TEST_ADC_VALUE = 3000;
    
    // Process samples with specific ADC value
    processSamples(500000, TEST_ADC_VALUE);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    EXPECT_EQ(last_telemetry_.adc_value, TEST_ADC_VALUE)
        << "Telemetry should contain current ADC value";
}

// ===== Test 7: Telemetry Contains Threshold =====

/**
 * Verify telemetry includes current adaptive threshold
 * 
 * Expected: AudioTelemetry.threshold matches detector's current threshold
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsThreshold) {
    registerTelemetryCallback();
    
    // Process samples to let threshold adapt
    processSamples(500000, 2500);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // Threshold should be populated (non-zero after adaptation)
    EXPECT_GT(last_telemetry_.threshold, 0)
        << "Telemetry threshold should be set";
    
    // Should match detector's current threshold
    EXPECT_EQ(last_telemetry_.threshold, detector_->getThreshold())
        << "Telemetry threshold should match detector state";
}

// ===== Test 8: Telemetry Contains Gain Level =====

/**
 * Verify telemetry includes current AGC gain level
 * 
 * Expected: AudioTelemetry.gain_level matches detector's gain level
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsGainLevel) {
    registerTelemetryCallback();
    
    // Process samples (default gain is GAIN_60DB = 2)
    processSamples(500000, 2048);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // Gain level should match detector state
    uint8_t expected_gain = static_cast<uint8_t>(detector_->getGainLevel());
    EXPECT_EQ(last_telemetry_.gain_level, expected_gain)
        << "Telemetry gain level should match detector state";
}

// ===== Test 9: Telemetry Contains Beat Count =====

/**
 * Verify telemetry includes cumulative beat count
 * 
 * Expected: AudioTelemetry.beat_count matches detector's beat count
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsBeatCount) {
    registerTelemetryCallback();
    
    // Register beat callback to verify beats detected
    bool beat_received = false;
    detector_->onBeat([&beat_received](const BeatEvent&) {
        beat_received = true;
    });
    
    // Simulate beat: baseline → high samples → baseline
    processSamples(10000, 2048);  // Establish baseline
    processSamples(5000, 3700);   // High amplitude beat
    processSamples(10000, 2048);  // Return to baseline
    
    // Continue to 500ms to trigger telemetry
    processSamples(475000, 2048);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // Beat count should match detector state
    EXPECT_EQ(last_telemetry_.beat_count, detector_->getBeatCount())
        << "Telemetry beat count should match detector state";
    
    // If beat was detected, count should be > 0
    if (beat_received) {
        EXPECT_GT(last_telemetry_.beat_count, 0)
            << "Beat count should be > 0 if beat detected";
    }
}

// ===== Test 10: Telemetry Contains State =====

/**
 * Verify telemetry includes current detection state
 * 
 * Expected: AudioTelemetry.state matches detector's current state
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsState) {
    registerTelemetryCallback();
    
    // Process samples (detector should be in IDLE state at baseline)
    processSamples(500000, 2048);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // State should match detector state
    uint8_t expected_state = static_cast<uint8_t>(detector_->getState());
    EXPECT_EQ(last_telemetry_.state, expected_state)
        << "Telemetry state should match detector state";
}

// ===== Test 11: Multiple Telemetry Updates =====

/**
 * Verify telemetry published periodically every 500ms
 * 
 * Expected: Telemetry count increases at 500ms, 1000ms, 1500ms, 2000ms
 */
TEST_F(TelemetryUpdateTest, MultipleTelemetryUpdatesOverTime) {
    registerTelemetryCallback();
    
    // Process to 500ms - first telemetry
    processSamples(500000);
    EXPECT_EQ(telemetry_count_, 1)
        << "First telemetry at 500ms";
    
    // Process to 1000ms - second telemetry
    processSamples(500000);
    EXPECT_EQ(telemetry_count_, 2)
        << "Second telemetry at 1000ms";
    
    // Process to 1500ms - third telemetry
    processSamples(500000);
    EXPECT_EQ(telemetry_count_, 3)
        << "Third telemetry at 1500ms";
    
    // Process to 2000ms - fourth telemetry
    processSamples(500000);
    EXPECT_EQ(telemetry_count_, 4)
        << "Fourth telemetry at 2000ms";
}

// ===== Test 12: Telemetry Callback Replaceable =====

/**
 * Verify telemetry callback can be replaced
 * 
 * Expected: New callback receives telemetry, old callback no longer called
 */
TEST_F(TelemetryUpdateTest, TelemetryCallbackReplaceable) {
    // Register first callback
    int first_callback_count = 0;
    detector_->onTelemetry([&first_callback_count](const AudioTelemetry&) {
        first_callback_count++;
    });
    
    // Process to 500ms - first callback fires
    processSamples(500000);
    EXPECT_EQ(first_callback_count, 1)
        << "First callback should fire once";
    
    // Replace with second callback
    int second_callback_count = 0;
    detector_->onTelemetry([&second_callback_count](const AudioTelemetry&) {
        second_callback_count++;
    });
    
    // Process to 1000ms - only second callback fires
    processSamples(500000);
    EXPECT_EQ(first_callback_count, 1)
        << "First callback should not fire again (replaced)";
    EXPECT_EQ(second_callback_count, 1)
        << "Second callback should fire once";
}

// ===== Test 13: Telemetry Window Min/Max =====

/**
 * Verify telemetry includes min/max ADC values from current window
 * 
 * Expected: AudioTelemetry.min_value and max_value capture window range
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsWindowMinMax) {
    registerTelemetryCallback();
    
    // Process samples mostly at baseline, then vary values near telemetry time
    processSamples(499900, 2048);  // Get close to 500ms
    
    // Add varied samples that will be in the window when telemetry fires
    processSamples(50, 1500);  // Low values
    processSamples(50, 3500);  // High values
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // Min should capture low samples, max should capture high samples
    EXPECT_LE(last_telemetry_.min_value, 1500)
        << "Min value should capture low samples in window";
    EXPECT_GE(last_telemetry_.max_value, 3500)
        << "Max value should capture high samples in window";
}

// ===== Test 14: Telemetry False Positive Count =====

/**
 * Verify telemetry includes false positive rejection count
 * 
 * Expected: AudioTelemetry.false_positive_count matches detector state
 */
TEST_F(TelemetryUpdateTest, TelemetryContainsFalsePositiveCount) {
    registerTelemetryCallback();
    
    // Process samples (false positives detected during normal operation)
    processSamples(500000, 2048);
    
    ASSERT_TRUE(telemetry_received_)
        << "Telemetry should be published";
    
    // False positive count should match detector state
    EXPECT_EQ(last_telemetry_.false_positive_count, detector_->getFalsePositiveCount())
        << "Telemetry false positive count should match detector state";
}

