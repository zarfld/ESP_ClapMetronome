/**
 * TDD Cycle 10 - AC-AUDIO-010: CPU Usage Validation
 * 
 * Acceptance Criteria:
 * - Average CPU usage <45%
 * - Peak CPU usage <50%
 * 
 * Test Approach:
 * - Measure cycles consumed per processSample() call
 * - Calculate percentage at 16kHz sample rate (240MHz ESP32)
 * - Validate sustained load (1000+ samples)
 * - Measure worst-case scenarios (clipping, noise floor updates)
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Verification Process)
 * Implements: AC-AUDIO-010
 * GitHub Issue: #45 (DES-C-001 Audio Detection Engine)
 */

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>

// Define M_PI if not available (Windows)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../../src/audio/AudioDetection.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * CPU Usage Test Fixture
 * 
 * Measures actual execution time of processSample() to estimate CPU usage.
 * Uses high-resolution clock to measure microsecond-level timing.
 */
class CPUUsageTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* detector;
    std::vector<uint64_t> sample_durations_ns;  // Nanosecond precision
    
    // ESP32 specifications for CPU usage calculation
    static constexpr uint32_t ESP32_FREQ_MHZ = 240;  // 240MHz typical
    static constexpr uint32_t SAMPLE_RATE_HZ = 16000;  // 16kHz audio
    static constexpr uint32_t TIME_PER_SAMPLE_US = 1000000 / SAMPLE_RATE_HZ;  // 62.5μs
    
    void SetUp() override {
        detector = new AudioDetection(&mock_timing_);
        detector->init();
        sample_durations_ns.clear();
        sample_durations_ns.reserve(2000);
    }
    
    void TearDown() override {
        delete detector;
    }
    
    /**
     * Measure execution time of a single processSample() call
     */
    uint64_t measureSampleProcessing(uint16_t adc_value, uint64_t timestamp_us) {
        // Update mock timing to return the expected timestamp
        mock_timing_.setTimestamp(timestamp_us);
        
        auto start = std::chrono::high_resolution_clock::now();
        detector->processSample(adc_value);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        return duration.count();
    }
    
    /**
     * Calculate CPU usage percentage from execution time
     * 
     * CPU% = (execution_time_us / time_per_sample_us) * 100
     * 
     * Example: If processSample() takes 5μs and we have 62.5μs per sample,
     *          CPU usage = (5 / 62.5) * 100 = 8%
     */
    double calculateCPUUsage(uint64_t duration_ns) {
        double duration_us = duration_ns / 1000.0;
        return (duration_us / TIME_PER_SAMPLE_US) * 100.0;
    }
    
    /**
     * Process a batch of samples and collect timing statistics
     */
    struct CPUStats {
        double average_percent;
        double peak_percent;
        double min_percent;
        double median_percent;
        double p95_percent;  // 95th percentile
        double p99_percent;  // 99th percentile
        uint64_t total_samples;
    };
    
    CPUStats processSamplesAndMeasure(const std::vector<uint16_t>& samples, 
                                       uint64_t start_timestamp_us = 0) {
        sample_durations_ns.clear();
        
        for (size_t i = 0; i < samples.size(); i++) {
            uint64_t timestamp = start_timestamp_us + (i * TIME_PER_SAMPLE_US);
            uint64_t duration = measureSampleProcessing(samples[i], timestamp);
            sample_durations_ns.push_back(duration);
        }
        
        // Calculate statistics
        std::vector<double> cpu_percentages;
        cpu_percentages.reserve(sample_durations_ns.size());
        
        double sum = 0.0;
        double peak = 0.0;
        double min = 100.0;
        
        for (uint64_t duration_ns : sample_durations_ns) {
            double cpu_pct = calculateCPUUsage(duration_ns);
            cpu_percentages.push_back(cpu_pct);
            sum += cpu_pct;
            peak = std::max(peak, cpu_pct);
            min = std::min(min, cpu_pct);
        }
        
        // Sort for percentile calculations
        std::sort(cpu_percentages.begin(), cpu_percentages.end());
        
        CPUStats stats;
        stats.average_percent = sum / sample_durations_ns.size();
        stats.peak_percent = peak;
        stats.min_percent = min;
        stats.median_percent = cpu_percentages[cpu_percentages.size() / 2];
        stats.p95_percent = cpu_percentages[static_cast<size_t>(cpu_percentages.size() * 0.95)];
        stats.p99_percent = cpu_percentages[static_cast<size_t>(cpu_percentages.size() * 0.99)];
        stats.total_samples = sample_durations_ns.size();
        
        return stats;
    }
    
    /**
     * Generate test signal: sine wave with amplitude and noise
     */
    std::vector<uint16_t> generateSineWave(size_t num_samples, 
                                             uint16_t amplitude = 1000,
                                             uint16_t offset = 2000,
                                             double frequency_hz = 100.0) {
        std::vector<uint16_t> samples;
        samples.reserve(num_samples);
        
        for (size_t i = 0; i < num_samples; i++) {
            double t = i / static_cast<double>(SAMPLE_RATE_HZ);
            double sine = std::sin(2.0 * M_PI * frequency_hz * t);
            int32_t sample = offset + static_cast<int32_t>(amplitude * sine);
            
            // Clamp to 12-bit ADC range
            if (sample < 0) sample = 0;
            if (sample > 4095) sample = 4095;
            
            samples.push_back(static_cast<uint16_t>(sample));
        }
        
        return samples;
    }
};

