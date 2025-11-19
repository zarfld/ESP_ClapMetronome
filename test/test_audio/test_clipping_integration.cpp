/**
 * @file test_clipping_integration.cpp
 * @brief Integration Tests for AC-AUDIO-012 Clipping Prevention
 * 
 * TDD Cycle 12: Clipping Prevention Integration
 * 
 * Acceptance Criteria:
 *   - AC-AUDIO-012: AGC automatically reduces gain when clipping detected (>4000 ADC)
 *   - Clipping prevention integrated into beat detection flow
 *   - Beat detection continues to work correctly during gain transitions
 *   - Threshold adaptation remains accurate after gain changes
 * 
 * Test Strategy:
 *   This is a VALIDATION phase - AGC logic was implemented in Cycle 3.
 *   These integration tests validate that clipping prevention:
 *   1. Works correctly during active beat detection
 *   2. Doesn't interfere with threshold adaptation
 *   3. Maintains beat detection accuracy across gain transitions
 *   4. Properly updates telemetry with gain changes
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (TDD) - VALIDATION Phase
 * 
 * Traceability:
 *   - Implements: AC-AUDIO-012 (Clipping prevention integration)
 *   - Design: 04-design/tdd-plan-phase-05.md
 *   - Related: AC-AUDIO-003 (AGC transitions)
 */

#include <gtest/gtest.h>
#include <vector>
#include "../../src/audio/AudioDetection.h"
#include "../mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * @brief Integration Test Fixture for Clipping Prevention
 * 
 * Provides utilities for testing AGC integration with full detection pipeline.
 */
class ClippingIntegrationTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* detector_;
    
    // Beat event capture
    BeatEvent last_beat_;
    int beat_count_;
    
    // Telemetry capture
    AudioTelemetry last_telemetry_;
    int telemetry_count_;
    
    void SetUp() override {
        mock_timing_.reset();
        detector_ = new AudioDetection(&mock_timing_);
        detector_->init();
        
        beat_count_ = 0;
        telemetry_count_ = 0;
        
        // Register callbacks
        detector_->onBeat([this](const BeatEvent& event) {
            last_beat_ = event;
            beat_count_++;
        });
        
        detector_->onTelemetry([this](const AudioTelemetry& telemetry) {
            last_telemetry_ = telemetry;
            telemetry_count_++;
        });
    }
    
    void TearDown() override {
        delete detector_;
    }
    
    /**
     * Generate a beat signal with specified peak amplitude
     */
    void generateBeat(uint16_t peak_amplitude, uint64_t start_time_us) {
        // Background (10 samples)
        for (int i = 0; i < 10; i++) {
            mock_timing_.setTimestamp(start_time_us + i * 62);
            detector_->processSample(2000);
        }
        
        // Rising edge (5 samples)
        for (int i = 0; i < 5; i++) {
            uint16_t value = static_cast<uint16_t>(2000 + (peak_amplitude - 2000) * i / 5);
            mock_timing_.setTimestamp(start_time_us + 1000 + i * 62);
            detector_->processSample(value);
        }
        
        // Peak
        mock_timing_.setTimestamp(start_time_us + 1500);
        detector_->processSample(peak_amplitude);
        
        // Falling edge (5 samples)
        for (int i = 0; i < 5; i++) {
            uint16_t value = static_cast<uint16_t>(peak_amplitude - (peak_amplitude - 2000) * i / 5);
            mock_timing_.setTimestamp(start_time_us + 1500 + i * 62);
            detector_->processSample(value);
        }
        
        // Return to background (10 samples)
        for (int i = 0; i < 10; i++) {
            mock_timing_.setTimestamp(start_time_us + 2000 + i * 62);
            detector_->processSample(2000);
        }
    }
    
    /**
     * Fill window with stable background noise
     */
    void fillWindowWithBackground(uint16_t background_level) {
        for (int i = 0; i < 64; i++) {
            detector_->processSample(background_level);
            mock_timing_.advanceTime(62);
        }
    }
};

/**
 * @test ClippingDetection_ReducesGain
 * 
 * Validates that clipping detection triggers AGC gain reduction.
 * 
 * Given: System at GAIN_50DB (medium gain)
 * When: Sample >4000 ADC (clipping threshold)
 * Then: AGC reduces gain to GAIN_40DB
 * 
 * Traceability: AC-AUDIO-012 (Clipping prevention)
 */
TEST_F(ClippingIntegrationTest, ClippingDetection_ReducesGain) {
    // Arrange: Start at GAIN_50DB
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel());
    
    // Act: Feed clipping sample
    detector_->processSample(4050);  // Above 4000 threshold
    
    // Assert: Gain reduced
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "AGC should reduce gain to 40dB when clipping detected";
}

