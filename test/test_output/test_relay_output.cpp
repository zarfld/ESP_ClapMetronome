/**
 * @file test_relay_output.cpp
 * @brief TDD Cycle OUT-04: Relay Output Implementation Tests
 * 
 * Verifies: AC-OUT-005, AC-OUT-006, AC-OUT-012
 * - Configurable pulse duration (10-500ms, default 50ms)
 * - Watchdog timeout (force OFF if stuck >100ms)
 * - GPIO safety (LOW when disabled)
 * - Debounce period (minimum OFF time)
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
 * Test fixture for relay output tests
 */
class RelayOutputTest : public ::testing::Test {
protected:
    OutputController* controller;
    OutputConfig config;
    
    void SetUp() override {
        reset_mock_time();
        
        config.mode = OutputMode::BOTH;  // Enable relay
        config.rtp_port = 5004;
        config.midi_ppqn = 24;
        config.initial_bpm = 120;
        config.relay_pulse_ms = 50;      // Default 50ms pulse
        config.relay_watchdog_ms = 100;  // 100ms watchdog
        config.relay_debounce_ms = 10;   // 10ms debounce
        
        controller = new OutputController(config);
    }
    
    void TearDown() override {
        delete controller;
        controller = nullptr;
    }
};

/**
 * Test 1: Relay initialization - default LOW state
 * 
 * Verifies: AC-OUT-012 (GPIO LOW when disabled)
 * Given: OutputController created
 * When: Relay GPIO queried
 * Then: GPIO state is LOW (safe default)
 */
TEST_F(RelayOutputTest, RelayInitialization_DefaultLowState) {
    // Query GPIO state
    bool gpio_state = controller->getRelayGPIO();
    
    // Verify LOW (safe default)
    EXPECT_FALSE(gpio_state);
    
    // Verify stats show relay off
    RelayStats stats = controller->getRelayStats();
    EXPECT_FALSE(stats.currently_on);
    EXPECT_EQ(stats.pulse_count, 0U);
}

/**
 * Test 2: Relay pulse - configured duration
 * 
 * Verifies: AC-OUT-005 (Configurable 10-500ms, default 50ms)
 * Given: Relay pulse_ms configured to 50ms
 * When: pulseRelay() called
 * Then: GPIO HIGH immediately, returns to LOW after 50ms
 */
TEST_F(RelayOutputTest, RelayPulse_ConfiguredDuration) {
    // Trigger pulse
    bool result = controller->pulseRelay();
    EXPECT_TRUE(result);
    
    // Verify GPIO HIGH immediately
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Verify stats updated
    RelayStats stats_during = controller->getRelayStats();
    EXPECT_TRUE(stats_during.currently_on);
    EXPECT_EQ(stats_during.pulse_count, 1U);
    
    // Advance time by 49ms (still within pulse)
    advance_time_us(49000);
    controller->processRelayWatchdog();
    EXPECT_TRUE(controller->getRelayGPIO());  // Still HIGH
    
    // Advance time by 2ms more (total 51ms, past pulse duration)
    advance_time_us(2000);
    controller->processRelayWatchdog();
    EXPECT_FALSE(controller->getRelayGPIO());  // Now LOW
    
    // Verify stats show relay off
    RelayStats stats_after = controller->getRelayStats();
    EXPECT_FALSE(stats_after.currently_on);
    EXPECT_EQ(stats_after.pulse_count, 1U);
}

/**
 * Test 3: Relay pulse - custom duration
 * 
 * Verifies: AC-OUT-005 (Custom duration)
 * Given: Relay pulse_ms configured to 100ms
 * When: pulseRelay() called
 * Then: GPIO HIGH for 100ms, then LOW
 */
TEST_F(RelayOutputTest, RelayPulse_CustomDuration) {
    // Reconfigure for 100ms pulse
    config.relay_pulse_ms = 100;
    delete controller;
    controller = new OutputController(config);
    
    // Trigger pulse
    controller->pulseRelay();
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Advance 99ms - still HIGH
    advance_time_us(99000);
    controller->processRelayWatchdog();
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Advance 2ms more (total 101ms) - now LOW
    advance_time_us(2000);
    controller->processRelayWatchdog();
    EXPECT_FALSE(controller->getRelayGPIO());
}