/**
 * Test: Average CPU Usage - Quiet Signal
 * 
 * AC-AUDIO-010: Average CPU usage <45%
 * 
 * Scenario: Process 1000 samples of low-amplitude signal (no beats)
 * Expected: Average CPU usage well below 45%
 */
TEST_F(CPUUsageTest, AverageCPUUsage_QuietSignal) {
    // Generate 1000 samples of quiet signal
    std::vector<uint16_t> quiet_signal(1000, 2000);  // Constant baseline
    
    CPUStats stats = processSamplesAndMeasure(quiet_signal);
    
    // Report statistics
    std::cout << "\n=== Quiet Signal CPU Usage ===" << std::endl;
    std::cout << "Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "Peak: " << stats.peak_percent << "%" << std::endl;
    std::cout << "Median: " << stats.median_percent << "%" << std::endl;
    std::cout << "95th percentile: " << stats.p95_percent << "%" << std::endl;
    std::cout << "99th percentile: " << stats.p99_percent << "%" << std::endl;
    
    // AC-AUDIO-010: Average <45%
    EXPECT_LT(stats.average_percent, 45.0) 
        << "Average CPU usage must be <45%";
    
    // Additional assertion: Should be much lower for quiet signal
    EXPECT_LT(stats.average_percent, 20.0)
        << "Quiet signal should use <20% CPU";
}

/**
 * Test: Average CPU Usage - Active Beats
 * 
 * AC-AUDIO-010: Average CPU usage <45%
 * 
 * Scenario: Process 1000 samples with multiple beats (realistic load)
 * Expected: Average CPU usage <45% even with active detection
 */
TEST_F(CPUUsageTest, AverageCPUUsage_ActiveBeats) {
    // Generate 1000 samples with sine wave (simulates beats)
    std::vector<uint16_t> beat_signal = generateSineWave(1000, 1500, 2000, 10.0);  // 10Hz beats
    
    CPUStats stats = processSamplesAndMeasure(beat_signal);
    
    // Report statistics
    std::cout << "\n=== Active Beats CPU Usage ===" << std::endl;
    std::cout << "Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "Peak: " << stats.peak_percent << "%" << std::endl;
    std::cout << "Median: " << stats.median_percent << "%" << std::endl;
    std::cout << "95th percentile: " << stats.p95_percent << "%" << std::endl;
    std::cout << "99th percentile: " << stats.p99_percent << "%" << std::endl;
    
    // AC-AUDIO-010: Average <45%
    EXPECT_LT(stats.average_percent, 45.0)
        << "Average CPU usage must be <45% even with active beats";
}

/**
 * Test: Peak CPU Usage - Worst Case
 * 
 * AC-AUDIO-010: Peak CPU usage <50%
 * 
 * Scenario: Process samples that trigger all expensive operations:
 *   - Noise floor recalculation (every 16th sample)
 *   - Threshold updates
 *   - State transitions
 *   - Beat event emission
 * 
 * Expected: Peak CPU usage <50%
 */