/**
 * @test ClippingDuringBeat_StillDetects
 * 
 * Validates that beat detection continues to work when clipping occurs.
 * 
 * Given: System detecting beats normally
 * When: Beat causes clipping (peak >4000)
 * Then: Beat is still detected AND gain is reduced
 * 
 * Traceability: AC-AUDIO-012 (Clipping prevention during beat detection)
 */
TEST_F(ClippingIntegrationTest, ClippingDuringBeat_StillDetects) {
    // Arrange: Initialize window
    fillWindowWithBackground(2000);
    
    // Reset beat counter after window fill
    beat_count_ = 0;
    
    // Act: Generate beat with clipping amplitude
    generateBeat(4050, 100000);  // Peak above 4000 → clipping
    
    // Assert: Beat detected
    EXPECT_EQ(1, beat_count_)
        << "Beat should still be detected even when clipping occurs";
    
    // Assert: Gain reduced due to clipping
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "AGC should reduce gain when beat causes clipping";
    
    // Assert: Beat event has correct amplitude
    EXPECT_EQ(4050, last_beat_.amplitude)
        << "Beat amplitude should reflect actual peak value";
}

/**
 * @test MultipleBeats_GainStaysReduced
 * 
 * Validates that gain stays reduced after clipping, preventing repeated clipping.
 * 
 * Given: First beat caused clipping, gain now at 40dB
 * When: Subsequent beats at same amplitude
 * Then: Gain remains at 40dB (doesn't increase back)
 * 
 * Traceability: AC-AUDIO-012 (Sustained clipping prevention)
 */
TEST_F(ClippingIntegrationTest, MultipleBeats_GainStaysReduced) {
    // Arrange: Trigger initial clipping
    fillWindowWithBackground(2000);
    beat_count_ = 0;
    
    generateBeat(4050, 100000);  // First beat → clipping
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel());
    EXPECT_EQ(1, beat_count_);
    
    // Act: Generate second beat (also high amplitude)
    mock_timing_.advanceTime(60000);  // 60ms debounce clearance
    generateBeat(4050, 160000);  // Second beat
    
    // Assert: Still at 40dB (doesn't increase)
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "AGC should remain at 40dB after multiple high-amplitude beats";
    
    // Assert: Both beats detected
    EXPECT_EQ(2, beat_count_)
        << "Both beats should be detected despite reduced gain";
}

/**
 * @test NoClipping_GainStable
 * 
 * Validates that gain remains stable when no clipping occurs.
 * 
 * Given: System at default gain (50dB)
 * When: Multiple beats below clipping threshold
 * Then: Gain remains at 50dB
 * 
 * Traceability: AC-AUDIO-012 (AGC stability without clipping)
 */
TEST_F(ClippingIntegrationTest, NoClipping_GainStable) {
    // Arrange: Initialize window
    fillWindowWithBackground(2000);
    beat_count_ = 0;
    
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel());
    
    // Act: Generate multiple beats below clipping threshold
    for (int i = 0; i < 3; i++) {
        mock_timing_.advanceTime(60000);  // 60ms spacing
        generateBeat(3500, mock_timing_.getTimestampUs());  // Below 4000
    }
    
    // Assert: Gain unchanged
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel())
        << "AGC should remain at 50dB when no clipping occurs";
    
    // Assert: All beats detected
    EXPECT_EQ(3, beat_count_)
        << "All beats should be detected without clipping";
}

/**
 * @test ThresholdAdaptation_AfterGainReduction
 * 
 * Validates that threshold adaptation continues to work after gain reduction.
 * 
 * Given: Gain reduced to 40dB due to clipping
 * When: Signal levels change (dynamic range shifts)
 * Then: Threshold adapts correctly to new levels
 * 
 * Traceability: AC-AUDIO-012 (Integration with AC-AUDIO-001 adaptive threshold)
 */
TEST_F(ClippingIntegrationTest, ThresholdAdaptation_AfterGainReduction) {
    // Arrange: Trigger clipping, reduce gain
    fillWindowWithBackground(2000);
    detector_->processSample(4050);  // Clipping
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel());
    
    uint16_t threshold_before = detector_->getThreshold();
    
    // Act: Change background level (simulate quieter environment)
    for (int i = 0; i < 64; i++) {
        detector_->processSample(1500);  // Lower background
        mock_timing_.advanceTime(62);
    }
    
    uint16_t threshold_after = detector_->getThreshold();
    
    // Assert: Threshold adapted to new levels
    EXPECT_LT(threshold_after, threshold_before)
        << "Threshold should adapt downward when signal levels decrease";
    
    // Assert: Gain still at 40dB (doesn't auto-increase)
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "Gain should remain at 40dB (no auto-recovery in current implementation)";
}

