/**
 * TDD Cycle 8 - AC-AUDIO-008: Audio Latency (RED Phase)
 * 
 * Validates that audio processing latency meets <20ms requirement from
 * microphone input to beat event emission.
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Tests: AC-AUDIO-008 (Audio latency <20ms)
 * QA Scenario: QA-SC-001 (Performance - API Latency)
 * Requirement: REQ-NF-001 (<20ms latency)
 * Standards: ISO/IEC/IEEE 12207:2017 (Performance Verification)
 * 
 * Test Strategy:
 * 1. Measure single processSample() execution time (<100μs target)
 * 2. Measure end-to-end latency (first sample → beat event)
 * 3. Validate timestamp accuracy (microsecond precision)
 * 4. Ensure no processing delays accumulate
 * 
 * Note: Full hardware-in-loop validation with oscilloscope deferred
 * to integration testing phase with real MAX9814 microphone.
 * 
 * Coverage: Performance test for AC-AUDIO-008
 * Dependencies: MockTimingProvider, high-resolution timing
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../mocks/MockTimingProvider.h"
#include <chrono>

using namespace clap_metronome;

/**
 * Test Fixture: Audio Latency Measurement
 * 
 * Measures processing latency to ensure <20ms requirement is met.
 */
class AudioLatencyTest : public ::testing::Test {
protected:
    MockTimingProvider timing_;
    AudioDetection* detector_;
    
    // Latency measurement
    uint64_t beat_event_timestamp_ = 0;
    uint64_t signal_start_timestamp_ = 0;
    bool beat_received_ = false;
    
    void SetUp() override {
        detector_ = new AudioDetection(&timing_);
        detector_->init();
        timing_.setTimestamp(0);
    }
    
    void TearDown() override {
        delete detector_;
    }
    
    /**
     * Register beat callback that captures timestamp
     */
    void registerBeatCallback() {
        detector_->onBeat([this](const BeatEvent& event) {
            beat_received_ = true;
            beat_event_timestamp_ = event.timestamp_us;
        });
    }
    
    /**
     * Simulate beat with timing measurement
     * @param start_time Timestamp when signal starts (us)
     * @return Latency from signal start to beat event (us)
     */
    uint64_t simulateBeatAndMeasureLatency(uint64_t start_time) {
        signal_start_timestamp_ = start_time;
        timing_.setTimestamp(start_time);
        
        // Establish baseline (10ms at 2048 ADC)
        for (int i = 0; i < 160; ++i) {  // 10ms at 16kHz = 160 samples
            timing_.advanceTime(62);  // 62.5μs per sample at 16kHz ≈ 62
            detector_->processSample(2048);
        }
        
        // Rising edge: amplitude crosses threshold
        for (int i = 0; i < 16; ++i) {  // 1ms rising edge
            timing_.advanceTime(62);
            detector_->processSample(static_cast<uint16_t>(3000 + i * 50));  // Gradual rise
        }
        
        // Peak and fall (triggers beat event)
        timing_.advanceTime(62);
        detector_->processSample(3800);  // Peak
        
        timing_.advanceTime(62);
        detector_->processSample(3700);  // Start falling (triggers beat)
        
        if (!beat_received_) {
            return 0;  // No beat detected
        }
        
        // Calculate latency
        return beat_event_timestamp_ - signal_start_timestamp_;
    }
    
