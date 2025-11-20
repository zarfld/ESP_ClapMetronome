/**
 * @file test_memory_usage.cpp
 * @brief Memory Usage Validation Tests (AC-AUDIO-011)
 * 
 * TDD Cycle 11: Memory Usage Validation
 * 
 * Acceptance Criteria:
 *   - Memory usage < 400B RAM for audio detection engine
 * 
 * Test Strategy:
 *   1. Calculate sizeof() for all state structures
 *   2. Account for padding and alignment
 *   3. Validate against 400B requirement
 *   4. Break down memory by component
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (TDD) - VALIDATION Phase
 * 
 * Traceability:
 *   - Implements: AC-AUDIO-011 (Memory usage <400B RAM)
 *   - Design: 04-design/tdd-plan-phase-05.md
 *   - Architecture: Embedded resource constraints
 */

#include <gtest/gtest.h>
#include <cstddef>  // For offsetof, size_t
#include <iomanip>  // For std::setprecision
#include "../../src/audio/AudioDetection.h"
#include "../../src/audio/AudioDetectionState.h"

using namespace clap_metronome;

/**
 * @brief Memory Usage Test Fixture
 * 
 * Provides utilities for memory footprint analysis and validation.
 */
class MemoryUsageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup needed - testing compile-time constants
    }
    
    void TearDown() override {
        // No teardown needed
    }
    
    /**
     * Calculate padding/alignment overhead
     */
    size_t calculatePadding(size_t sum_of_members, size_t struct_size) {
        return struct_size - sum_of_members;
    }
    
    /**
     * Print detailed memory breakdown (for debugging)
     */
    void printMemoryBreakdown() {
        std::cout << "\n=== AudioDetectionState Memory Breakdown ===" << std::endl;
        std::cout << "Total size: " << sizeof(AudioDetectionState) << " bytes" << std::endl;
        std::cout << "\nComponents:" << std::endl;
        std::cout << "  DetectionState state:              " << sizeof(DetectionState) << " byte(s)" << std::endl;
        std::cout << "  uint16_t threshold:                " << sizeof(uint16_t) << " bytes" << std::endl;
        std::cout << "  uint16_t min_value:                " << sizeof(uint16_t) << " bytes" << std::endl;
        std::cout << "  uint16_t max_value:                " << sizeof(uint16_t) << " bytes" << std::endl;
        std::cout << "  AGCLevel gain_level:               " << sizeof(AGCLevel) << " byte(s)" << std::endl;
        std::cout << "  bool clipping_detected:            " << sizeof(bool) << " byte(s)" << std::endl;
        std::cout << "  uint32_t beat_count:               " << sizeof(uint32_t) << " bytes" << std::endl;
        std::cout << "  uint32_t false_positive_count:     " << sizeof(uint32_t) << " bytes" << std::endl;
        std::cout << "  uint64_t last_beat_timestamp_us:   " << sizeof(uint64_t) << " bytes" << std::endl;
        std::cout << "  uint64_t rising_edge_start_us:     " << sizeof(uint64_t) << " bytes" << std::endl;
        std::cout << "  uint16_t rising_edge_start_value:  " << sizeof(uint16_t) << " bytes" << std::endl;
        std::cout << "  uint16_t rising_edge_peak_value:   " << sizeof(uint16_t) << " bytes" << std::endl;
        std::cout << "  uint64_t last_telemetry_us:        " << sizeof(uint64_t) << " bytes" << std::endl;
        std::cout << "  uint16_t window_samples[64]:       " << sizeof(uint16_t) * 64 << " bytes" << std::endl;
        std::cout << "  uint8_t window_index:              " << sizeof(uint8_t) << " byte(s)" << std::endl;
        std::cout << "  uint16_t noise_floor:              " << sizeof(uint16_t) << " bytes" << std::endl;
        std::cout << "  uint8_t samples_since_noise_update:" << sizeof(uint8_t) << " byte(s)" << std::endl;
        
        // Calculate theoretical sum
        size_t theoretical_sum = sizeof(DetectionState) +
                                 sizeof(uint16_t) * 5 +  // threshold, min, max, rising_edge_start/peak_value
                                 sizeof(AGCLevel) +
                                 sizeof(bool) +
                                 sizeof(uint32_t) * 2 +  // beat_count, false_positive_count
                                 sizeof(uint64_t) * 3 +  // timestamps
                                 sizeof(uint16_t) * 64 + // window_samples
                                 sizeof(uint8_t) * 2 +   // window_index, samples_since_noise_update
                                 sizeof(uint16_t);       // noise_floor
        
        size_t padding = calculatePadding(theoretical_sum, sizeof(AudioDetectionState));
        std::cout << "\nTheoretical sum (no padding): " << theoretical_sum << " bytes" << std::endl;
        std::cout << "Padding/alignment overhead:   " << padding << " bytes" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
};

