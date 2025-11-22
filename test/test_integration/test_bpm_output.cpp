/**
 * @file test_bpm_output.cpp
 * @brief Integration tests for BPM Calculation Engine and Output Controller
 * 
 * TDD Cycle: OUT-06 (BPM Engine Integration)
 * Tests: 12 integration tests
 * 
 * Acceptance Criteria:
 * - AC-OUT-014: BPM Update Integration
 * - AC-OUT-015: Detection Event Integration
 * - AC-OUT-016: Stability-Based Synchronization
 * - AC-OUT-017: End-to-End Integration
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Integration Process)
 * Phase: Phase 05 - Implementation
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../src/output/OutputController.h"
#include "../../src/integration/BPMOutputBridge.h"
#include "../mocks/MockTimingProvider.h"
#include "../mocks/time_mock.h"

using namespace clap_metronome;

/**
 * @brief Integration Test Fixture
 * 
 * Sets up complete system: BPM engine + Output controller + Bridge
 */
class BPMOutputIntegrationTest : public ::testing::Test {
protected:
    // Components
    MockTimingProvider* timing_;
    BPMCalculation* bpm_engine_;
    OutputController* output_controller_;
    BPMOutputBridge* bridge_;
    
    void SetUp() override {
        // Reset time mock
        reset_mock_time();
        
        // Create timing provider
        timing_ = new MockTimingProvider();
        
        // Create BPM engine
        bpm_engine_ = new BPMCalculation(timing_);
        bpm_engine_->init();
        
        // Create output controller
        output_controller_ = new OutputController();
        
        // Configure output for tests
        OutputConfig config = output_controller_->getConfig();
        config.mode = OutputMode::BOTH;
        config.midi_send_start_stop = true;
        config.midi_ppqn = 24;
        config.relay_pulse_ms = 50;
        output_controller_->setConfig(config);
        
        // Create integration bridge
        bridge_ = new BPMOutputBridge(bpm_engine_, output_controller_);
        bridge_->init();
    }
    
    void TearDown() override {
        delete bridge_;
        delete output_controller_;
        delete bpm_engine_;
        delete timing_;
    }
    
    /**
     * @brief Add taps at specified BPM
     * @param count Number of taps to add
     * @param bpm Target BPM (determines interval)
     */
    void addTapsAtBPM(uint8_t count, uint16_t bpm) {
        // Calculate interval for desired BPM
        // BPM = 60,000,000 / interval_us
        // interval_us = 60,000,000 / BPM
        uint64_t interval_us = 60000000UL / bpm;
        
        for (uint8_t i = 0; i < count; i++) {
            uint64_t timestamp = micros();
            bpm_engine_->addTap(timestamp);
            
            if (i < count - 1) {  // Don't advance after last tap
                advance_time_us(interval_us);
            }
        }
    }
};

// ========== Category 1: BPM Update Integration (3 tests) ==========

/**
 * Test 1: BPM Update Updates Timer Interval
 * 
 * Verifies: AC-OUT-014 (BPM Update Integration)
 * Given: BPM engine connected to OutputController
 * When: BPM calculated (4 taps at 120 BPM)
 * Then: Timer interval updated to match BPM
 */
TEST_F(BPMOutputIntegrationTest, BPMUpdate_UpdatesTimerInterval) {
    // Add 4 taps at 120 BPM
    addTapsAtBPM(4, 120);
    
    // Verify BPM calculated
    float calculated_bpm = bpm_engine_->getBPM();
    EXPECT_NEAR(calculated_bpm, 120.0f, 2.0f);  // Within 2 BPM tolerance
    
    // Verify timer interval updated in OutputController
    // Expected: 60,000,000 / 120 / 24 = 20833 µs
    uint32_t expected_interval = output_controller_->calculateTimerInterval(120, 24);
    EXPECT_EQ(expected_interval, 20833U);
    
    // Verify timer stats reflect new interval
    TimerStats stats = output_controller_->getTimerStats();
    EXPECT_EQ(stats.interval_us, expected_interval);
}

/**
 * Test 2: BPM Update Handles Multiple BPM Changes
 * 
 * Verifies: AC-OUT-014 (Dynamic BPM updates)
 * Given: System syncing at initial BPM
 * When: BPM changes multiple times
 * Then: Timer interval adjusts for each change
 */