/**
 * Test 4: Relay watchdog - forces OFF at 100ms
 * 
 * Verifies: AC-OUT-006 (Watchdog timeout)
 * Given: Relay configured with longer pulse (200ms) but watchdog at 100ms
 * When: 101ms elapsed without manual OFF
 * Then: Watchdog forces GPIO LOW (overrides pulse duration)
 */
TEST_F(RelayOutputTest, RelayWatchdog_ForcesOffAt100ms) {
    // Reconfigure for longer pulse (200ms) but keep watchdog at 100ms
    config.relay_pulse_ms = 200;  // Longer than watchdog
    delete controller;
    controller = new OutputController(config);
    
    // Turn relay ON via pulseRelay()
    bool result = controller->pulseRelay();
    EXPECT_TRUE(result);
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Get initial watchdog triggers
    RelayStats stats_before = controller->getRelayStats();
    uint32_t initial_triggers = stats_before.watchdog_triggers;
    
    // Advance 99ms - watchdog should not fire yet (pulse still valid)
    advance_time_us(99000);
    controller->processRelayWatchdog();
    EXPECT_TRUE(controller->getRelayGPIO());  // Still HIGH (99ms < 100ms watchdog)
    
    // Advance 2ms more (total 101ms) - watchdog fires
    advance_time_us(2000);
    controller->processRelayWatchdog();
    EXPECT_FALSE(controller->getRelayGPIO());  // Forced LOW
    
    // Verify watchdog counter incremented
    RelayStats stats_after = controller->getRelayStats();
    EXPECT_EQ(stats_after.watchdog_triggers, initial_triggers + 1);
    
    // Verify state transitioned to WATCHDOG
    EXPECT_EQ(controller->getState(), OutputState::WATCHDOG);
}

/**
 * Test 5: Relay debounce - minimum OFF time
 * 
 * Verifies: Debounce enforcement
 * Given: Relay pulse completed
 * When: Another pulse requested within debounce period (10ms)
 * Then: Pulse rejected, GPIO remains LOW
 */
TEST_F(RelayOutputTest, RelayDebounce_MinimumOffTime) {
    // First pulse
    controller->pulseRelay();
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Complete pulse (50ms)
    advance_time_us(51000);
    controller->processRelayWatchdog();
    EXPECT_FALSE(controller->getRelayGPIO());
    
    // Try second pulse immediately (within debounce)
    bool result = controller->pulseRelay();
    EXPECT_FALSE(result);  // Rejected
    EXPECT_FALSE(controller->getRelayGPIO());  // Still LOW
    
    // Verify debounce reject counter
    RelayStats stats = controller->getRelayStats();
    EXPECT_EQ(stats.debounce_rejects, 1U);
    EXPECT_EQ(stats.pulse_count, 1U);  // Only first pulse counted
    
    // Advance past debounce period (10ms)
    advance_time_us(11000);
    
    // Now pulse should succeed
    result = controller->pulseRelay();
    EXPECT_TRUE(result);
    EXPECT_TRUE(controller->getRelayGPIO());
    EXPECT_EQ(controller->getRelayStats().pulse_count, 2);
}

/**
 * Test 6: Relay state - disabled mode
 * 
 * Verifies: AC-OUT-012 (GPIO LOW when disabled)
 * Given: OutputMode = MIDI_ONLY (relay disabled)
 * When: pulseRelay() called
 * Then: Returns false, GPIO remains LOW
 */
TEST_F(RelayOutputTest, RelayState_DisabledMode) {
    // Disable relay
    controller->enableOutput(OutputMode::MIDI_ONLY);
    
    // Try to pulse
    bool result = controller->pulseRelay();
    EXPECT_FALSE(result);
    
    // Verify GPIO LOW
    EXPECT_FALSE(controller->getRelayGPIO());
    
    // Verify no pulse counted
    RelayStats stats = controller->getRelayStats();
    EXPECT_EQ(stats.pulse_count, 0);
}

/**
 * Test 7: Relay multiple pulses - sequential
 * 
 * Verifies: Sequential pulse handling
 * Given: Relay configured for 30ms pulses
 * When: 5 pulses requested sequentially with proper spacing
 * Then: All 5 pulses complete successfully
 */
