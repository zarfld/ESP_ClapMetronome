/**
 * @file test_detection_accuracy.cpp
 * @brief TDD Cycle 9 - AC-AUDIO-009 Detection Accuracy Tests
 * 
 * Tests validate >95% true positive rate and <5% false positive rate
 * for audio beat detection under varied conditions.
 * 
 * AC-AUDIO-009: Detection accuracy
 *   - Pass condition: >95% of 100 kicks detected
 *   - QA Scenario: QA-SC-001 (hardware-in-loop with oscilloscope)
 *   - Unit test approach: Statistical validation with simulated signals
 * 
 * TDD Cycle: RED → GREEN → REFACTOR
 *   Phase: RED - Writing tests first
 *   Expected: Initial failures or validation pass
 * 
 * @author TDD Cycle 9
 * @date 2025-11-12
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../../test/mocks/MockTimingProvider.h"
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace clap_metronome;

// Test fixture
class DetectionAccuracyTest : public ::testing::Test {
protected:
    MockTimingProvider mockTiming_;
    AudioDetection* detector_;
    std::vector<BeatEvent> beatEvents_;

    void SetUp() override {
        detector_ = new AudioDetection(&mockTiming_);
        detector_->init();
        detector_->onBeat([this](const BeatEvent& event) {
            beatEvents_.push_back(event);
        });
        mockTiming_.setTimestamp(0);
        beatEvents_.clear();
    }

    void TearDown() override {
        delete detector_;
    }

    /**
     * @brief Generate a simulated beat signal with specified characteristics
     * @param peakValue Peak amplitude of the beat
     * @param riseTimeSamples Number of samples for attack phase
     * @param decayTimeSamples Number of samples for decay phase
     * @param baselineValue Baseline noise level
     * @return Vector of audio samples representing one beat
     */
    std::vector<uint16_t> generateBeatSignal(
        uint16_t peakValue,
        size_t riseTimeSamples,
        size_t decayTimeSamples,
        uint16_t baselineValue = 2048
    ) {
        std::vector<uint16_t> signal;
        
        // Baseline before beat
        for (size_t i = 0; i < 20; ++i) {
            signal.push_back(baselineValue);
        }
        
        // Attack phase (rise to peak)
        for (size_t i = 0; i < riseTimeSamples; ++i) {
            double progress = static_cast<double>(i) / riseTimeSamples;
            uint16_t value = baselineValue + static_cast<uint16_t>(
                (peakValue - baselineValue) * progress
            );
            signal.push_back(value);
        }
        
        // Peak hold (2 samples)
        signal.push_back(peakValue);
        signal.push_back(peakValue);
        
        // Decay phase (fall back to baseline)
        for (size_t i = 0; i < decayTimeSamples; ++i) {
            double progress = static_cast<double>(i) / decayTimeSamples;
            uint16_t value = peakValue - static_cast<uint16_t>(
                (peakValue - baselineValue) * progress
            );
            signal.push_back(value);
        }
        
        // Baseline after beat
        for (size_t i = 0; i < 20; ++i) {
            signal.push_back(baselineValue);
        }
        
        return signal;
    }

    /**
     * @brief Add random noise to a signal
     * @param signal Signal to modify
     * @param noiseLevel Amplitude of noise (+/- range)
     */
    void addNoise(std::vector<uint16_t>& signal, uint16_t noiseLevel) {
        for (auto& sample : signal) {
            int noise = (rand() % (2 * noiseLevel + 1)) - static_cast<int>(noiseLevel);
            int noisySample = static_cast<int>(sample) + noise;
            // Clamp to valid range
            noisySample = std::max(0, std::min(4095, noisySample));
            sample = static_cast<uint16_t>(noisySample);
        }
    }

    /**
     * @brief Process a signal and count detected beats
     * @param signal Audio samples to process
     * @return Number of beats actually detected
     */
    int processSignalAndCountBeats(
        const std::vector<uint16_t>& signal
    ) {
        beatEvents_.clear();
        mockTiming_.setTimestamp(0);
        
        for (uint16_t sample : signal) {
            detector_->processSample(sample);
            mockTiming_.advanceTime(62); // 16kHz sample rate (62.5μs)
        }
        
        int detectedBeats = static_cast<int>(beatEvents_.size());
        return detectedBeats;
    }

    /**
     * @brief Calculate detection rate as percentage
     * @param detected Number of beats detected
     * @param total Total number of beats in signal
     * @return Detection rate as percentage (0.0 to 100.0)
     */
    double calculateDetectionRate(int detected, int total) {
        return (static_cast<double>(detected) / total) * 100.0;
    }
};