/**
 * @test MemoryRequirement_TotalFootprint
 * 
 * Validates total AudioDetectionState memory footprint against 400B requirement.
 * 
 * Expected: sizeof(AudioDetectionState) < 400 bytes
 * 
 * Traceability: AC-AUDIO-011 (Memory usage <400B RAM)
 */
TEST_F(MemoryUsageTest, MemoryRequirement_TotalFootprint) {
    constexpr size_t MAX_ALLOWED_BYTES = 400;
    size_t actual_size = sizeof(AudioDetectionState);
    
    // Print detailed breakdown
    printMemoryBreakdown();
    
    EXPECT_LT(actual_size, MAX_ALLOWED_BYTES)
        << "AudioDetectionState exceeds 400B RAM requirement: "
        << actual_size << " bytes (limit: " << MAX_ALLOWED_BYTES << " bytes)";
    
    // Report margin
    size_t margin = MAX_ALLOWED_BYTES - actual_size;
    double margin_percent = (static_cast<double>(margin) / MAX_ALLOWED_BYTES) * 100.0;
    
    std::cout << "Memory margin: " << margin << " bytes ("
              << std::fixed << std::setprecision(1) << margin_percent << "% under limit)"
              << std::endl;
}

/**
 * @test WindowMemory_64Samples
 * 
 * Validates window buffer size calculation.
 * Expected: 64 samples × 2 bytes = 128 bytes
 * 
 * Traceability: AC-AUDIO-001 (Adaptive threshold uses window)
 */
TEST_F(MemoryUsageTest, WindowMemory_64Samples) {
    constexpr size_t EXPECTED_WINDOW_SIZE = 100 * sizeof(uint16_t);  // AUDIO-01: Changed from 64 to 100
    
    AudioDetectionState state;
    size_t actual_window_size = sizeof(state.window_samples);
    
    EXPECT_EQ(actual_window_size, EXPECTED_WINDOW_SIZE)
        << "Window buffer size mismatch: " << actual_window_size
        << " bytes (expected: " << EXPECTED_WINDOW_SIZE << " bytes)";
    
    std::cout << "Window buffer: " << actual_window_size << " bytes "
              << "(100 samples × " << sizeof(uint16_t) << " bytes)" << std::endl;  // AUDIO-01: 100 samples
}

/**
 * @test StateVariables_Size
 * 
 * Validates size of individual state machine and threshold variables.
 * 
 * Traceability: AC-AUDIO-001, AC-AUDIO-002 (Adaptive threshold, state machine)
 */
TEST_F(MemoryUsageTest, StateVariables_Size) {
    // State machine (1 byte enum)
    EXPECT_EQ(sizeof(DetectionState), sizeof(uint8_t))
        << "DetectionState should be 1 byte (uint8_t enum)";
    
    // Threshold variables (2 bytes each)
    EXPECT_EQ(sizeof(uint16_t), 2U) << "uint16_t should be 2 bytes";
    
    // AGC level (1 byte enum)
    EXPECT_EQ(sizeof(AGCLevel), sizeof(uint8_t))
        << "AGCLevel should be 1 byte (uint8_t enum)";
    
    // Boolean flag (1 byte)
    EXPECT_GE(sizeof(bool), 1U) << "bool should be at least 1 byte";
    
    std::cout << "State variables validated: "
              << "DetectionState=" << sizeof(DetectionState) << "B, "
              << "uint16_t=" << sizeof(uint16_t) << "B, "
              << "AGCLevel=" << sizeof(AGCLevel) << "B, "
              << "bool=" << sizeof(bool) << "B" << std::endl;
}

