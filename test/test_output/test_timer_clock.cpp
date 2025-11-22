/**
 * @file test_timer_clock.cpp
 * @brief TDD Cycle OUT-03: Timer-based Clock Sending Tests
 * 
 * Verifies: AC-OUT-007, AC-OUT-008, AC-OUT-009, AC-OUT-010
 * - Hardware timer initialization and configuration
 * - ISR-based clock sending at 24 PPQN
 * - Dynamic BPM updates
 * - Timing jitter <1ms
 * - CPU usage <3%
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (RED phase)
 */

#include <gtest/gtest.h>
#include "output/OutputController.h"

#ifdef NATIVE_BUILD
#include "mocks/time_mock.h"
#endif

/**
 * Test fixture for timer-based clock sending tests
 */
class TimerClockTest : public ::testing::Test {
protected:
    OutputController* controller;
    OutputConfig config;
    
    void SetUp() override {
        reset_mock_time();
        
        config.mode = OutputMode::MIDI_ONLY;
        config.rtp_port = 5004;
        config.midi_ppqn = 24;
        config.initial_bpm = 120;  // Default BPM
        
        controller = new OutputController(config);
    }
    
    void TearDown() override {
        delete controller;
        controller = nullptr;
    }
};

/**
 * Test 1: Timer initialization calculates correct interval
 * 
 * Verifies: AC-OUT-010 (BPM synchronization)
 * Given: OutputController configured with 140 BPM, 24 PPQN
 * When: startSync() called
 * Then: Timer interval = (60,000,000 µs / 140 BPM / 24 PPQN) = 17,857 µs
 */
TEST_F(TimerClockTest, TimerInitialization_CalculatesCorrectInterval) {
    // Set BPM to 140
    controller->setBPM(140);
    
    // Start timer-based clock
    bool result = controller->startTimerClock();
    EXPECT_TRUE(result);
    
    // Get timer configuration
    TimerConfig timer_config = controller->getTimerConfig();
    
    // Verify timer enabled
    EXPECT_TRUE(timer_config.enabled);
    
    // Verify BPM stored
    EXPECT_EQ(timer_config.bpm, 140);
    EXPECT_EQ(timer_config.ppqn, 24);
    
    // Calculate expected interval: 60,000,000 µs/min ÷ 140 BPM ÷ 24 PPQN
    // = 60,000,000 / 140 / 24 = 17,857.14 µs ≈ 17,857 µs
    uint32_t expected_interval = 17857;
    
    // Allow ±1µs tolerance for rounding
    EXPECT_NEAR(timer_config.interval_us, expected_interval, 1);
}

/**
 * Test 2: Timer ISR triggers at 24 PPQN rate
 * 
 * Verifies: AC-OUT-010 (BPM synchronization)
 * Given: Timer running at 120 BPM
 * When: Advance time by 20.833ms (one tick at 120 BPM)
 * Then: Clock pulse counter incremented
 */
TEST_F(TimerClockTest, TimerISR_TriggersAt24PPQN) {
    // Configure for 120 BPM: interval = 60,000,000 / 120 / 24 = 20,833 µs
    controller->setBPM(120);
    controller->startTimerClock();
    
    // Get initial stats
    TimerStats initial_stats = controller->getTimerStats();
    uint32_t initial_interrupts = initial_stats.total_interrupts;
    
    // Advance time by one tick interval (20,833 µs)
    advance_time_us(20833);
    
    // Trigger ISR manually in test
    controller->handleTimerInterrupt();
    
    // Verify interrupt counter incremented
    TimerStats current_stats = controller->getTimerStats();
    EXPECT_EQ(current_stats.total_interrupts, initial_interrupts + 1);
}

/**
 * Test 3: Clock sending from ISR
 * 
 * Verifies: AC-OUT-008 (ISR execution time <10ms network latency)
 * Given: Timer interrupt triggered
 * When: ISR executes
 * Then: MIDI Clock (0xF8) sent via network
 * And: Last clock timestamp recorded
 */
TEST_F(TimerClockTest, ClockSending_FromISR) {
    controller->setBPM(120);
    controller->startTimerClock();
    
    // Get initial network stats
    NetworkStats initial_net = controller->getNetworkStats();
    
    // Trigger ISR
    advance_time_us(20833);
    controller->handleTimerInterrupt();
    
    // Verify clock was sent
    NetworkStats current_net = controller->getNetworkStats();
    EXPECT_EQ(current_net.packets_sent, initial_net.packets_sent + 1);
    
    // Verify packet contains MIDI Clock (0xF8)
    std::vector<uint8_t> packet = controller->getLastSentPacket();
    ASSERT_FALSE(packet.empty());
    
    // MIDI Clock should be in packet (last byte after RTP header)
    bool found_clock = false;
    for (uint8_t byte : packet) {
        if (byte == 0xF8) {
            found_clock = true;
            break;
        }
    }
    EXPECT_TRUE(found_clock);
    
    // Verify timestamp recorded
    TimerStats stats = controller->getTimerStats();
    EXPECT_GT(stats.total_interrupts, 0U);
}