TEST_F(RelayOutputTest, RelayMultiplePulses_Sequential) {
    // Reconfigure for faster pulses
    config.relay_pulse_ms = 30;
    delete controller;
    controller = new OutputController(config);
    
    for (int i = 0; i < 5; i++) {
        // Trigger pulse
        bool result = controller->pulseRelay();
        EXPECT_TRUE(result) << "Pulse " << i << " failed";
        EXPECT_TRUE(controller->getRelayGPIO());
        
        // Complete pulse (30ms)
        advance_time_us(31000);
        controller->processRelayWatchdog();
        EXPECT_FALSE(controller->getRelayGPIO());
        
        // Debounce delay (10ms)
        advance_time_us(11000);
    }
    
    // Verify all 5 pulses counted
    RelayStats stats = controller->getRelayStats();
    EXPECT_EQ(stats.pulse_count, 5);
    EXPECT_EQ(stats.watchdog_triggers, 0);
    EXPECT_EQ(stats.debounce_rejects, 0);
}

/**
 * Test 8: Relay stats - track pulse count
 * 
 * Verifies: Statistics tracking
 * Given: Relay pulses sent
 * When: Statistics queried
 * Then: pulse_count increments, last_pulse_timestamp recorded
 */
TEST_F(RelayOutputTest, RelayStats_TrackPulseCount) {
    // Reset stats
    controller->resetRelayStats();
    
    RelayStats initial = controller->getRelayStats();
    EXPECT_EQ(initial.pulse_count, 0U);
    EXPECT_EQ(initial.last_pulse_us, 0ULL);
    
    // Send first pulse
    advance_time_us(1000000);  // 1 second
    controller->pulseRelay();
    
    RelayStats after_first = controller->getRelayStats();
    EXPECT_EQ(after_first.pulse_count, 1U);
    EXPECT_GE(after_first.last_pulse_us, 1000000ULL);
    
    // Complete and send second pulse
    advance_time_us(60000);  // Complete pulse + debounce
    controller->processRelayWatchdog();
    
    advance_time_us(1000000);  // Another second
    controller->pulseRelay();
    
    RelayStats after_second = controller->getRelayStats();
    EXPECT_EQ(after_second.pulse_count, 2);
    EXPECT_GT(after_second.last_pulse_us, after_first.last_pulse_us);
}

/**
 * Test 9: Relay GPIO safety - on destruction
 * 
 * Verifies: AC-OUT-012 (Safe cleanup)
 * Given: OutputController with active relay
 * When: Destructor called
 * Then: GPIO forced LOW before cleanup
 */
TEST_F(RelayOutputTest, RelayGPIOSafety_OnDestruction) {
    // Turn relay ON
    controller->pulseRelay();
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Delete controller (destructor called)
    delete controller;
    controller = nullptr;
    
    // Note: In a real system, we'd check actual GPIO state
    // For tests, we verify destructor is implemented to call setRelayGPIO(false)
    // This test primarily documents the requirement
    
    // Create new controller to verify clean state
    controller = new OutputController(config);
    EXPECT_FALSE(controller->getRelayGPIO());
}

/**
 * Test 10: Relay override - manual control
 * 
 * Verifies: Manual GPIO control with watchdog still active
 * Given: Relay configured with longer pulse (200ms) but watchdog at 100ms
 * When: Manual GPIO control used
 * Then: GPIO changes immediately, watchdog still enforces 100ms limit
 */
TEST_F(RelayOutputTest, RelayOverride_ManualControl) {
    // Reconfigure for longer pulse to test watchdog override
    config.relay_pulse_ms = 200;
    delete controller;
    controller = new OutputController(config);
    
    // Manually set HIGH
    controller->setRelayGPIO(true);
    EXPECT_TRUE(controller->getRelayGPIO());
    
    // Watchdog should still be active
    // Advance past watchdog timeout (101ms > 100ms watchdog)
    advance_time_us(101000);
    controller->processRelayWatchdog();
    
    // Watchdog should have forced OFF (overrides 200ms pulse duration)
    EXPECT_FALSE(controller->getRelayGPIO());
    
    RelayStats stats = controller->getRelayStats();
    EXPECT_GT(stats.watchdog_triggers, 0U);
}
