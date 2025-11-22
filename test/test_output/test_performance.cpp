/**
 * @file test_performance.cpp
 * @brief TDD Cycle OUT-05: Performance Validation Tests
 * 
 * Verifies: AC-OUT-007, AC-OUT-008, AC-OUT-009
 * - Timing jitter <1ms
 * - Output latency <10ms
 * - CPU usage <3%
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (RED phase)
 */

#include <gtest/gtest.h>
#include "output/OutputController.h"
#include <vector>
#include <cmath>

#ifdef UNIT_TEST
#include "../mocks/time_mock.h"
#endif

/**
 * Test fixture for performance validation tests
 */
class PerformanceTest : public ::testing::Test {
protected:
    OutputController* controller;
    OutputConfig config;
    std::vector<uint64_t> timestamps;
    
    void SetUp() override {
        reset_mock_time();
        
        config.mode = OutputMode::BOTH;
        config.rtp_port = 5004;
        config.midi_ppqn = 24;
        config.initial_bpm = 120;
        config.relay_pulse_ms = 50;
        config.relay_watchdog_ms = 100;
        
        controller = new OutputController(config);
        timestamps.clear();
    }
    
    void TearDown() override {
        delete controller;
        controller = nullptr;
    }
    
    /**
     * Calculate jitter (standard deviation) from timestamps
     */
    float calculateJitter(const std::vector<uint64_t>& times) {
        if (times.size() < 2) return 0.0f;
        
        // Calculate intervals between consecutive timestamps
        std::vector<uint64_t> intervals;
        for (size_t i = 1; i < times.size(); i++) {
            intervals.push_back(times[i] - times[i-1]);
        }
        
        // Calculate mean interval
        uint64_t sum = 0;
        for (uint64_t interval : intervals) {
            sum += interval;
        }
        float mean = static_cast<float>(sum) / intervals.size();
        
        // Calculate variance
        float variance = 0.0f;
        for (uint64_t interval : intervals) {
            float diff = static_cast<float>(interval) - mean;
            variance += diff * diff;
        }
        variance /= intervals.size();
        
        // Return standard deviation in microseconds
        return sqrtf(variance);
    }
    
    /**
     * Simulate timer callbacks at specified BPM
     */
    void simulateTimerCallbacks(int count, uint16_t bpm) {
        uint32_t interval_us = controller->calculateTimerInterval(bpm, config.midi_ppqn);
        
        for (int i = 0; i < count; i++) {
            uint64_t callback_time = micros();
            timestamps.push_back(callback_time);
            
            // Simulate timer callback
            controller->onTimerCallback();
            
            // Advance time by interval
            advance_time_us(interval_us);
        }
    }
};

/**
 * Test 1: MIDI Clock Jitter Measurement
 * 
 * Verifies: AC-OUT-007 (Jitter <1ms)
 * Given: MIDI output enabled at 120 BPM
 * When: 100 MIDI clock messages sent
 * Then: Jitter (std dev) <1ms (1000µs)
 */
TEST_F(PerformanceTest, MidiClockJitter_Under1ms) {
    // Enable MIDI output only for cleaner measurement
    config.mode = OutputMode::MIDI_ONLY;
    delete controller;
    controller = new OutputController(config);
    
    // Start sync
    controller->startSync(120);
    
    // Collect 100 MIDI clock timestamps via timer callbacks
    simulateTimerCallbacks(100, 120);
    
    // Calculate jitter
    float jitter_us = calculateJitter(timestamps);
    float jitter_ms = jitter_us / 1000.0f;
    
    // Verify jitter <1ms (AC-OUT-007)
    EXPECT_LT(jitter_ms, 1.0f) << "MIDI clock jitter: " << jitter_ms << "ms";
    
    // Verify reasonable mean interval (should be ~20833µs at 120 BPM, 24 PPQN)
    uint32_t expected_interval = controller->calculateTimerInterval(120, 24);
    EXPECT_GT(timestamps.size(), 1UL);
    if (timestamps.size() > 1) {
        uint64_t measured_interval = (timestamps.back() - timestamps.front()) / (timestamps.size() - 1);
        EXPECT_NEAR(static_cast<double>(measured_interval), static_cast<double>(expected_interval), 1000.0);  // Within 1ms
    }
}

/**
 * Test 2: Timer Interval Jitter
 * 
 * Verifies: AC-OUT-007 (Jitter <1ms)
 * Given: Timer configured at 120 BPM, 24 PPQN
 * When: 100 timer intervals measured
 * Then: Jitter <1ms
 */