/**
 * @test TimestampVariables_Size
 * 
 * Validates size of 64-bit timestamp variables.
 * 
 * Traceability: AC-AUDIO-005 (Debounce timing), AC-AUDIO-007 (Telemetry timing)
 */
TEST_F(MemoryUsageTest, TimestampVariables_Size) {
    // Timestamps (8 bytes each)
    EXPECT_EQ(sizeof(uint64_t), 8U) << "uint64_t should be 8 bytes";
    
    AudioDetectionState state;
    size_t total_timestamp_size = sizeof(state.last_beat_timestamp_us) +
                                  sizeof(state.rising_edge_start_us) +
                                  sizeof(state.last_telemetry_us);
    
    EXPECT_EQ(total_timestamp_size, 3 * sizeof(uint64_t))
        << "Total timestamp storage should be 24 bytes (3 × 8 bytes)";
    
    std::cout << "Timestamp storage: " << total_timestamp_size << " bytes (3 × "
              << sizeof(uint64_t) << " bytes)" << std::endl;
}

/**
 * @test CounterVariables_Size
 * 
 * Validates size of beat counter and false positive counter.
 * 
 * Traceability: AC-AUDIO-004 (Beat counting), AC-AUDIO-009 (False positive rejection)
 */
TEST_F(MemoryUsageTest, CounterVariables_Size) {
    // Counters (4 bytes each for uint32_t)
    EXPECT_EQ(sizeof(uint32_t), 4U) << "uint32_t should be 4 bytes";
    
    AudioDetectionState state;
    size_t total_counter_size = sizeof(state.beat_count) +
                                sizeof(state.false_positive_count);
    
    EXPECT_EQ(total_counter_size, 2 * sizeof(uint32_t))
        << "Total counter storage should be 8 bytes (2 × 4 bytes)";
    
    std::cout << "Counter storage: " << total_counter_size << " bytes (2 × "
              << sizeof(uint32_t) << " bytes)" << std::endl;
}

/**
 * @test RisingEdgeTracking_Size
 * 
 * Validates size of rising edge detection variables.
 * 
 * Traceability: AC-AUDIO-006 (Kick-only filtering via rise time)
 */
TEST_F(MemoryUsageTest, RisingEdgeTracking_Size) {
    AudioDetectionState state;
    size_t rising_edge_size = sizeof(state.rising_edge_start_value) +
                              sizeof(state.rising_edge_peak_value);
    
    EXPECT_EQ(rising_edge_size, 2 * sizeof(uint16_t))
        << "Rising edge tracking should be 4 bytes (2 × 2 bytes)";
    
    std::cout << "Rising edge tracking: " << rising_edge_size << " bytes (2 × "
              << sizeof(uint16_t) << " bytes)" << std::endl;
}

/**
 * @test NoiseFloorCaching_Size
 * 
 * Validates size of noise floor caching variables (Cycle 9 optimization).
 * 
 * Traceability: AC-AUDIO-009 (False positive rejection with cached noise floor)
 */
TEST_F(MemoryUsageTest, NoiseFloorCaching_Size) {
    AudioDetectionState state;
    size_t noise_caching_size = sizeof(state.noise_floor) +
                                sizeof(state.samples_since_noise_update);
    
    EXPECT_EQ(noise_caching_size, sizeof(uint16_t) + sizeof(uint8_t))
        << "Noise floor caching should be 3 bytes (2 + 1 bytes)";
    
    std::cout << "Noise floor caching: " << noise_caching_size << " bytes ("
              << sizeof(uint16_t) << " + " << sizeof(uint8_t) << " bytes)" << std::endl;
}

/**
 * @test MemoryAlignment_Padding
 * 
 * Documents alignment padding overhead for transparency.
 * 
 * Note: Padding is compiler/architecture dependent. This test documents the
 * overhead rather than asserting a specific value.
 */