TEST_F(BPMOutputIntegrationTest, BPMUpdate_MultipleBPMChanges) {
    // Start at 120 BPM
    addTapsAtBPM(4, 120);
    float bpm1 = bpm_engine_->getBPM();
    EXPECT_NEAR(bpm1, 120.0f, 2.0f);
    
    TimerStats stats1 = output_controller_->getTimerStats();
    uint32_t interval1 = stats1.interval_us;
    EXPECT_NEAR(interval1, 20833, 1000);  // ~20833µs ±1ms
    
    // Change to 100 BPM
    advance_time_us(1000000);  // 1 second gap
    bpm_engine_->clear();
    addTapsAtBPM(4, 100);
    float bpm2 = bpm_engine_->getBPM();
    EXPECT_NEAR(bpm2, 100.0f, 2.0f);
    
    TimerStats stats2 = output_controller_->getTimerStats();
    uint32_t interval2 = stats2.interval_us;
    EXPECT_NEAR(interval2, 25000, 1000);  // ~25000µs ±1ms
    EXPECT_NE(interval1, interval2);      // Should be different
    
    // Change to 140 BPM
    advance_time_us(1000000);  // 1 second gap
    bpm_engine_->clear();
    addTapsAtBPM(4, 140);
    float bpm3 = bpm_engine_->getBPM();
    EXPECT_NEAR(bpm3, 140.0f, 2.0f);
    
    TimerStats stats3 = output_controller_->getTimerStats();
    uint32_t interval3 = stats3.interval_us;
    EXPECT_NEAR(interval3, 17857, 1000);  // ~17857µs ±1ms
    EXPECT_NE(interval2, interval3);      // Should be different
}

/**
 * Test 3: BPM Update Ignores Invalid BPM
 * 
 * Verifies: AC-OUT-014 (BPM validation)
 * Given: System running at valid BPM
 * When: Invalid BPM values provided
 * Then: Updates rejected, timer unchanged
 */
TEST_F(BPMOutputIntegrationTest, BPMUpdate_IgnoresInvalidBPM) {
    // Start at valid 120 BPM
    addTapsAtBPM(4, 120);
    TimerStats initial_stats = output_controller_->getTimerStats();
    uint32_t initial_interval = initial_stats.interval_us;
    
    // Attempt too-low BPM (30 BPM, valid range 40-240)
    output_controller_->updateBPM(30);
    TimerStats stats_after_low = output_controller_->getTimerStats();
    EXPECT_EQ(stats_after_low.interval_us, initial_interval);  // Unchanged
    
    // Attempt too-high BPM (250 BPM, valid range 40-240)
    output_controller_->updateBPM(250);
    TimerStats stats_after_high = output_controller_->getTimerStats();
    EXPECT_EQ(stats_after_high.interval_us, initial_interval);  // Unchanged
    
    // Valid BPM should work (100 BPM)
    output_controller_->updateBPM(100);
    TimerStats stats_after_valid = output_controller_->getTimerStats();
    EXPECT_NE(stats_after_valid.interval_us, initial_interval);  // Changed
    EXPECT_NEAR(stats_after_valid.interval_us, 25000, 1000);
}

// ========== Category 2: Detection Event Integration (3 tests) ==========

/**
 * Test 4: Detection Event Triggers Relay Pulse
 * 
 * Verifies: AC-OUT-015 (Detection event integration)
 * Given: Output in RELAY_ONLY mode
 * When: Beat detection event occurs
 * Then: Relay pulse triggered
 */
TEST_F(BPMOutputIntegrationTest, DetectionEvent_TriggersRelayPulse) {
    // Configure relay-only mode
    OutputConfig config = output_controller_->getConfig();
    config.mode = OutputMode::RELAY_ONLY;
    output_controller_->setConfig(config);
    
    // Get initial relay stats
    RelayStats initial_stats = output_controller_->getRelayStats();
    uint32_t initial_pulses = initial_stats.pulse_count;
    
    // Simulate beat detection
    uint64_t detection_time = micros();
    bridge_->onBeatDetected(detection_time);
    
    // Verify relay pulse triggered
    RelayStats after_stats = output_controller_->getRelayStats();
    EXPECT_EQ(after_stats.pulse_count, initial_pulses + 1);
    
    // Verify relay state
    EXPECT_TRUE(output_controller_->isRelayActive());
    
    // Wait for pulse duration (50ms default)
    advance_time_us(50000);
    output_controller_->processRelayWatchdog();
    
    // Relay should be off after pulse
    EXPECT_FALSE(output_controller_->isRelayActive());
}