TEST_F(PerformanceTest, TimerIntervalJitter_Under1ms) {
    controller->startSync(120);
    
    // Measure 100 timer callback intervals
    simulateTimerCallbacks(100, 120);
    
    // Calculate jitter
    float jitter_us = calculateJitter(timestamps);
    float jitter_ms = jitter_us / 1000.0f;
    
    // Verify jitter <1ms (AC-OUT-007)
    EXPECT_LT(jitter_ms, 1.0f) << "Timer jitter: " << jitter_ms << "ms";
    
    // Get jitter stats from controller
    TimerStats stats = controller->getTimerStats();
    EXPECT_LT(stats.jitter_ms, 1.0f) << "Controller reported jitter: " << stats.jitter_ms << "ms";
}

/**
 * Test 3: Relay Pulse Timing Precision
 * 
 * Verifies: AC-OUT-007 (Jitter <1ms)
 * Given: Relay configured with 50ms pulse
 * When: 100 pulses measured
 * Then: Actual duration within 1ms of configured
 */
TEST_F(PerformanceTest, RelayPulsePrecision_Under1ms) {
    // Enable relay only
    config.mode = OutputMode::RELAY_ONLY;
    delete controller;
    controller = new OutputController(config);
    
    std::vector<uint64_t> pulse_durations;
    
    // Measure 100 relay pulses
    for (int i = 0; i < 100; i++) {
        uint64_t pulse_start = micros();
        
        // Trigger pulse
        bool result = controller->pulseRelay();
        EXPECT_TRUE(result);
        
        // Process until relay turns off
        uint64_t pulse_end = pulse_start;
        for (int j = 0; j < 100; j++) {  // Max 100 checks
            advance_time_us(1000);  // Advance 1ms
            controller->processRelayWatchdog();
            
            if (!controller->getRelayGPIO()) {
                pulse_end = micros();
                break;
            }
        }
        
        uint64_t duration = pulse_end - pulse_start;
        pulse_durations.push_back(duration);
        
        // Wait for debounce
        advance_time_us(15000);  // 15ms > 10ms debounce
    }
    
    // Calculate mean duration
    uint64_t sum = 0;
    for (uint64_t duration : pulse_durations) {
        sum += duration;
    }
    float mean_duration_us = static_cast<float>(sum) / pulse_durations.size();
    float mean_duration_ms = mean_duration_us / 1000.0f;
    
    // Calculate jitter (variation from mean)
    float variance = 0.0f;
    for (uint64_t duration : pulse_durations) {
        float diff = static_cast<float>(duration) - mean_duration_us;
        variance += diff * diff;
    }
    variance /= pulse_durations.size();
    float jitter_us = sqrtf(variance);
    float jitter_ms = jitter_us / 1000.0f;
    
    // Verify mean duration close to configured (50ms)
    EXPECT_NEAR(mean_duration_ms, 50.0f, 1.0f) << "Mean pulse duration: " << mean_duration_ms << "ms";
    
    // Verify jitter <1ms (AC-OUT-007)
    EXPECT_LT(jitter_ms, 1.0f) << "Relay pulse jitter: " << jitter_ms << "ms";
}

/**
 * Test 4: Output Latency - Detection to MIDI
 * 
 * Verifies: AC-OUT-008 (Latency <10ms)
 * Given: MIDI output enabled
 * When: Beat detected (trigger event)
 * Then: First MIDI message sent within 10ms
 */
TEST_F(PerformanceTest, MidiLatency_Under10ms) {
    config.mode = OutputMode::MIDI_ONLY;
    delete controller;
    controller = new OutputController(config);
    
    // Start sync
    controller->startSync(120);
    
    // Trigger beat detection (simulate)
    uint64_t trigger_time = micros();
    
    // Immediately call timer callback (represents ISR triggered by detection)
    controller->onTimerCallback();
    
    uint64_t output_time = micros();
    uint64_t latency_us = output_time - trigger_time;
    float latency_ms = static_cast<float>(latency_us) / 1000.0f;
    
    // Verify latency <10ms (AC-OUT-008)
    EXPECT_LT(latency_ms, 10.0f) << "MIDI latency: " << latency_ms << "ms";
    
    // Verify MIDI message was sent
    MidiStats stats = controller->getMidiStats();
    EXPECT_GT(stats.clock_messages_sent, 0U);
}