/**
 * Test 4: BPM change updates timer interval
 * 
 * Verifies: AC-OUT-010 (BPM synchronization)
 * Given: Timer running at 120 BPM (interval 20.833ms)
 * When: setBPM(140) called
 * Then: Timer reconfigured to 17.857ms interval
 * And: No clock pulses dropped during transition
 */
TEST_F(TimerClockTest, BPMChange_UpdatesTimerInterval) {
    // Start at 120 BPM
    controller->setBPM(120);
    controller->startTimerClock();
    
    TimerConfig config_120 = controller->getTimerConfig();
    EXPECT_NEAR(config_120.interval_us, 20833, 1);
    
    // Send a few clocks at 120 BPM
    for (int i = 0; i < 5; i++) {
        advance_time_us(20833);
        controller->handleTimerInterrupt();
    }
    
    TimerStats stats_before = controller->getTimerStats();
    uint32_t clocks_before = stats_before.clocks_sent;
    
    // Change to 140 BPM
    bool result = controller->setBPM(140);
    EXPECT_TRUE(result);
    
    TimerConfig config_140 = controller->getTimerConfig();
    EXPECT_NEAR(config_140.interval_us, 17857, 1);
    EXPECT_EQ(config_140.bpm, 140);
    
    // Send clocks at new rate
    for (int i = 0; i < 5; i++) {
        advance_time_us(17857);
        controller->handleTimerInterrupt();
    }
    
    // Verify no clocks dropped (should have 10 total: 5 + 5)
    TimerStats stats_after = controller->getTimerStats();
    EXPECT_EQ(stats_after.clocks_sent, clocks_before + 5);
}

/**
 * Test 5: startSync enables timer
 * 
 * Verifies: Timer control, MIDI Start message
 * Given: Timer disabled, BPM = 130
 * When: startSync() called
 * Then: Timer enabled, interrupts start firing
 * And: MIDI Start (0xFA) sent
 * And: Clock counter reset to 0
 */
TEST_F(TimerClockTest, StartSync_EnablesTimer) {
    controller->setBPM(130);
    
    // Verify timer initially disabled
    TimerConfig config_before = controller->getTimerConfig();
    EXPECT_FALSE(config_before.enabled);
    
    // Start sync
    bool result = controller->startSync();
    EXPECT_TRUE(result);
    
    // Verify timer enabled
    TimerConfig config_after = controller->getTimerConfig();
    EXPECT_TRUE(config_after.enabled);
    
    // Verify MIDI Start (0xFA) sent
    std::vector<uint8_t> packet = controller->getLastSentPacket();
    bool found_start = false;
    for (uint8_t byte : packet) {
        if (byte == 0xFA) {
            found_start = true;
            break;
        }
    }
    EXPECT_TRUE(found_start);
    
    // Verify clock counter reset
    EXPECT_EQ(controller->getClockCounter(), 0);
}

/**
 * Test 6: stopSync disables timer
 * 
 * Verifies: Timer control, MIDI Stop message
 * Given: Timer running
 * When: stopSync() called
 * Then: Timer disabled, no more interrupts
 * And: MIDI Stop (0xFC) sent
 * And: Clock counter holds final value
 */
TEST_F(TimerClockTest, StopSync_DisablesTimer) {
    controller->setBPM(120);
    controller->startSync();
    
    // Send a few clocks
    for (int i = 0; i < 10; i++) {
        advance_time_us(20833);
        controller->handleTimerInterrupt();
    }
    
    uint8_t counter_before_stop = controller->getClockCounter();
    EXPECT_GT(counter_before_stop, 0);
    
    // Stop sync
    bool result = controller->stopSync();
    EXPECT_TRUE(result);
    
    // Verify timer disabled
    TimerConfig timer_config = controller->getTimerConfig();
    EXPECT_FALSE(timer_config.enabled);
    
    // Verify MIDI Stop (0xFC) sent
    std::vector<uint8_t> packet = controller->getLastSentPacket();
    bool found_stop = false;
    for (uint8_t byte : packet) {
        if (byte == 0xFC) {
            found_stop = true;
            break;
        }
    }
    EXPECT_TRUE(found_stop);
    
    // Verify clock counter holds value
    EXPECT_EQ(controller->getClockCounter(), counter_before_stop);
}

/**
 * Test 7: Jitter measurement under threshold
 * 
 * Verifies: AC-OUT-007 (Jitter <1ms over 1000 outputs)
 * Given: Timer running for 100 clock pulses
 * When: Jitter calculated (std deviation of intervals)
 * Then: Jitter <1ms
 * And: Statistics available via getTimerStats()
 */