/**
 * Test 5: Detection Event - MIDI and Relay Coordinated
 * 
 * Verifies: AC-OUT-015 (Concurrent outputs)
 * Given: System in BOTH mode, syncing at 120 BPM
 * When: Beat detection occurs
 * Then: Both relay pulse and MIDI clocks continue
 */
TEST_F(BPMOutputIntegrationTest, DetectionEvent_MIDIAndRelayCoordinated) {
    // Start sync at 120 BPM
    addTapsAtBPM(4, 120);
    output_controller_->startSync(120);
    
    // Get initial stats
    MidiStats initial_midi = output_controller_->getMidiStats();
    RelayStats initial_relay = output_controller_->getRelayStats();
    
    // Simulate beat detection
    bridge_->onBeatDetected(micros());
    
    // Run some timer callbacks to send MIDI clocks
    for (int i = 0; i < 10; i++) {
        output_controller_->onTimerCallback();
        advance_time_us(20833);  // 120 BPM interval
    }
    
    // Verify both outputs active
    MidiStats after_midi = output_controller_->getMidiStats();
    RelayStats after_relay = output_controller_->getRelayStats();
    
    // MIDI clocks sent
    EXPECT_GT(after_midi.clock_messages_sent, initial_midi.clock_messages_sent);
    EXPECT_EQ(after_midi.clock_messages_sent, initial_midi.clock_messages_sent + 10);
    
    // Relay pulse occurred
    EXPECT_EQ(after_relay.pulse_count, initial_relay.pulse_count + 1);
}

/**
 * Test 6: Detection Event Respects Debounce
 * 
 * Verifies: AC-OUT-015 (Relay debounce)
 * Given: Relay with 10ms debounce
 * When: Rapid detection events
 * Then: Debounce prevents immediate re-trigger
 */
TEST_F(BPMOutputIntegrationTest, DetectionEvent_RespectsDebounce) {
    // Configure 10ms debounce
    OutputConfig config = output_controller_->getConfig();
    config.mode = OutputMode::RELAY_ONLY;
    config.relay_debounce_ms = 10;
    output_controller_->setConfig(config);
    
    // First pulse
    bridge_->onBeatDetected(micros());
    RelayStats stats1 = output_controller_->getRelayStats();
    uint32_t pulses_after_1 = stats1.pulse_count;
    
    // Wait for pulse to complete (50ms)
    advance_time_us(50000);
    output_controller_->processRelayWatchdog();
    
    // Immediate re-trigger (within debounce)
    advance_time_us(5000);  // 5ms later
    bridge_->onBeatDetected(micros());
    RelayStats stats2 = output_controller_->getRelayStats();
    EXPECT_EQ(stats2.pulse_count, pulses_after_1);  // Rejected
    
    // After debounce period
    advance_time_us(10000);  // 10ms more (total 15ms from end)
    bridge_->onBeatDetected(micros());
    RelayStats stats3 = output_controller_->getRelayStats();
    EXPECT_EQ(stats3.pulse_count, pulses_after_1 + 1);  // Accepted
}

// ========== Category 3: Stability-Based Synchronization (3 tests) ==========

/**
 * Test 7: Stable Detection Starts Sync
 * 
 * Verifies: AC-OUT-016 (Stability-based auto-sync)
 * Given: BPM engine and output connected with auto-sync enabled
 * When: BPM becomes stable (4 taps, CV < 5%)
 * Then: MIDI sync starts automatically
 */
TEST_F(BPMOutputIntegrationTest, StableDetection_StartsSync) {
    // Enable auto-sync
    bridge_->setAutoSyncEnabled(true);
    
    // Configure MIDI-only mode
    OutputConfig config = output_controller_->getConfig();
    config.mode = OutputMode::MIDI_ONLY;
    output_controller_->setConfig(config);
    
    // Add 4 taps at 120 BPM (should be stable)
    addTapsAtBPM(4, 120);
    
    // Verify BPM is stable
    EXPECT_TRUE(bpm_engine_->isStable());
    float bpm = bpm_engine_->getBPM();
    EXPECT_NEAR(bpm, 120.0f, 2.0f);
    
    // Verify sync started
    OutputState state = output_controller_->getState();
    EXPECT_EQ(state, OutputState::RUNNING);
    
    // Verify START message sent
    MidiStats stats = output_controller_->getMidiStats();
    EXPECT_GE(stats.start_messages_sent, 1U);
}