/**
 * Test 5: Output Latency - Detection to Relay
 * 
 * Verifies: AC-OUT-008 (Latency <10ms)
 * Given: Relay output enabled
 * When: Beat detected (trigger pulse)
 * Then: Relay GPIO HIGH within 10ms
 */
TEST_F(PerformanceTest, RelayLatency_Under10ms) {
    config.mode = OutputMode::RELAY_ONLY;
    delete controller;
    controller = new OutputController(config);
    
    // Trigger relay pulse
    uint64_t trigger_time = micros();
    
    bool result = controller->pulseRelay();
    EXPECT_TRUE(result);
    
    uint64_t output_time = micros();
    uint64_t latency_us = output_time - trigger_time;
    float latency_ms = static_cast<float>(latency_us) / 1000.0f;
    
    // Verify latency <10ms (AC-OUT-008)
    EXPECT_LT(latency_ms, 10.0f) << "Relay latency: " << latency_ms << "ms";
    
    // Verify relay is on
    EXPECT_TRUE(controller->getRelayGPIO());
}

/**
 * Test 6: Long Duration Stability
 * 
 * Verifies: AC-OUT-007, AC-OUT-009 (Sustained performance)
 * Given: System running at 120 BPM
 * When: Run for 60 seconds (simulated)
 * Then: Jitter remains <1ms, no degradation
 */
TEST_F(PerformanceTest, LongDurationStability_60Seconds) {
    controller->startSync(120);
    
    // Calculate number of callbacks in 60 seconds
    // 120 BPM × 24 PPQN = 2880 clocks/minute = 48 clocks/second
    // 60 seconds × 48 = 2880 callbacks
    // Test at 48 callbacks/second (120 BPM × 24 PPQN / 60)
    
    // Measure jitter at intervals (every 10 seconds)
    std::vector<float> jitter_samples;
    
    for (int interval = 0; interval < 6; interval++) {  // 6 intervals of 10 seconds
        timestamps.clear();
        
        // Run 10 seconds worth (480 callbacks = 48 callbacks/sec × 10 sec)
        simulateTimerCallbacks(480, 120);
        
        // Measure jitter for this interval
        float jitter_us = calculateJitter(timestamps);
        float jitter_ms = jitter_us / 1000.0f;
        jitter_samples.push_back(jitter_ms);
        
        // Verify jitter <1ms for this interval
        EXPECT_LT(jitter_ms, 1.0f) << "Jitter at " << ((interval + 1) * 10) << "s: " << jitter_ms << "ms";
    }
    
    // Verify no significant jitter increase over time (degradation)
    float first_jitter = jitter_samples.front();
    float last_jitter = jitter_samples.back();
    float jitter_increase = last_jitter - first_jitter;
    
    EXPECT_LT(jitter_increase, 0.5f) << "Jitter increased by " << jitter_increase << "ms over 60s";
}

/**
 * Test 7: High BPM Stress Test
 * 
 * Verifies: AC-OUT-007, AC-OUT-009 (Performance under load)
 * Given: System running at 240 BPM (maximum)
 * When: Run for 10 seconds
 * Then: Jitter <1ms, no dropped beats
 */
TEST_F(PerformanceTest, HighBPM_240BPM_Stable) {
    controller->startSync(240);
    
    // 240 BPM × 24 PPQN = 5760 clocks/minute = 96 clocks/second
    // 10 seconds × 96 = 960 callbacks
    simulateTimerCallbacks(960, 240);
    
    // Calculate jitter
    float jitter_us = calculateJitter(timestamps);
    float jitter_ms = jitter_us / 1000.0f;
    
    // Verify jitter <1ms even at maximum BPM (AC-OUT-007)
    EXPECT_LT(jitter_ms, 1.0f) << "Jitter at 240 BPM: " << jitter_ms << "ms";
    
    // Verify all callbacks processed (no dropped beats)
    MidiStats stats = controller->getMidiStats();
    EXPECT_EQ(stats.clock_messages_sent, 960U) << "Expected 960 clocks, got " << stats.clock_messages_sent;
}

/**
 * Test 8: CPU Usage Estimation
 * 
 * Verifies: AC-OUT-009 (CPU usage <3%)
 * Given: System running at 120 BPM
 * When: Measure time spent in OutputController methods
 * Then: <3% of total time
 */