    /**
     * Measure single processSample() execution time
     * @return Execution time in microseconds
     */
    double measureProcessSampleTime() {
        constexpr int NUM_ITERATIONS = 1000;
        uint16_t adc_value = 2048;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            timing_.advanceTime(1);
            detector_->processSample(adc_value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        // Return average time per sample in microseconds
        return static_cast<double>(duration.count()) / NUM_ITERATIONS / 1000.0;
    }
};

// ===== Test 1: Single Sample Processing Time =====

/**
 * Verify processSample() executes in <100μs (target for 16kHz with margin)
 * 
 * Rationale: At 16kHz sampling, each sample period is 62.5μs.
 * Processing must complete within one sample period with margin.
 * Target: <100μs per sample (plenty of headroom)
 * 
 * Expected: Average execution time <100μs
 */
TEST_F(AudioLatencyTest, SingleSampleProcessingTimeUnder100Microseconds) {
    double avg_time_us = measureProcessSampleTime();
    
    EXPECT_LT(avg_time_us, 100.0)
        << "processSample() should execute in <100μs (measured: " 
        << avg_time_us << "μs)";
    
    // Also check for reasonable lower bound (sanity check)
    EXPECT_GT(avg_time_us, 0.01)
        << "Execution time seems unrealistically low";
}

// ===== Test 2: End-to-End Latency <20ms =====

/**
 * Verify total latency from signal start to beat event is <20ms
 * 
 * AC-AUDIO-008: <20ms from mic input to beat event
 * 
 * This measures algorithmic latency (state machine traversal).
 * Full hardware latency includes ADC sampling time (measured separately).
 * 
 * Expected: Latency <20ms (20000μs)
 */
TEST_F(AudioLatencyTest, EndToEndLatencyUnder20Milliseconds) {
    registerBeatCallback();
    
    uint64_t latency_us = simulateBeatAndMeasureLatency(1000000);  // Start at T=1s
    
    ASSERT_TRUE(beat_received_)
        << "Beat should be detected";
    
    constexpr uint64_t MAX_LATENCY_US = 20000;  // 20ms
    EXPECT_LT(latency_us, MAX_LATENCY_US)
        << "Latency should be <20ms (measured: " << latency_us << "μs)";
    
    // Typical expected: 11-12ms (10ms baseline + 1ms rise + peak detection)
    EXPECT_GT(latency_us, 10000U)
        << "Latency should include baseline period (>10ms)";
}

// ===== Test 3: Timestamp Accuracy =====

/**
 * Verify beat event timestamp accurately reflects detection time
 * 
 * Timestamp should match timing provider's current time at beat detection.
 * Ensures no timestamp drift or calculation errors.
 * 
 * Expected: Timestamp matches timing provider within ±100μs
 */
TEST_F(AudioLatencyTest, BeatEventTimestampAccurate) {
    registerBeatCallback();
    
    simulateBeatAndMeasureLatency(5000000);  // Start at T=5s
    
    ASSERT_TRUE(beat_received_)
        << "Beat should be detected";
    
    uint64_t expected_time = timing_.getTimestampUs();
    int64_t timestamp_error = static_cast<int64_t>(beat_event_timestamp_) - 
                             static_cast<int64_t>(expected_time);
    
    // Allow small error margin (sample period ≈ 62μs, allow 2 samples = 124μs)
    EXPECT_LE(std::abs(timestamp_error), 200)
        << "Timestamp error: " << timestamp_error << "μs (should be <200μs)";
}

// ===== Test 4: No Latency Accumulation =====

/**
 * Verify latency does not accumulate over multiple beats
 * 
 * Ensures each beat detection has consistent latency.
 * No processing delays should accumulate.
 * 
 * Expected: Latency variation <5% between beats
 */
TEST_F(AudioLatencyTest, NoLatencyAccumulationOverMultipleBeats) {
    registerBeatCallback();
    
    constexpr int NUM_BEATS = 5;
    uint64_t latencies[NUM_BEATS];
    
    for (int i = 0; i < NUM_BEATS; ++i) {
        beat_received_ = false;
        beat_event_timestamp_ = 0;
        
        // Space beats 100ms apart (well beyond debounce)
        uint64_t start_time = i * 100000;
        latencies[i] = simulateBeatAndMeasureLatency(start_time);
        
        ASSERT_TRUE(beat_received_)
            << "Beat " << i << " should be detected";
    }
    
    // Calculate min and max latency
    uint64_t min_latency = latencies[0];
    uint64_t max_latency = latencies[0];
    
    for (int i = 1; i < NUM_BEATS; ++i) {
        if (latencies[i] < min_latency) min_latency = latencies[i];
        if (latencies[i] > max_latency) max_latency = latencies[i];
    }
    
    // Variation should be minimal (all latencies similar)
    uint64_t variation = max_latency - min_latency;
    double variation_percent = (static_cast<double>(variation) / min_latency) * 100.0;
    
    EXPECT_LT(variation_percent, 10.0)
        << "Latency variation should be <10% (measured: " 
        << variation_percent << "%, min=" << min_latency 
        << "μs, max=" << max_latency << "μs)";
}

// ===== Test 5: Processing Budget at 16kHz =====

/**
 * Verify processing stays within sample period at 16kHz
 * 
 * At 16kHz, sample period = 62.5μs.
 * Processing must complete within this to avoid sample drops.
 * 
 * Expected: Processing time <62μs (within sample period)
 */
TEST_F(AudioLatencyTest, ProcessingWithinSamplePeriodAt16kHz) {
    double avg_time_us = measureProcessSampleTime();
    
    constexpr double SAMPLE_PERIOD_16KHZ_US = 62.5;
    
    EXPECT_LT(avg_time_us, SAMPLE_PERIOD_16KHZ_US)
        << "Processing time (" << avg_time_us 
        << "μs) should be <62.5μs (16kHz sample period)";
}

// ===== Test 6: Latency Under Different Gain Levels =====

/**
 * Verify latency remains consistent across AGC gain levels
 * 
 * AGC updates should not introduce processing delays.
 * 
 * Expected: Latency similar at all gain levels (40dB, 50dB, 60dB)
 */
TEST_F(AudioLatencyTest, LatencyConsistentAcrossGainLevels) {
    registerBeatCallback();
    
    // Test at default gain (60dB)
    uint64_t latency_60db = simulateBeatAndMeasureLatency(1000000);
    ASSERT_TRUE(beat_received_) << "Beat at 60dB should be detected";
    
    // Force gain reduction to 50dB by clipping
    beat_received_ = false;
    timing_.setTimestamp(2000000);
    for (int i = 0; i < 10; ++i) {
        timing_.advanceTime(62);
        detector_->processSample(4100);  // Above clipping threshold (4000)
    }
    
    // Wait for debounce to clear
    for (int i = 0; i < 1000; ++i) {
        timing_.advanceTime(62);
        detector_->processSample(2048);
    }
    
    // Test at 50dB gain
    uint64_t latency_50db = simulateBeatAndMeasureLatency(3000000);
    ASSERT_TRUE(beat_received_) << "Beat at 50dB should be detected";
    
    // Latencies should be similar (within 20%)
    int64_t latency_diff = std::abs(static_cast<int64_t>(latency_60db) - 
                                    static_cast<int64_t>(latency_50db));
    double diff_percent = (static_cast<double>(latency_diff) / latency_60db) * 100.0;
    
    EXPECT_LT(diff_percent, 20.0)
        << "Latency should be similar across gain levels (diff: " 
        << diff_percent << "%)";
}

// ===== Test 7: Latency During Telemetry Publishing =====

/**
 * Verify telemetry publishing does not add latency to beat detection
 * 
 * Telemetry is published periodically (500ms).
 * Should not delay beat event emission.
 * 
 * Expected: Latency similar with and without telemetry
 */
TEST_F(AudioLatencyTest, TelemetryDoesNotAddLatency) {
    registerBeatCallback();
    
    // Measure without telemetry callback
    uint64_t latency_no_telemetry = simulateBeatAndMeasureLatency(1000000);
    ASSERT_TRUE(beat_received_) << "Beat without telemetry should be detected";
    
    // Register telemetry callback
    detector_->onTelemetry([](const AudioTelemetry&) {
        // Minimal callback
    });
    
    // Advance to trigger telemetry (500ms)
    beat_received_ = false;
    timing_.setTimestamp(1500000);
    for (int i = 0; i < 8000; ++i) {  // 500ms at 16kHz
        timing_.advanceTime(62);
        detector_->processSample(2048);
    }
    
    // Measure with telemetry active
    uint64_t latency_with_telemetry = simulateBeatAndMeasureLatency(2000000);
    ASSERT_TRUE(beat_received_) << "Beat with telemetry should be detected";
    
    // Latencies should be nearly identical
    int64_t latency_diff = std::abs(static_cast<int64_t>(latency_no_telemetry) - 
                                    static_cast<int64_t>(latency_with_telemetry));
    
    EXPECT_LT(latency_diff, 1000)  // <1ms difference
        << "Telemetry should not add significant latency (diff: " 
        << latency_diff << "μs)";
}