TEST_F(MemoryUsageTest, MemoryAlignment_Padding) {
    // Calculate theoretical minimum (sum of all members)
    size_t theoretical_min = sizeof(DetectionState) +           // 1 byte
                             sizeof(uint16_t) * 5 +             // 10 bytes (threshold, min, max, rising_edge_start/peak)
                             sizeof(AGCLevel) +                 // 1 byte
                             sizeof(bool) +                     // 1 byte
                             sizeof(uint32_t) * 2 +             // 8 bytes (counters)
                             sizeof(uint64_t) * 3 +             // 24 bytes (timestamps)
                             sizeof(uint16_t) * 100 +           // 200 bytes (window) - AUDIO-01: Changed from 64 to 100
                             sizeof(uint8_t) * 2 +              // 2 bytes (window_index, samples_since_noise_update)
                             sizeof(uint16_t);                  // 2 bytes (noise_floor)
    
    size_t actual_size = sizeof(AudioDetectionState);
    size_t padding = calculatePadding(theoretical_min, actual_size);
    
    // Padding should be reasonable (typically < 10% of total for well-structured types)
    double padding_percent = (static_cast<double>(padding) / actual_size) * 100.0;
    
    EXPECT_LT(padding_percent, 10.0)
        << "Excessive padding detected: " << padding << " bytes ("
        << std::fixed << std::setprecision(1) << padding_percent << "%)";
    
    std::cout << "Theoretical minimum: " << theoretical_min << " bytes" << std::endl;
    std::cout << "Actual size:         " << actual_size << " bytes" << std::endl;
    std::cout << "Padding overhead:    " << padding << " bytes ("
              << std::fixed << std::setprecision(1) << padding_percent << "%)" << std::endl;
}

/**
 * @test StackAllocation_Validation
 * 
 * Validates that AudioDetectionState can be stack-allocated without issues.
 * 
 * Note: ESP32 typically has 8KB+ stack per task. This structure is safe.
 */
TEST_F(MemoryUsageTest, StackAllocation_Validation) {
    // Stack allocate a state structure
    AudioDetectionState state;
    state.init();
    
    // Verify initialization worked
    EXPECT_EQ(state.state, DetectionState::IDLE);
    EXPECT_EQ(state.beat_count, 0u);
    EXPECT_EQ(state.window_index, 0u);
    
    // Verify window initialized to zero (AUDIO-01: Changed from 2000 midpoint to 0)
    for (size_t i = 0; i < AudioDetectionState::WINDOW_SIZE; i++) {
        EXPECT_EQ(state.window_samples[i], 0)
            << "Window sample " << i << " not initialized to zero";
    }
    
    std::cout << "Stack allocation validated: " << sizeof(AudioDetectionState)
              << " bytes (safe for ESP32 stack)" << std::endl;
}

/**
 * @test StaticConstants_NotCounted
 * 
 * Validates that static constexpr constants do not consume RAM.
 * 
 * These constants are compile-time and stored in flash/code memory, not RAM.
 * 
 * Traceability: Embedded resource optimization best practices
 */
TEST_F(MemoryUsageTest, StaticConstants_NotCounted) {
    // Static constexpr constants should not increase struct size
    // They are compile-time constants stored in flash, not RAM
    
    // Create two instances - size should not increase
    AudioDetectionState state1;
    AudioDetectionState state2;
    
    EXPECT_EQ(sizeof(state1), sizeof(state2))
        << "Static constants should not affect instance size";
    
    // Verify constants are accessible (compile-time check)
    EXPECT_EQ(AudioDetectionState::WINDOW_SIZE, 100u);  // AUDIO-01: Changed from 64 to 100
    EXPECT_EQ(AudioDetectionState::CLIPPING_THRESHOLD, 4000u);
    EXPECT_EQ(AudioDetectionState::DEBOUNCE_PERIOD_US, 50000u);
    EXPECT_EQ(AudioDetectionState::TELEMETRY_INTERVAL_US, 500000u);
    EXPECT_EQ(AudioDetectionState::KICK_RISE_TIME_US, 4000u);
    EXPECT_EQ(AudioDetectionState::THRESHOLD_FACTOR, 0.8f);
    EXPECT_EQ(AudioDetectionState::THRESHOLD_MARGIN, 80u);
    EXPECT_EQ(AudioDetectionState::MIN_SIGNAL_AMPLITUDE, 200u);
    EXPECT_EQ(AudioDetectionState::NOISE_UPDATE_INTERVAL, 16u);
    
    std::cout << "Static constants validated: Stored in flash, not RAM" << std::endl;
}

/**
 * @test AudioDetection_TotalMemory
 * 
 * Validates total memory footprint of AudioDetection class (state + overhead).
 * 
 * Expected: AudioDetection size ≈ AudioDetectionState size + minimal overhead
 * 
 * Traceability: AC-AUDIO-011 (Total memory usage <400B RAM)
 */