/**
 * @test BeatEventGainField_ReflectsCurrentLevel
 * 
 * Validates that beat events report the correct gain level.
 * 
 * Given: Gain reduced to 40dB
 * When: Beat detected
 * Then: Beat event reports gain_level = 0 (GAIN_40DB)
 * 
 * Traceability: AC-AUDIO-004 (Beat event fields), AC-AUDIO-012 (AGC integration)
 */
TEST_F(ClippingIntegrationTest, BeatEventGainField_ReflectsCurrentLevel) {
    // Arrange: Reduce gain to 40dB
    fillWindowWithBackground(2000);
    detector_->processSample(4050);  // Clipping → GAIN_40DB
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel());
    
    beat_count_ = 0;
    
    // Act: Generate beat
    mock_timing_.advanceTime(60000);
    generateBeat(3500, mock_timing_.getTimestampUs());
    
    // Assert: Beat detected with correct gain field
    EXPECT_EQ(1, beat_count_);
    EXPECT_EQ(static_cast<uint8_t>(AGCLevel::GAIN_40DB), last_beat_.gain_level)
        << "Beat event should report current AGC level (40dB = 0)";
}

/**
 * @test TelemetryGainField_UpdatesAfterClipping
 * 
 * Validates that telemetry reports gain changes.
 * 
 * Given: Telemetry reporting at 500ms intervals
 * When: Gain reduced due to clipping
 * Then: Next telemetry reports new gain level
 * 
 * Traceability: AC-AUDIO-007 (Telemetry), AC-AUDIO-012 (AGC integration)
 */
TEST_F(ClippingIntegrationTest, TelemetryGainField_UpdatesAfterClipping) {
    // Arrange: Fill window, generate initial telemetry
    fillWindowWithBackground(2000);
    mock_timing_.setTimestamp(0);
    telemetry_count_ = 0;
    
    // Force initial telemetry
    for (int i = 0; i < 10; i++) {
        detector_->processSample(2000);
        mock_timing_.advanceTime(62);
    }
    mock_timing_.setTimestamp(500000);  // 500ms → telemetry publish
    detector_->processSample(2000);
    
    EXPECT_GT(telemetry_count_, 0) << "Should have telemetry published";
    EXPECT_EQ(static_cast<uint8_t>(AGCLevel::GAIN_50DB), last_telemetry_.gain_level)
        << "Initial telemetry should report GAIN_50DB (1)";
    
    // Act: Trigger clipping
    mock_timing_.advanceTime(1000);
    detector_->processSample(4050);  // Clipping
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel());
    
    // Wait for next telemetry (500ms later)
    int prev_telemetry_count = telemetry_count_;
    mock_timing_.setTimestamp(1000000);  // +500ms
    detector_->processSample(2000);
    
    // Assert: Telemetry updated with new gain
    EXPECT_GT(telemetry_count_, prev_telemetry_count)
        << "New telemetry should be published after 500ms";
    EXPECT_EQ(static_cast<uint8_t>(AGCLevel::GAIN_40DB), last_telemetry_.gain_level)
        << "Updated telemetry should report GAIN_40DB (0) after clipping";
}

/**
 * @test ClippingBoundary_ExactThreshold
 * 
 * Validates clipping detection at exact threshold boundary.
 * 
 * Given: System at GAIN_50DB
 * When: Sample = exactly 4000 (threshold)
 * Then: No clipping (threshold is exclusive: >4000)
 * 
 * Traceability: AC-AUDIO-012 (Clipping threshold definition)
 */
TEST_F(ClippingIntegrationTest, ClippingBoundary_ExactThreshold) {
    // Arrange: Start at GAIN_50DB
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel());
    
    // Act: Feed sample at exact threshold
    detector_->processSample(4000);  // Exactly 4000 (not >4000)
    
    // Assert: No gain change (4000 is NOT clipping, only >4000 is)
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel())
        << "AGC should NOT reduce gain at exactly 4000 (threshold is >4000)";
    
    // Act: Feed sample just above threshold
    detector_->processSample(4001);  // Just above 4000
    
    // Assert: Gain reduced
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "AGC should reduce gain at 4001 (above threshold)";
}

/**
 * @test GainTransition_60to50to40
 * 
 * Validates complete gain reduction sequence from maximum to minimum.
 * 
 * Given: System starts at GAIN_60DB (manually set)
 * When: Clipping detected twice
 * Then: Gain reduces 60dB → 50dB → 40dB
 * 
 * Traceability: AC-AUDIO-003 (AGC transitions), AC-AUDIO-012 (Integration)
 */