/**
 * Test 8: Unstable BPM No Auto Sync
 * 
 * Verifies: AC-OUT-016 (Requires stability for auto-sync)
 * Given: Auto-sync enabled
 * When: BPM calculated but unstable (< 4 taps)
 * Then: Sync does NOT start automatically
 */
TEST_F(BPMOutputIntegrationTest, UnstableBPM_NoAutoSync) {
    // Enable auto-sync
    bridge_->setAutoSyncEnabled(true);
    
    // Add only 2 taps (minimum for BPM but not stable)
    addTapsAtBPM(2, 120);
    
    // Verify BPM calculated but unstable
    float bpm = bpm_engine_->getBPM();
    EXPECT_GT(bpm, 0.0f);  // BPM exists
    EXPECT_FALSE(bpm_engine_->isStable());  // But not stable (needs 4+ taps)
    
    // Verify sync did NOT start
    OutputState state = output_controller_->getState();
    EXPECT_EQ(state, OutputState::STOPPED);
    
    // No MIDI messages sent
    MidiStats stats = output_controller_->getMidiStats();
    EXPECT_EQ(stats.start_messages_sent, 0);
}

/**
 * Test 9: Stability Loss Maintains Sync
 * 
 * Verifies: AC-OUT-016 (Sync continues through temporary instability)
 * Given: System syncing at stable BPM
 * When: Temporary instability (erratic tap)
 * Then: Sync continues, BPM adjusts
 */
TEST_F(BPMOutputIntegrationTest, StabilityLoss_MaintainsSync) {
    // Start with stable 120 BPM
    bridge_->setAutoSyncEnabled(true);
    addTapsAtBPM(4, 120);
    
    // Verify syncing
    EXPECT_TRUE(bpm_engine_->isStable());
    EXPECT_EQ(output_controller_->getState(), OutputState::RUNNING);
    
    uint32_t initial_clocks = output_controller_->getMidiStats().clock_messages_sent;
    
    // Add erratic tap (causes instability)
    advance_time_us(200000);  // 200ms (very slow, will affect stability)
    bpm_engine_->addTap(micros());
    
    // May lose stability temporarily
    // (depends on buffer and CV calculation)
    
    // Verify sync continues
    OutputState state_after = output_controller_->getState();
    EXPECT_EQ(state_after, OutputState::RUNNING);  // Still running
    
    // Run some callbacks to verify clocks still sent
    for (int i = 0; i < 10; i++) {
        output_controller_->onTimerCallback();
        advance_time_us(20833);
    }
    
    uint32_t clocks_after = output_controller_->getMidiStats().clock_messages_sent;
    EXPECT_GT(clocks_after, initial_clocks);  // Clocks continue
    
    // No STOP message sent
    EXPECT_EQ(output_controller_->getMidiStats().stop_messages_sent, 0);
}

// ========== Category 4: End-to-End Integration (3 tests) ==========

/**
 * Test 10: End-to-End - Clap to BPM to Output
 * 
 * Verifies: AC-OUT-017 (Full pipeline)
 * Given: Complete system (detection → BPM → output)
 * When: Series of clap detections
 * Then: BPM calculated, outputs synchronized
 */
TEST_F(BPMOutputIntegrationTest, EndToEnd_ClapToBPMToOutput) {
    // Enable auto-sync
    bridge_->setAutoSyncEnabled(true);
    
    // Configure BOTH mode (MIDI + Relay)
    OutputConfig config = output_controller_->getConfig();
    config.mode = OutputMode::BOTH;
    output_controller_->setConfig(config);
    
    // Simulate 4 clap detections at 120 BPM
    // Each clap: detection event + tap
    uint64_t interval_us = 60000000UL / 120;  // 500ms
    
    for (int i = 0; i < 4; i++) {
        uint64_t detection_time = micros();
        
        // Detection event (triggers relay)
        bridge_->onBeatDetected(detection_time);
        
        // Tap event (for BPM calculation)
        bpm_engine_->addTap(detection_time);
        
        if (i < 3) {
            advance_time_us(interval_us);
        }
    }
    
    // Verify BPM calculated
    float bpm = bpm_engine_->getBPM();
    EXPECT_NEAR(bpm, 120.0f, 2.0f);
    EXPECT_TRUE(bpm_engine_->isStable());
    
    // Verify relay pulses on each detection
    RelayStats relay_stats = output_controller_->getRelayStats();
    EXPECT_EQ(relay_stats.pulse_count, 4);
    
    // Verify MIDI sync started
    EXPECT_EQ(output_controller_->getState(), OutputState::RUNNING);
    
    // Verify timer interval correct
    TimerStats timer_stats = output_controller_->getTimerStats();
    EXPECT_NEAR(timer_stats.interval_us, 20833, 1000);  // 120 BPM
}