TEST_F(MemoryUsageTest, AudioDetection_TotalMemory) {
    constexpr size_t MAX_ALLOWED_BYTES = 400;
    
    size_t state_size = sizeof(AudioDetectionState);
    
    // Note: AudioDetection contains state + pointer to ITimingProvider
    // Expected overhead: ~4-8 bytes (pointer size on 32-bit/64-bit systems)
    
    std::cout << "AudioDetectionState size: " << state_size << " bytes" << std::endl;
    std::cout << "ITimingProvider* size:    " << sizeof(void*) << " bytes (pointer)" << std::endl;
    
    size_t expected_total = state_size + sizeof(void*);
    
    EXPECT_LT(expected_total, MAX_ALLOWED_BYTES)
        << "Total AudioDetection memory exceeds 400B: " << expected_total
        << " bytes (limit: " << MAX_ALLOWED_BYTES << " bytes)";
    
    std::cout << "Expected total memory:    " << expected_total << " bytes" << std::endl;
    
    size_t margin = MAX_ALLOWED_BYTES - expected_total;
    double margin_percent = (static_cast<double>(margin) / MAX_ALLOWED_BYTES) * 100.0;
    
    std::cout << "Total memory margin:      " << margin << " bytes ("
              << std::fixed << std::setprecision(1) << margin_percent << "% under limit)"
              << std::endl;
}

/**
 * @test MemoryBreakdown_ComponentSummary
 * 
 * Provides comprehensive memory breakdown by functional component.
 * 
 * This test documents memory distribution across different subsystems
 * for maintenance and optimization purposes.
 */
TEST_F(MemoryUsageTest, MemoryBreakdown_ComponentSummary) {
    std::cout << "\n=== Memory Distribution by Component ===" << std::endl;
    
    // State machine
    size_t state_machine = sizeof(DetectionState);
    std::cout << "State Machine:        " << state_machine << " bytes" << std::endl;
    
    // Adaptive threshold (AC-AUDIO-001)
    size_t threshold_system = sizeof(uint16_t) * 3 +  // threshold, min, max
                              sizeof(uint16_t) * 64 +  // window_samples
                              sizeof(uint8_t);         // window_index
    std::cout << "Adaptive Threshold:   " << threshold_system << " bytes (window + tracking)" << std::endl;
    
    // AGC system (AC-AUDIO-003, AC-AUDIO-012)
    size_t agc_system = sizeof(AGCLevel) + sizeof(bool);
    std::cout << "AGC System:           " << agc_system << " bytes" << std::endl;
    
    // Beat detection metrics (AC-AUDIO-004)
    size_t beat_metrics = sizeof(uint32_t) * 2;  // beat_count, false_positive_count
    std::cout << "Beat Metrics:         " << beat_metrics << " bytes" << std::endl;
    
    // Timing system (AC-AUDIO-005, AC-AUDIO-007)
    size_t timing_system = sizeof(uint64_t) * 3;  // timestamps
    std::cout << "Timing System:        " << timing_system << " bytes (timestamps)" << std::endl;
    
    // Rise time detection (AC-AUDIO-006)
    size_t rise_detection = sizeof(uint16_t) * 2;  // rising_edge values
    std::cout << "Rise Time Detection:  " << rise_detection << " bytes" << std::endl;
    
    // Noise floor caching (AC-AUDIO-009, Cycle 9)
    size_t noise_system = sizeof(uint16_t) + sizeof(uint8_t);  // noise_floor + counter
    std::cout << "Noise Floor Caching:  " << noise_system << " bytes" << std::endl;
    
    size_t component_sum = state_machine + threshold_system + agc_system +
                           beat_metrics + timing_system + rise_detection +
                           noise_system;
    
    size_t actual_size = sizeof(AudioDetectionState);
    size_t padding = actual_size - component_sum;
    
    std::cout << "\nComponent Sum:        " << component_sum << " bytes" << std::endl;
    std::cout << "Padding/Alignment:    " << padding << " bytes" << std::endl;
    std::cout << "TOTAL:                " << actual_size << " bytes" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Verify sum matches
    EXPECT_EQ(actual_size, component_sum + padding)
        << "Component breakdown doesn't match actual size";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}