/**
 * @test TruePositiveRate_StrongSignals
 * @brief Validate >95% detection rate with strong beat signals
 * 
 * AC-AUDIO-009: >95% of 100 kicks detected
 * Test: Strong signals (3500 ADC units, well above threshold)
 * Expected: 100% detection (stronger than requirement)
 */
TEST_F(DetectionAccuracyTest, TruePositiveRate_StrongSignals) {
    const int NUM_BEATS = 100;
    int totalDetected = 0;
    
    // Process 100 strong beats
    for (int i = 0; i < NUM_BEATS; ++i) {
        auto beatSignal = generateBeatSignal(
            3500,  // Strong peak (well above threshold)
            16,    // Fast attack (1ms at 16kHz)
            80,    // Normal decay (5ms)
            2048   // Mid-range baseline
        );
        
        int detected = processSignalAndCountBeats(beatSignal);
        totalDetected += detected;
        
        // Reset for next beat (debounce period)
        mockTiming_.advanceTime(100000); // 100ms gap
    }
    
    double detectionRate = calculateDetectionRate(totalDetected, NUM_BEATS);
    
    EXPECT_GE(detectionRate, 95.0)
        << "Strong signals should have >95% detection rate. Got: "
        << detectionRate << "%";
    
    // With strong signals, expect near-perfect detection
    EXPECT_GE(detectionRate, 98.0)
        << "Strong signals should approach 100% detection";
}

/**
 * @test TruePositiveRate_MediumSignals
 * @brief Validate >95% detection rate with medium-strength signals
 * 
 * AC-AUDIO-009: >95% of 100 kicks detected
 * Test: Medium signals (3000 ADC units, moderately above threshold)
 * Expected: >95% detection (meets requirement)
 */
TEST_F(DetectionAccuracyTest, TruePositiveRate_MediumSignals) {
    const int NUM_BEATS = 100;
    int totalDetected = 0;
    
    // Process 100 medium-strength beats
    for (int i = 0; i < NUM_BEATS; ++i) {
        auto beatSignal = generateBeatSignal(
            3000,  // Medium peak (moderately above threshold)
            16,    // Fast attack
            80,    // Normal decay
            2048   // Mid-range baseline
        );
        
        int detected = processSignalAndCountBeats(beatSignal);
        totalDetected += detected;
        
        mockTiming_.advanceTime(100000); // 100ms gap
    }
    
    double detectionRate = calculateDetectionRate(totalDetected, NUM_BEATS);
    
    EXPECT_GE(detectionRate, 95.0)
        << "Medium signals must achieve >95% detection rate (AC-AUDIO-009). Got: "
        << detectionRate << "%";
}

/**
 * @test TruePositiveRate_WeakSignals
 * @brief Measure detection rate with weak signals near threshold
 * 
 * Test: Weak signals (2600 ADC units, just above threshold)
 * Expected: Lower detection rate acceptable (documents sensitivity limit)
 * Note: AC-AUDIO-009 assumes normal kick strength, not weak signals
 */
TEST_F(DetectionAccuracyTest, TruePositiveRate_WeakSignals) {
    const int NUM_BEATS = 100;
    int totalDetected = 0;
    
    // Process 100 weak beats (near threshold)
    for (int i = 0; i < NUM_BEATS; ++i) {
        auto beatSignal = generateBeatSignal(
            2600,  // Weak peak (near threshold)
            16,    // Fast attack
            80,    // Normal decay
            2048   // Mid-range baseline
        );
        
        int detected = processSignalAndCountBeats(beatSignal);
        totalDetected += detected;
        
        mockTiming_.advanceTime(100000);
    }
    
    double detectionRate = calculateDetectionRate(totalDetected, NUM_BEATS);
    
    // Document detection rate for weak signals
    EXPECT_GT(detectionRate, 0.0)
        << "Weak signals should still trigger some detections";
    
    // Weak signals may not meet 95% (that's okay - not typical use case)
    // But rate should be reasonable (>70% as sanity check)
    EXPECT_GE(detectionRate, 70.0)
        << "Even weak signals should detect >70%. Got: " << detectionRate << "%";
}

/**
 * @test TruePositiveRate_WithBackgroundNoise
 * @brief Validate >95% detection with realistic background noise
 * 
 * AC-AUDIO-009: >95% detection in real-world conditions
 * Test: Medium signals + moderate noise (SNR ~15dB)
 * Expected: >95% detection maintained
 */