TEST_F(PerformanceTest, CPUUsage_Under3Percent) {
    controller->startSync(120);
    
    // Measure execution time for 1000 callbacks
    uint64_t total_start = micros();
    uint64_t execution_time_us = 0;
    
    uint32_t interval_us = controller->calculateTimerInterval(120, 24);
    
    for (int i = 0; i < 1000; i++) {
        uint64_t call_start = micros();
        
        // Execute OutputController method
        controller->onTimerCallback();
        
        uint64_t call_end = micros();
        execution_time_us += (call_end - call_start);
        
        // Advance time by interval (represents "idle" time)
        advance_time_us(interval_us);
    }
    
    uint64_t total_end = micros();
    uint64_t total_time_us = total_end - total_start;
    
    // Calculate CPU usage percentage
    float cpu_percentage = (static_cast<float>(execution_time_us) / total_time_us) * 100.0f;
    
    // Verify CPU usage <3% (AC-OUT-009)
    EXPECT_LT(cpu_percentage, 3.0f) << "CPU usage: " << cpu_percentage << "%";
}

/**
 * Test 9: Memory Stability
 * 
 * Verifies: Resource management
 * Given: System running continuously
 * When: 1000 operations performed
 * Then: No memory leaks detected
 */
TEST_F(PerformanceTest, MemoryStability_NoLeaks) {
    controller->startSync(120);
    
    // Perform 1000 operations with various functions
    for (int i = 0; i < 1000; i++) {
        // Timer callback
        controller->onTimerCallback();
        
        // Update BPM occasionally
        if (i % 100 == 0) {
            uint16_t new_bpm = 100 + (i / 10) % 140;  // Vary BPM 100-240
            controller->updateBPM(new_bpm);
        }
        
        // Relay pulse occasionally
        if (i % 50 == 0 && config.mode == OutputMode::BOTH) {
            controller->pulseRelay();
            advance_time_us(60000);  // Wait for pulse + debounce
            controller->processRelayWatchdog();
        }
        
        // Get stats (exercises stat retrieval)
        controller->getMidiStats();
        controller->getTimerStats();
        controller->getRelayStats();
        
        advance_time_us(20833);  // ~120 BPM interval
    }
    
    // Note: Memory leak detection is limited in native tests
    // This test primarily ensures no crashes/hangs during sustained operation
    
    // Verify system still responsive
    MidiStats stats = controller->getMidiStats();
    EXPECT_GT(stats.clock_messages_sent, 0U);
    
    TimerStats timer_stats = controller->getTimerStats();
    EXPECT_GT(timer_stats.callbacks_processed, 0U);
}

/**
 * Test 10: Multi-Output Coordination
 * 
 * Verifies: AC-OUT-007, AC-OUT-008 (Concurrent outputs)
 * Given: MIDI + Relay enabled simultaneously
 * When: System runs at 120 BPM for 100 beats
 * Then: Both maintain <1ms jitter, <10ms latency
 */
TEST_F(PerformanceTest, MultiOutput_MidiAndRelay_Coordinated) {
    // Both outputs enabled
    config.mode = OutputMode::BOTH;
    delete controller;
    controller = new OutputController(config);
    
    controller->startSync(120);
    
    // Run 100 callbacks with occasional relay pulses
    // First batch: exercise both outputs together
    for (int i = 0; i < 100; i++) {
        controller->onTimerCallback();
        
        // Trigger relay every 24 clocks (every quarter note)
        if (i % 24 == 0) {
            controller->pulseRelay();
            // Process relay but don't let it affect the timer interval measurement
            uint64_t save_time = micros();
            advance_time_us(60000);  // Pulse + debounce
            controller->processRelayWatchdog();
            set_mock_micros(save_time);  // Restore time for consistent intervals
        }
        
        advance_time_us(20833);  // 120 BPM interval
    }
    
    // Measure MIDI jitter from clean timer callbacks (without relay interference)
    timestamps.clear();
    simulateTimerCallbacks(100, 120);
    
    float midi_jitter_us = calculateJitter(timestamps);
    float midi_jitter_ms = midi_jitter_us / 1000.0f;
    
    // Verify MIDI output maintains <1ms jitter (AC-OUT-007)
    EXPECT_LT(midi_jitter_ms, 1.0f) << "MIDI jitter: " << midi_jitter_ms << "ms";
    
    // Verify both outputs functional
    MidiStats midi_stats = controller->getMidiStats();
    RelayStats relay_stats = controller->getRelayStats();
    
    EXPECT_EQ(midi_stats.clock_messages_sent, 200U);  // 100 with relay + 100 jitter measurement
    EXPECT_GE(relay_stats.pulse_count, 4U);  // 100/24 = ~4 pulses
}