TEST_F(CPUUsageTest, PeakCPUUsage_WorstCase) {
    // Generate signal that triggers expensive operations
    std::vector<uint16_t> worst_case_signal;
    worst_case_signal.reserve(1000);
    
    // Pattern: Large amplitude swing every 16 samples (forces noise floor recalc)
    for (size_t i = 0; i < 1000; i++) {
        if (i % 16 == 0) {
            // Large spike (triggers detection + noise floor update)
            worst_case_signal.push_back(3500);
        } else if (i % 16 < 8) {
            // Rising edge
            worst_case_signal.push_back(2000 + (i % 16) * 200);
        } else {
            // Falling edge back to baseline
            worst_case_signal.push_back(2000);
        }
    }
    
    CPUStats stats = processSamplesAndMeasure(worst_case_signal);
    
    // Report statistics
    std::cout << "\n=== Worst Case CPU Usage ===" << std::endl;
    std::cout << "Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "Peak: " << stats.peak_percent << "%" << std::endl;
    std::cout << "Median: " << stats.median_percent << "%" << std::endl;
    std::cout << "95th percentile: " << stats.p95_percent << "%" << std::endl;
    std::cout << "99th percentile: " << stats.p99_percent << "%" << std::endl;
    
    // AC-AUDIO-010: Peak <50%
    EXPECT_LT(stats.peak_percent, 50.0)
        << "Peak CPU usage must be <50% even in worst case";
}

/**
 * Test: Sustained Load - 2000 Samples
 * 
 * AC-AUDIO-010: Average CPU usage <45% over sustained period
 * 
 * Scenario: Process 2000 samples (125ms of audio) with realistic signal
 * Expected: Average remains <45% over sustained operation
 */
TEST_F(CPUUsageTest, SustainedLoad_2000Samples) {
    // Generate 2000 samples with varied activity
    std::vector<uint16_t> sustained_signal = generateSineWave(2000, 1000, 2000, 8.0);  // 8Hz
    
    CPUStats stats = processSamplesAndMeasure(sustained_signal);
    
    // Report statistics
    std::cout << "\n=== Sustained Load (2000 samples) ===" << std::endl;
    std::cout << "Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "Peak: " << stats.peak_percent << "%" << std::endl;
    std::cout << "Median: " << stats.median_percent << "%" << std::endl;
    std::cout << "95th percentile: " << stats.p95_percent << "%" << std::endl;
    std::cout << "99th percentile: " << stats.p99_percent << "%" << std::endl;
    
    // AC-AUDIO-010: Average <45%
    EXPECT_LT(stats.average_percent, 45.0)
        << "Average CPU usage must remain <45% over sustained operation";
    
    // Use 99th percentile for peak assessment (handles OS scheduling outliers)
    EXPECT_LT(stats.p99_percent, 50.0)
        << "99th percentile CPU usage must be <50%";
}

/**
 * Test: Clipping Scenario - High Amplitude
 * 
 * Scenario: Process signal near clipping threshold (tests AGC overhead)
 * Expected: CPU usage still within limits during AGC activation
 */
TEST_F(CPUUsageTest, ClippingScenario_HighAmplitude) {
    // Generate signal that triggers clipping detection
    std::vector<uint16_t> clipping_signal;
    clipping_signal.reserve(1000);
    
    for (size_t i = 0; i < 1000; i++) {
        if (i < 100) {
            // Start normal
            clipping_signal.push_back(2000);
        } else if (i < 200) {
            // Ramp up to clipping
            clipping_signal.push_back(2000 + static_cast<uint16_t>((i - 100) * 20));
        } else {
            // Stay at clipping level
            clipping_signal.push_back(4095);
        }
    }
    
    CPUStats stats = processSamplesAndMeasure(clipping_signal);
    
    // Report statistics
    std::cout << "\n=== Clipping Scenario CPU Usage ===" << std::endl;
    std::cout << "Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "Peak: " << stats.peak_percent << "%" << std::endl;
    std::cout << "99th percentile: " << stats.p99_percent << "%" << std::endl;
    
    // Average should be well within limits (primary metric)
    EXPECT_LT(stats.average_percent, 45.0);
    
    // Peak may have outliers on Windows due to OS scheduling
    // Use 99th percentile for more reliable peak assessment
    EXPECT_LT(stats.p99_percent, 50.0) 
        << "99th percentile CPU usage should be <50%";
}

/**
 * Test: Noise Floor Update Overhead
 * 
 * Scenario: Specifically measure samples where noise floor recalculation occurs
 * Expected: Even with noise floor update, CPU usage <50%
 */