TEST_F(DetectionAccuracyTest, TruePositiveRate_WithBackgroundNoise) {
    const int NUM_BEATS = 100;
    int totalDetected = 0;
    
    // Process 100 beats with background noise
    for (int i = 0; i < NUM_BEATS; ++i) {
        auto beatSignal = generateBeatSignal(
            3200,  // Strong enough to overcome noise
            16,    // Fast attack
            80,    // Normal decay
            2048   // Mid-range baseline
        );
        
        // Add realistic background noise
        addNoise(beatSignal, 50); // +/-50 ADC units (~2.4% of range)
        
        int detected = processSignalAndCountBeats(beatSignal);
        totalDetected += detected;
        
        mockTiming_.advanceTime(100000);
    }
    
    double detectionRate = calculateDetectionRate(totalDetected, NUM_BEATS);
    
    EXPECT_GE(detectionRate, 95.0)
        << "Detection must maintain >95% rate with background noise (AC-AUDIO-009). Got: "
        << detectionRate << "%";
}

/**
 * @test FalsePositiveRate_RandomNoise
 * @brief Validate <5% false positive rate with random noise
 * 
 * AC-AUDIO-009 (implied): <5% false positives
 * Test: Random noise without beats
 * Expected: <5% spurious detections
 */
TEST_F(DetectionAccuracyTest, FalsePositiveRate_RandomNoise) {
    const int NUM_WINDOWS = 100; // Equivalent to 100 "no-beat" periods
    int falsePositives = 0;
    
    // Process 100 windows of random noise (no beats)
    for (int i = 0; i < NUM_WINDOWS; ++i) {
        beatEvents_.clear();
        mockTiming_.setTimestamp(i * 100000UL); // 100ms windows
        
        // Generate 160 samples of random noise (10ms window at 16kHz)
        for (int j = 0; j < 160; ++j) {
            uint16_t noise = 2048 + (rand() % 200) - 100; // +/-100 ADC units
            detector_->processSample(noise);
            mockTiming_.advanceTime(62);
        }
        
        // Count false detections
        falsePositives += static_cast<int>(beatEvents_.size());
    }
    
    double falsePositiveRate = calculateDetectionRate(falsePositives, NUM_WINDOWS);
    
    EXPECT_LT(falsePositiveRate, 5.0)
        << "False positive rate must be <5% (AC-AUDIO-009). Got: "
        << falsePositiveRate << "%";
    
    // Ideally, should be much lower
    EXPECT_LT(falsePositiveRate, 2.0)
        << "False positive rate should be very low with pure noise";
}

/**
 * @test FalsePositiveRate_QuietBaseline
 * @brief Validate no false positives with quiet baseline signal
 * 
 * Test: Quiet baseline (no noise, no beats)
 * Expected: 0% false positives (perfect rejection)
 */
TEST_F(DetectionAccuracyTest, FalsePositiveRate_QuietBaseline) {
    const int NUM_WINDOWS = 100;
    int falsePositives = 0;
    
    // Process 100 windows of quiet baseline
    for (int i = 0; i < NUM_WINDOWS; ++i) {
        beatEvents_.clear();
        mockTiming_.setTimestamp(i * 100000UL);
        
        // Generate 160 samples of stable baseline
        for (int j = 0; j < 160; ++j) {
            detector_->processSample(2048); // Perfect baseline
            mockTiming_.advanceTime(62);
        }
        
        falsePositives += static_cast<int>(beatEvents_.size());
    }
    
    double falsePositiveRate = calculateDetectionRate(falsePositives, NUM_WINDOWS);
    
    EXPECT_EQ(falsePositiveRate, 0.0)
        << "Quiet baseline should produce zero false positives";
}

/**
 * @test EdgeCase_SignalsNearThreshold
 * @brief Test detection behavior at threshold boundary
 * 
 * Test: Signals right at adaptive threshold level
 * Expected: Consistent behavior (either detect all or none)
 */