/**
 * Test 11: End-to-End - BPM Change Output Adjusts
 * 
 * Verifies: AC-OUT-017 (Dynamic adjustment)
 * Given: System running at initial BPM
 * When: Tempo changes (new taps at different BPM)
 * Then: Output adjusts to new BPM seamlessly
 */
TEST_F(BPMOutputIntegrationTest, EndToEnd_BPMChange_OutputAdjusts) {
    // Start at 120 BPM
    bridge_->setAutoSyncEnabled(true);
    addTapsAtBPM(4, 120);
    
    EXPECT_EQ(output_controller_->getState(), OutputState::RUNNING);
    TimerStats initial_timer = output_controller_->getTimerStats();
    EXPECT_NEAR(initial_timer.interval_us, 20833, 1000);
    
    // Run some clocks at 120 BPM
    for (int i = 0; i < 20; i++) {
        output_controller_->onTimerCallback();
        advance_time_us(20833);
    }
    
    uint32_t clocks_at_120 = output_controller_->getMidiStats().clock_messages_sent;
    
    // Change tempo to 140 BPM
    advance_time_us(1000000);  // 1 second gap
    bpm_engine_->clear();
    addTapsAtBPM(4, 140);
    
    // Verify BPM updated
    EXPECT_NEAR(bpm_engine_->getBPM(), 140.0f, 2.0f);
    
    // Verify timer interval adjusted
    TimerStats new_timer = output_controller_->getTimerStats();
    EXPECT_NEAR(new_timer.interval_us, 17857, 1000);  // 140 BPM
    EXPECT_NE(new_timer.interval_us, initial_timer.interval_us);
    
    // Verify sync continues (no stop/restart)
    EXPECT_EQ(output_controller_->getState(), OutputState::RUNNING);
    
    // Run clocks at new BPM
    for (int i = 0; i < 20; i++) {
        output_controller_->onTimerCallback();
        advance_time_us(17857);  // New interval
    }
    
    // Verify clocks continued
    uint32_t total_clocks = output_controller_->getMidiStats().clock_messages_sent;
    EXPECT_EQ(total_clocks, clocks_at_120 + 20);
}

/**
 * Test 12: End-to-End - Stop and Restart
 * 
 * Verifies: AC-OUT-017 (Clean lifecycle)
 * Given: System syncing
 * When: Stop, clear, and restart with new tempo
 * Then: Clean stop/start cycle
 */
TEST_F(BPMOutputIntegrationTest, EndToEnd_StopAndRestart) {
    // Start at 120 BPM
    bridge_->setAutoSyncEnabled(true);
    addTapsAtBPM(4, 120);
    
    EXPECT_EQ(output_controller_->getState(), OutputState::RUNNING);
    MidiStats stats1 = output_controller_->getMidiStats();
    EXPECT_GE(stats1.start_messages_sent, 1);
    
    // Stop sync
    output_controller_->stopSync();
    
    // Verify stopped
    EXPECT_EQ(output_controller_->getState(), OutputState::STOPPED);
    MidiStats stats2 = output_controller_->getMidiStats();
    EXPECT_GE(stats2.stop_messages_sent, 1);
    
    // Clear taps
    bpm_engine_->clear();
    EXPECT_EQ(bpm_engine_->getTapCount(), 0);
    EXPECT_FLOAT_EQ(bpm_engine_->getBPM(), 0.0f);
    
    // Start new sequence at 100 BPM
    advance_time_us(2000000);  // 2 second gap
    addTapsAtBPM(4, 100);
    
    // Verify new BPM calculated
    EXPECT_NEAR(bpm_engine_->getBPM(), 100.0f, 2.0f);
    EXPECT_TRUE(bpm_engine_->isStable());
    
    // Verify auto-sync restarted
    EXPECT_EQ(output_controller_->getState(), OutputState::RUNNING);
    
    // Verify new START message sent
    MidiStats stats3 = output_controller_->getMidiStats();
    EXPECT_GE(stats3.start_messages_sent, 2);  // Second start
    
    // Verify correct interval
    TimerStats timer = output_controller_->getTimerStats();
    EXPECT_NEAR(timer.interval_us, 25000, 1000);  // 100 BPM
}