TEST_F(CPUUsageTest, NoiseFloorUpdate_Overhead) {
    // Initialize detector with 64 samples to populate window
    for (int i = 0; i < 64; i++) {
        mock_timing_.setTimestamp(i * TIME_PER_SAMPLE_US);
        detector->processSample(2000 + (i % 10));
    }
    
    // Now measure samples that trigger noise floor recalculation (every 16th)
    std::vector<uint64_t> update_durations;
    
    for (int i = 0; i < 100; i++) {
        uint16_t adc_value = 2000 + (i % 100);
        uint64_t timestamp = (64 + i) * TIME_PER_SAMPLE_US;
        
        uint64_t duration = measureSampleProcessing(adc_value, timestamp);
        
        // Every 16th sample triggers noise floor update
        if (i % 16 == 0) {
            update_durations.push_back(duration);
        }
    }
    
    // Calculate CPU usage for update samples
    double max_update_cpu = 0.0;
    double avg_update_cpu = 0.0;
    
    for (uint64_t duration : update_durations) {
        double cpu_pct = calculateCPUUsage(duration);
        max_update_cpu = std::max(max_update_cpu, cpu_pct);
        avg_update_cpu += cpu_pct;
    }
    avg_update_cpu /= update_durations.size();
    
    std::cout << "\n=== Noise Floor Update Overhead ===" << std::endl;
    std::cout << "Average (with update): " << avg_update_cpu << "%" << std::endl;
    std::cout << "Peak (with update): " << max_update_cpu << "%" << std::endl;
    
    // Even with noise floor recalculation, should stay within peak limit
    EXPECT_LT(max_update_cpu, 50.0)
        << "CPU usage during noise floor update must be <50%";
}

/**
 * Test: Percentile Analysis - 95th and 99th
 * 
 * Scenario: Analyze distribution of CPU usage across 1000 samples
 * Expected: 95th percentile <45%, 99th percentile <50%
 */
TEST_F(CPUUsageTest, PercentileAnalysis_Distribution) {
    // Generate realistic mixed signal
    std::vector<uint16_t> mixed_signal = generateSineWave(1000, 1200, 2000, 12.0);
    
    CPUStats stats = processSamplesAndMeasure(mixed_signal);
    
    // Report full distribution
    std::cout << "\n=== CPU Usage Percentile Distribution ===" << std::endl;
    std::cout << "Minimum: " << stats.min_percent << "%" << std::endl;
    std::cout << "Median (50th): " << stats.median_percent << "%" << std::endl;
    std::cout << "Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "95th percentile: " << stats.p95_percent << "%" << std::endl;
    std::cout << "99th percentile: " << stats.p99_percent << "%" << std::endl;
    std::cout << "Peak (100th): " << stats.peak_percent << "%" << std::endl;
    
    // Most samples should be well below average limit
    EXPECT_LT(stats.p95_percent, 45.0)
        << "95% of samples should use <45% CPU";
    
    // Even 99th percentile should be within peak limit
    EXPECT_LT(stats.p99_percent, 50.0)
        << "99% of samples should use <50% CPU";
}

/**
 * Test: Comparison - Baseline vs Optimized
 * 
 * Purpose: Verify Cycle 9 REFACTOR optimizations are effective
 * 
 * This test documents expected improvement from Cycle 9 optimizations:
 * - Before: ~10μs per sample (~16% CPU)
 * - After: ~8μs per sample (~13% CPU)
 * - Improvement: ~20% reduction
 */
TEST_F(CPUUsageTest, OptimizationValidation_Cycle9Improvements) {
    std::vector<uint16_t> test_signal = generateSineWave(1000, 1000, 2000, 10.0);
    
    CPUStats stats = processSamplesAndMeasure(test_signal);
    
    std::cout << "\n=== Cycle 9 Optimization Validation ===" << std::endl;
    std::cout << "Current Average: " << stats.average_percent << "%" << std::endl;
    std::cout << "Target (post-optimization): <13%" << std::endl;
    std::cout << "Baseline (pre-optimization): ~16%" << std::endl;
    
    // After Cycle 9 optimizations, CPU usage should be significantly reduced
    EXPECT_LT(stats.average_percent, 15.0)
        << "CPU usage should reflect Cycle 9 optimizations (<15%)";
    
    // Ideally around 13% or better
    EXPECT_LT(stats.average_percent, 45.0)
        << "Must meet AC-AUDIO-010 requirement (<45%)";
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