TEST_F(TimerClockTest, JitterMeasurement_UnderThreshold) {
    controller->setBPM(120);
    controller->startTimerClock();
    
    // Send 100 clock pulses
    for (int i = 0; i < 100; i++) {
        advance_time_us(20833);
        controller->handleTimerInterrupt();
    }
    
    // Get jitter statistics
    TimerStats stats = controller->getTimerStats();
    
    // Verify jitter <1ms (AC-OUT-007)
    EXPECT_LT(stats.jitter_ms, 1.0f);
    
    // Verify statistics populated
    EXPECT_EQ(stats.total_interrupts, 100U);
    EXPECT_EQ(stats.clocks_sent, 100U);
    EXPECT_EQ(stats.missed_clocks, 0U);
}

/**
 * Test 8: ISR execution time under 10µs
 * 
 * Verifies: AC-OUT-008 (ISR execution time <10µs)
 * Given: Timer ISR triggered
 * When: ISR executes (flag set + counter increment)
 * Then: Execution time <10µs (measured via timestamp delta)
 * Note: Full network send happens in loop(), not ISR
 */
TEST_F(TimerClockTest, ISRExecutionTime_Under10us) {
    controller->setBPM(120);
    controller->startTimerClock();
    
    // Trigger ISR and measure execution time
    uint64_t start_us = micros();
    controller->handleTimerInterrupt();
    uint64_t end_us = micros();
    
    uint64_t execution_time = end_us - start_us;
    
    // ISR should complete in <10µs
    EXPECT_LT(execution_time, 10ULL);
    
    // Also verify stats track max ISR time
    TimerStats stats = controller->getTimerStats();
    EXPECT_LT(stats.max_isr_time_us, 10U);
}

/**
 * Test 9: CPU usage under 3%
 * 
 * Verifies: AC-OUT-009 (CPU usage <3%)
 * Given: Timer running at 140 BPM for 10 seconds
 * When: CPU profiling enabled
 * Then: Timer overhead <3%
 * And: Main loop remains responsive
 * 
 * Note: Simplified test - full profiling requires hardware
 */
TEST_F(TimerClockTest, CPUUsage_Under3Percent) {
    controller->setBPM(140);
    controller->startTimerClock();
    
    // Calculate ticks in 10 seconds at 140 BPM:
    // 140 beats/min * 24 ticks/beat = 3360 ticks/min = 56 ticks/sec
    // 10 seconds = 560 ticks
    uint32_t ticks_in_10_sec = 560;
    uint32_t interval_us = 17857;  // 140 BPM interval
    
    // Simulate 10 seconds of operation
    for (uint32_t i = 0; i < ticks_in_10_sec; i++) {
        advance_time_us(interval_us);
        controller->handleTimerInterrupt();
    }
    
    // Get stats
    TimerStats stats = controller->getTimerStats();
    
    // Verify all clocks sent successfully
    EXPECT_EQ(stats.clocks_sent, ticks_in_10_sec);
    
    // Calculate CPU overhead:
    // Total time in ISR = avg_isr_time * total_interrupts
    // Total elapsed time = 10 seconds = 10,000,000 µs
    uint64_t total_isr_time = stats.avg_isr_time_us * stats.total_interrupts;
    uint64_t total_elapsed_time = ticks_in_10_sec * interval_us;
    
    float cpu_usage_percent = (static_cast<float>(total_isr_time) / total_elapsed_time) * 100.0f;
    
    // Verify CPU usage <3% (AC-OUT-009)
    EXPECT_LT(cpu_usage_percent, 3.0f);
}

/**
 * Test 10: Clock counter wraps at 24 PPQN
 * 
 * Verifies: Counter wraparound logic
 * Given: Timer running, clock_counter = 23
 * When: Next clock pulse (24th in beat)
 * Then: Clock counter wraps to 0
 * And: Beat counter increments
 */
TEST_F(TimerClockTest, ClockCounter_Wraps24PPQN) {
    controller->setBPM(120);
    controller->startTimerClock();
    
    // Send 23 clocks (0-22)
    for (int i = 0; i < 23; i++) {
        advance_time_us(20833);
        controller->handleTimerInterrupt();
    }
    
    EXPECT_EQ(controller->getClockCounter(), 23);
    
    // Send 24th clock (should wrap to 0)
    advance_time_us(20833);
    controller->handleTimerInterrupt();
    
    EXPECT_EQ(controller->getClockCounter(), 0);
    
    // Verify total clocks sent = 24
    TimerStats stats = controller->getTimerStats();
    EXPECT_EQ(stats.clocks_sent, 24);
}