TEST_F(DetectionAccuracyTest, EdgeCase_SignalsNearThreshold) {
    // Let adaptive threshold stabilize
    for (int i = 0; i < 1000; ++i) {
        detector_->processSample(2048);
        mockTiming_.advanceTime(62);
    }
    
    // Get current threshold (should be ~2048 + margin)
    // Test with signals just above and just below
    
    const int NUM_BEATS = 20;
    int detectedAbove = 0;
    int detectedBelow = 0;
    
    // Test signals slightly above threshold
    for (int i = 0; i < NUM_BEATS; ++i) {
        auto beatSignal = generateBeatSignal(2700, 16, 80, 2048);
        detectedAbove += processSignalAndCountBeats(beatSignal);
        mockTiming_.advanceTime(100000);
    }
    
    // Test signals slightly below threshold
    for (int i = 0; i < NUM_BEATS; ++i) {
        auto beatSignal = generateBeatSignal(2400, 16, 80, 2048);
        detectedBelow += processSignalAndCountBeats(beatSignal);
        mockTiming_.advanceTime(100000);
    }
    
    // Signals above threshold should be detected
    EXPECT_GT(detectedAbove, NUM_BEATS / 2)
        << "Signals above threshold should be mostly detected";
    
    // Signals below threshold should be mostly rejected
    EXPECT_LT(detectedBelow, NUM_BEATS / 2)
        << "Signals below threshold should be mostly rejected";
}

/**
 * @test StatisticalConfidence_100BeatSequence
 * @brief Validate AC-AUDIO-009 with continuous 100-beat test
 * 
 * AC-AUDIO-009: >95% of 100 kicks detected (primary acceptance criterion)
 * Test: Continuous sequence of 100 medium-strength beats
 * Expected: >95 beats detected (at least 95 out of 100)
 */
TEST_F(DetectionAccuracyTest, StatisticalConfidence_100BeatSequence) {
    const int NUM_BEATS = 100;
    std::vector<bool> detectionResults(NUM_BEATS, false);
    
    // Process 100 beats in sequence
    for (int i = 0; i < NUM_BEATS; ++i) {
        beatEvents_.clear();
        
        auto beatSignal = generateBeatSignal(
            3100,  // Solid medium-strong signal
            16,    // Fast attack (1ms)
            80,    // Normal decay (5ms)
            2048   // Mid-range baseline
        );
        
        // Add light noise for realism
        addNoise(beatSignal, 30);
        
        // Process signal
        mockTiming_.setTimestamp(i * 150000UL); // 150ms between beats
        for (uint16_t sample : beatSignal) {
            detector_->processSample(sample);
            mockTiming_.advanceTime(62);
        }
        
        // Record if beat was detected
        detectionResults[i] = (!beatEvents_.empty());
    }
    
    // Count total detections
    int totalDetected = static_cast<int>(std::count(detectionResults.begin(), detectionResults.end(), true));
    double detectionRate = calculateDetectionRate(totalDetected, NUM_BEATS);
    
    // AC-AUDIO-009 pass criterion
    EXPECT_GE(totalDetected, 95)
        << "AC-AUDIO-009 requires >95 of 100 kicks detected. Got: "
        << totalDetected << " (" << detectionRate << "%)";
    
    EXPECT_GE(detectionRate, 95.0)
        << "Detection rate must be >95% (AC-AUDIO-009)";
    
    // Log any missed detections for analysis
    if (totalDetected < 100) {
        std::cout << "Missed detections at beat indices: ";
        for (size_t i = 0; i < detectionResults.size(); ++i) {
            if (!detectionResults[i]) {
                std::cout << i << " ";
            }
        }
        std::cout << std::endl;
    }
}

/**
 * @test RealWorldScenario_VariedBeatStrengths
 * @brief Validate accuracy with realistic varied beat strengths
 * 
 * Test: Mix of strong, medium, and weak beats (realistic drumming)
 * Expected: >95% detection across varied intensities
 */
TEST_F(DetectionAccuracyTest, RealWorldScenario_VariedBeatStrengths) {
    const int NUM_BEATS = 100;
    int totalDetected = 0;
    
    // Process 100 beats with varied strengths (simulating real drumming)
    for (int i = 0; i < NUM_BEATS; ++i) {
        uint16_t peakValue;
        
        // Vary beat strength: 60% strong, 30% medium, 10% weak
        int strength = i % 10;
        if (strength < 6) {
            peakValue = 3300; // Strong beats
        } else if (strength < 9) {
            peakValue = 2900; // Medium beats
        } else {
            peakValue = 2650; // Weak beats
        }
        
        auto beatSignal = generateBeatSignal(peakValue, 16, 80, 2048);
        addNoise(beatSignal, 40); // Realistic noise
        
        int detected = processSignalAndCountBeats(beatSignal);
        totalDetected += detected;
        
        mockTiming_.advanceTime(120000); // 120ms typical spacing
    }
    
    double detectionRate = calculateDetectionRate(totalDetected, NUM_BEATS);
    
    EXPECT_GE(detectionRate, 95.0)
        << "Real-world varied beat strengths must achieve >95% detection (AC-AUDIO-009). Got: "
        << detectionRate << "%";
}