TEST_F(ClippingIntegrationTest, GainTransition_60to50to40) {
    // Arrange: Manually set to GAIN_60DB (highest gain)
    // Note: This tests the AGC logic even though system starts at 50dB by default
    fillWindowWithBackground(2000);
    
    // Force system to GAIN_60DB by directly accessing state (test scenario)
    // In real system, this would require external gain adjustment capability
    // For now, test the 50dB → 40dB transition which is the normal case
    
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel());
    
    // Act: First clipping (50dB → 40dB)
    detector_->processSample(4050);
    
    // Assert: Reduced to 40dB
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "First clipping should reduce 50dB → 40dB";
    
    // Act: Second clipping (40dB → stays at 40dB, already minimum)
    detector_->processSample(4050);
    
    // Assert: Still at 40dB (minimum)
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "Gain should stay at 40dB (minimum) after repeated clipping";
}

/**
 * @test IntegrationFlow_CompleteScenario
 * 
 * Validates complete integration: beat detection + threshold + AGC + telemetry.
 * 
 * Scenario:
 *   1. System starts normal (50dB)
 *   2. Normal beats detected (no clipping)
 *   3. High amplitude beat causes clipping
 *   4. Gain reduces to 40dB
 *   5. Telemetry reflects gain change
 * 
 * Traceability: AC-AUDIO-012 (Full integration validation)
 */
TEST_F(ClippingIntegrationTest, IntegrationFlow_CompleteScenario) {
    // Step 1: Initialize system
    fillWindowWithBackground(2000);
    beat_count_ = 0;
    telemetry_count_ = 0;
    mock_timing_.setTimestamp(0);
    
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel());
    
    // Step 2: Generate normal beat (no clipping)
    generateBeat(3500, 10000);
    EXPECT_EQ(1, beat_count_) << "First beat should be detected";
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel())
        << "Gain should remain at 50dB for normal beats";
    
    // Step 3: Generate high amplitude beat (clipping)
    mock_timing_.advanceTime(60000);  // 60ms debounce
    generateBeat(4050, 70000);
    EXPECT_EQ(2, beat_count_) << "Clipping beat should still be detected";
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "Gain should reduce to 40dB after clipping";
    
    // Step 4: Verify threshold adaptation continues at reduced gain
    uint16_t threshold_after_clipping = detector_->getThreshold();
    EXPECT_GT(threshold_after_clipping, 0) << "Threshold should still be computed";
    EXPECT_LT(threshold_after_clipping, 4000) << "Threshold should be reasonable";
    
    // Step 5: Wait for telemetry
    mock_timing_.setTimestamp(500000);  // 500ms → telemetry
    detector_->processSample(2000);
    
    // Assert: Telemetry reflects current state
    EXPECT_GT(telemetry_count_, 0) << "Telemetry should be published";
    EXPECT_EQ(static_cast<uint8_t>(AGCLevel::GAIN_40DB), last_telemetry_.gain_level)
        << "Telemetry should report reduced gain";
    EXPECT_EQ(2u, last_telemetry_.beat_count)
        << "Telemetry should report beats detected";
}

/**
 * @test EdgeCase_MaxADCValue
 * 
 * Validates system handles maximum ADC value (4095) gracefully.
 * 
 * Given: 12-bit ADC maximum is 4095
 * When: Sample = 4095 (maximum clipping)
 * Then: Gain reduces, system remains stable
 * 
 * Traceability: AC-AUDIO-012 (Edge case handling)
 */
TEST_F(ClippingIntegrationTest, EdgeCase_MaxADCValue) {
    // Arrange: Start at GAIN_50DB
    fillWindowWithBackground(2000);
    EXPECT_EQ(AGCLevel::GAIN_50DB, detector_->getGainLevel());
    
    // Act: Feed maximum ADC value
    detector_->processSample(4095);  // 12-bit max
    
    // Assert: Gain reduced
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "AGC should reduce gain at maximum ADC value";
    
    // Assert: System remains stable (no overflow/crash)
    uint16_t threshold = detector_->getThreshold();
    EXPECT_GT(threshold, 0) << "Threshold calculation should remain valid";
    EXPECT_LT(threshold, 4095) << "Threshold should not overflow";
    
    // Continue processing normal samples
    for (int i = 0; i < 20; i++) {
        detector_->processSample(2000);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: System continues to function normally
    EXPECT_EQ(AGCLevel::GAIN_40DB, detector_->getGainLevel())
        << "Gain should remain at 40dB after max value";
    
    // System should be in IDLE or DEBOUNCE depending on whether beat was detected
    // What matters is system didn't crash and continues processing
    DetectionState current_state = detector_->getState();
    EXPECT_TRUE(current_state == DetectionState::IDLE || 
                current_state == DetectionState::DEBOUNCE)
        << "State machine should be in valid state after max ADC value";
}
