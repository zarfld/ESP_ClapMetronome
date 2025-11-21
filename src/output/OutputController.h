/**
 * @file OutputController.h
 * @brief Output Controller - MIDI and Relay Output Management
 * 
 * Component: DES-C-004 (#50)
 * Implements: REQ-F-008 (MIDI), REQ-F-009 (Relay), REQ-NF-001 (Real-time)
 * Architecture: ARC-C-004
 * Interfaces: DES-I-013 (Output Trigger), DES-I-006 (BPM Update consumer)
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practices: Test-Driven Development (TDD)
 * 
 * Acceptance Criteria:
 * - AC-OUT-001: MIDI Beat Clock format (0xF8 timing clock)
 * - AC-OUT-002: MIDI START message (0xFA)
 * - AC-OUT-003: RTP-MIDI network transport (UDP over WiFi)
 * - AC-OUT-004: MIDI clock rate (24 PPQN - pulses per quarter note)
 * - AC-OUT-005: Relay pulse duration (10-500ms, default 50ms)
 * - AC-OUT-006: Relay watchdog (force OFF if stuck >100ms)
 * - AC-OUT-007: Output timing jitter (<1ms)
 * - AC-OUT-008: Network latency handling (<10ms compensation)
 * - AC-OUT-009: CPU usage (<3%)
 * - AC-OUT-010: BPM synchronization (clock interval = 60s / BPM / 24)
 * - AC-OUT-011: MIDI STOP message (0xFC)
 * - AC-OUT-012: Relay GPIO safety (LOW when disabled)
 * - AC-OUT-013: State machine (STOPPED → RUNNING → STOPPED)
 * 
 * @see 04-design/PHASE-04-COMPLETION-SUMMARY.md (DES-C-004)
 * @see 04-design/tdd-plan-phase-05.md (Section 4.7)
 */

#ifndef OUTPUT_CONTROLLER_H
#define OUTPUT_CONTROLLER_H

#include <stdint.h>
#include <functional>
#include <vector>

/**
 * @brief MIDI Beat Clock Message Types (DES-D-007)
 * 
 * MIDI Beat Clock (System Real-Time Messages):
 * - 0xF8: Timing Clock (sent 24 times per quarter note)
 * - 0xFA: Start (begin sequence playback)
 * - 0xFB: Continue (resume from stop point)
 * - 0xFC: Stop (stop sequence playback)
 * 
 * These are single-byte messages (no data bytes).
 * Transmitted via RTP-MIDI (MIDI over UDP/WiFi) instead of DIN MIDI.
 * 
 * @see https://en.wikipedia.org/wiki/MIDI_beat_clock
 * @see https://en.wikipedia.org/wiki/RTP-MIDI
 */
enum class MIDIClockMessage : uint8_t {
    TIMING_CLOCK = 0xF8,  ///< Timing clock (24 ppqn)
    START = 0xFA,         ///< Start playback
    CONTINUE = 0xFB,      ///< Continue playback
    STOP = 0xFC           ///< Stop playback
};

/**
 * @brief Output Mode Configuration (AC-OUT-001)
 * 
 * Defines which output types are active.
 */
enum class OutputMode : uint8_t {
    DISABLED_MODE = 0, ///< No outputs (renamed to avoid Arduino DISABLED macro)
    MIDI_ONLY = 1,     ///< MIDI output only
    RELAY_ONLY = 2,    ///< Relay output only
    BOTH = 3           ///< Both MIDI and Relay
};

/**
 * @brief Output State Machine States (AC-OUT-013)
 */
enum class OutputState : uint8_t {
    STOPPED = 0,       ///< Playback stopped (no clocks)
    RUNNING = 1,       ///< Playback running (sending clocks)
    WATCHDOG = 3       ///< Watchdog triggered (relay stuck)
};

/**
 * @brief Output Configuration
 * 
 * Configuration for RTP-MIDI Beat Clock and Relay outputs.
 */
struct OutputConfig {
    // RTP-MIDI Beat Clock Configuration
    bool midi_send_start_stop;      ///< Send START/STOP messages (default true)
    uint8_t midi_ppqn;              ///< Pulses per quarter note (default 24 = standard)
    uint16_t rtp_port;              ///< RTP-MIDI UDP port (default 5004)
    
    // Relay Configuration
    uint16_t relay_pulse_ms;        ///< Relay pulse duration (10-500ms, default 50ms)
    uint16_t relay_watchdog_ms;     ///< Relay watchdog timeout (default 100ms)
    uint16_t relay_debounce_ms;     ///< Minimum OFF time (default 10ms)
    
    // Output Mode
    OutputMode mode;                ///< Which outputs are enabled
    
    // Timer Configuration
    uint16_t initial_bpm;           ///< Initial BPM for timer (default 120)
    
    /**
     * @brief Default constructor with safe defaults
     */
    OutputConfig() 
        : midi_send_start_stop(true) // Send START/STOP messages
        , midi_ppqn(24)              // 24 PPQN (MIDI standard)
        , rtp_port(5004)             // RTP-MIDI standard port
        , relay_pulse_ms(50)         // 50ms relay pulse
        , relay_watchdog_ms(100)     // 100ms watchdog
        , relay_debounce_ms(10)      // 10ms debounce
        , mode(OutputMode::BOTH)     // Both outputs enabled
        , initial_bpm(120)           // 120 BPM default tempo
    {}
};

/**
 * @brief Network Protocol Type
 */
enum class NetworkProtocol : uint8_t {
    UDP = 0,           ///< UDP protocol (RTP-MIDI)
    TCP = 1            ///< TCP protocol (future)
};

/**
 * @brief Network Configuration Information
 */
struct NetworkConfig {
    bool socket_open;              ///< UDP socket is open
    uint16_t local_port;           ///< Local bind port (5004)
    uint16_t data_port;            ///< Data port (5005)
    NetworkProtocol protocol;      ///< Transport protocol
};

/**
 * @brief Network Statistics
 */
struct NetworkStats {
    uint32_t packets_sent;         ///< Total packets transmitted
    uint32_t bytes_sent;           ///< Total bytes transmitted
    uint32_t send_failures;        ///< Failed transmissions
    uint32_t last_latency_us;      ///< Last send latency (microseconds)
};

/**
 * @brief Timer Configuration (AC-OUT-007, AC-OUT-010)
 */
struct TimerConfig {
    uint32_t interval_us;          ///< Timer interval in microseconds
    bool enabled;                  ///< Timer is active
    uint16_t bpm;                  ///< Current BPM
    uint8_t ppqn;                  ///< Pulses per quarter note (24)
};

/**
 * @brief Timer Statistics (Performance Monitoring)
 */
struct TimerStats {
    uint32_t total_interrupts;     ///< Total ISR invocations
    uint32_t clocks_sent;          ///< Successful clock transmissions
    uint32_t missed_clocks;        ///< Dropped due to overrun
    float jitter_ms;               ///< Timing jitter (std deviation)
    uint32_t avg_isr_time_us;      ///< Average ISR execution time
    uint32_t max_isr_time_us;      ///< Maximum ISR execution time
    uint32_t callbacks_processed;  ///< Alias for total_interrupts (for compatibility)
    uint32_t interval_us;          ///< Current timer interval
    bool timer_running;            ///< Timer is active
};

/**
 * @brief Relay Statistics (Pulse Tracking)
 */
struct RelayStats {
    uint32_t pulse_count;          ///< Total pulses since start
    uint32_t watchdog_triggers;    ///< Times watchdog fired
    uint32_t debounce_rejects;     ///< Pulses rejected by debounce
    uint64_t last_pulse_us;        ///< Timestamp of last pulse
    bool currently_on;             ///< Current GPIO state
};

/**
 * @brief MIDI Statistics (Message Tracking)
 */
struct MidiStats {
    uint32_t clock_messages_sent;      ///< 0xF8 timing clocks sent
    uint32_t start_messages_sent;      ///< 0xFA start messages sent
    uint32_t stop_messages_sent;       ///< 0xFC stop messages sent
    uint32_t continue_messages_sent;   ///< 0xFB continue messages sent
    uint64_t last_message_us;          ///< Timestamp of last MIDI message
};

/**
 * @brief Output State Information (for telemetry/monitoring)
 * 
 * Provides detailed state information for testing and monitoring.
 */
struct OutputStateInfo {
    bool     midi_enabled;          ///< MIDI output is active
    bool     relay_enabled;         ///< Relay output is active
    bool     syncing;               ///< Clock stream is active
    uint32_t clocks_sent;           ///< Total 0xF8 messages since START
    uint32_t last_clock_time_ms;    ///< millis() of last clock
    float    current_bpm;           ///< Active BPM tempo
    OutputMode mode;                ///< Current output mode
};

/**
 * @brief Output Controller Class
 * 
 * Manages MIDI and Relay outputs synchronized to BPM.
 * 
 * DES-I-013: Output Trigger Interface
 * - sendMIDIClock(): Send MIDI timing clock (0xF8) via RTP-MIDI
 * - sendMIDIStart(): Send MIDI start (0xFA) via RTP-MIDI
 * - sendMIDIStop(): Send MIDI stop (0xFC) via RTP-MIDI
 * - pulseRelay(): Trigger relay pulse
 * - enableOutput(): Enable/disable outputs
 * 
 * DES-I-006: BPM Update Consumer
 * - Subscribes to BPM updates from BPM Calculation Engine
 * - Adjusts output interval based on current BPM
 * 
 * Performance:
 * - Network latency: <10ms (AC-OUT-008)
 * - CPU usage: <3% (AC-OUT-009)
 * - Timing jitter: <1ms (AC-OUT-007)
 * 
 * Network:
 * - Protocol: RTP-MIDI (RFC 6295) over UDP
 * - Port: 5004 (control) and 5005 (data) by default
 * - Multicast or unicast to MIDI receivers
 */
class OutputController {
public:
    /**
     * @brief Constructor
     * 
     * @param config Output configuration
     */
    explicit OutputController(const OutputConfig& config = OutputConfig());
    
    /**
     * @brief Destructor
     */
    ~OutputController();
    
    /**
     * @brief Initialize output controller
     * 
     * Resets state to defaults. Called automatically by constructor,
     * but can be called again to reset state.
     */
    void init();
    
    // ========== DES-I-013: Output Trigger Interface ==========
    
    /**
     * @brief Send MIDI Beat Clock message (0xF8)
     * 
     * Sends a single timing clock byte. Should be called 24 times per quarter note.
     * 
     * AC-OUT-001: MIDI timing clock format (0xF8)
     * AC-OUT-004: 24 PPQN (standard MIDI beat clock)
     * 
     * @return true if sent successfully
     */
    bool sendMIDIClock();
    
    /**
     * @brief Send MIDI Start message (0xFA)
     * 
     * Signals the start of playback. Should be sent before first clock.
     * 
     * AC-OUT-002: MIDI START format (0xFA)
     * 
     * @return true if sent successfully
     */
    bool sendMIDIStart();
    
    /**
     * @brief Send MIDI Stop message (0xFC)
     * 
     * Signals the end of playback. No more clocks sent after this.
     * 
     * AC-OUT-011: MIDI STOP format (0xFC)
     * 
     * @return true if sent successfully
     */
    bool sendMIDIStop();
    
    /**
     * @brief Trigger relay pulse
     * 
     * AC-OUT-005: Configurable 10-500ms pulse (default 50ms)
     * AC-OUT-006: Watchdog forces OFF if stuck >100ms
     * AC-OUT-012: GPIO LOW when disabled
     * 
     * @return true if triggered successfully
     */
    bool pulseRelay();
    
    /**
     * @brief Enable or disable outputs
     * 
     * @param mode Output mode (DISABLED, MIDI_ONLY, RELAY_ONLY, BOTH)
     */
    void enableOutput(OutputMode mode);
    
    /**
     * @brief Check if outputs are enabled
     * 
     * @return true if any output is enabled
     */
    bool isEnabled() const;
    
    // ========== BPM Synchronization (AC-OUT-010) ==========
    
    /**
     * @brief Set BPM for synchronized outputs
     * 
     * Adjusts MIDI clock interval and timer to match BPM.
     * Clock interval = 60 seconds / BPM / 24 PPQN
     * Example: 120 BPM → 60/120/24 = 0.020833s = 20.833ms per clock
     * 
     * AC-OUT-010: Clock interval matches BPM (24 clocks per beat)
     * 
     * @param bpm Beats per minute (50-300 BPM)
     * @return true if BPM set successfully
     * 
     * @note Replaces old void setBPM(float) - now returns bool and uses uint16_t
     */
    bool setBPM(uint16_t bpm);
    
    /**
     * @brief Get current BPM
     * 
     * @return Current BPM value
     */
    float getBPM() const;
    
    /**
     * @brief Update BPM dynamically
     * 
     * Changes BPM while sync is running. Recalculates timer interval.
     * 
     * @param bpm New BPM value (40-240)
     */
    void updateBPM(uint16_t bpm);
    
    /**
     * @brief Start synchronized output
     * 
     * Sends MIDI START (0xFA) and begins timer-based periodic clock output.
     * Clocks sent at 24 PPQN based on current BPM.
     * 
     * AC-OUT-002: Send START before first clock
     * 
     * @return true if started successfully
     */
    bool startSync();
    
    /**
     * @brief Start synchronized output with specific BPM
     * 
     * Sets BPM and starts synchronized output in one call.
     * 
     * @param bpm Beats per minute (40-240)
     * @return true if started successfully
     */
    bool startSync(uint16_t bpm);
    
    /**
     * @brief Stop synchronized output
     * 
     * Sends MIDI STOP (0xFC) and halts timer-based clock output.
     * 
     * AC-OUT-011: Send STOP to end playback
     * 
     * @return true if stopped successfully
     */
    bool stopSync();
    
    /**
     * @brief Check if synchronized output is running
     * 
     * @return true if sync is active
     */
    bool isSyncing() const;
    
    // ========== Configuration ==========
    
    /**
     * @brief Update configuration
     * 
     * @param config New configuration
     */
    void setConfig(const OutputConfig& config);
    
    /**
     * @brief Get current configuration
     * 
     * @return Current configuration
     */
    OutputConfig getConfig() const;
    
    // ========== State Machine (AC-OUT-013) ==========
    
    /**
     * @brief Get current output state
     * 
     * AC-OUT-013: STOPPED → RUNNING → STOPPED
     * 
     * @return Current state
     */
    OutputState getState() const;
    
    /**
     * @brief Get detailed output state information
     * 
     * Returns comprehensive state for testing and telemetry.
     * 
     * @return Detailed state information
     */
    OutputStateInfo getOutputState() const;
    
    // ========== Network Interface (AC-OUT-003, AC-OUT-008) ==========
    
    /**
     * @brief Get network configuration
     * 
     * Returns UDP socket and port configuration for RTP-MIDI.
     * 
     * @return Network configuration details
     */
    NetworkConfig getNetworkConfig() const;
    
    /**
     * @brief Get network statistics
     * 
     * Returns packet counts and latency metrics.
     * 
     * @return Network transmission statistics
     */
    NetworkStats getNetworkStats() const;
    
    /**
     * @brief Get MIDI statistics
     * 
     * Returns MIDI message counts and timing.
     * 
     * @return MIDI transmission statistics
     */
    MidiStats getMidiStats() const;
    
    /**
     * @brief Get last sent packet (for testing)
     * 
     * Returns a copy of the most recently transmitted RTP-MIDI packet.
     * Useful for verifying packet format in tests.
     * 
     * @return Byte vector of last packet
     */
    std::vector<uint8_t> getLastSentPacket() const;
    
    /**
     * @brief Simulate network failure (for testing)
     * 
     * Forces send operations to fail, simulating WiFi disconnection.
     * 
     * @param failed true to simulate failure, false to restore
     */
    void simulateNetworkFailure(bool failed);
    
    /**
     * @brief Simulate slow network (for testing)
     * 
     * Injects artificial latency to test timeout handling.
     * 
     * @param slow true to enable, false to disable
     * @param delay_us Artificial delay in microseconds
     */
    void simulateSlowNetwork(bool slow, uint32_t delay_us);
    
    // ========== Hardware Interface (for testing) ==========
    
    /**
     * @brief Send MIDI Real-Time message (low-level interface)
     * 
     * Sends a single System Real-Time message byte (0xF8-0xFF) via RTP-MIDI.
     * Encapsulates the message in an RTP-MIDI packet and sends over UDP.
     * 
     * AC-OUT-003: RTP-MIDI network transport (UDP over WiFi)
     * 
     * @param message MIDI Real-Time message byte
     * @return true if sent successfully, false on network failure
     */
    bool sendMIDIRealTime(uint8_t message);
    
    /**
     * @brief Set relay GPIO state
     * 
     * @param high true for HIGH (ON), false for LOW (OFF)
     */
    void setRelayGPIO(bool high);
    
    /**
     * @brief Get relay GPIO state
     * 
     * @return true if HIGH, false if LOW
     */
    bool getRelayGPIO() const;
    
    /**
     * @brief Check if relay is currently active
     * 
     * Alias for getRelayGPIO() for better test readability.
     * 
     * @return true if relay is currently ON
     */
    bool isRelayActive() const;
    
    // ========== Timer Control (AC-OUT-007, AC-OUT-008, AC-OUT-009, AC-OUT-010) ==========
    
    /**
     * @brief Start hardware timer for clock sending
     * 
     * Initializes and enables hardware timer to send MIDI clocks at 24 PPQN.
     * Timer interval calculated from current BPM.
     * 
     * AC-OUT-010: Clock interval = 60,000,000 µs / BPM / 24 PPQN
     * 
     * @return true if timer started successfully
     */
    bool startTimerClock();
    
    /**
     * @brief Stop hardware timer
     * 
     * Disables hardware timer interrupts.
     * 
     * @return true if timer stopped successfully
     */
    bool stopTimerClock();
    
    /**
     * @brief Get timer configuration
     * 
     * Returns current timer settings.
     * 
     * @return Timer configuration details
     */
    TimerConfig getTimerConfig() const;
    
    /**
     * @brief Set timer interval directly
     * 
     * Advanced control for testing. Normally use setBPM().
     * 
     * @param interval_us Interval in microseconds
     * @return true if interval set successfully
     */
    bool setTimerInterval(uint32_t interval_us);
    
    /**
     * @brief Get timer statistics
     * 
     * Returns performance metrics for monitoring.
     * 
     * AC-OUT-007: Jitter <1ms
     * AC-OUT-008: ISR execution <10µs
     * AC-OUT-009: CPU usage <3%
     * 
     * @return Timer statistics
     */
    TimerStats getTimerStats() const;
    
    /**
     * @brief Reset timer statistics
     * 
     * Clears performance counters.
     */
    void resetTimerStats();
    
    /**
     * @brief Handle timer interrupt (called from static ISR)
     * 
     * Non-static method called by static ISR wrapper.
     * Minimal work: set flag, record timestamp, increment counters.
     * 
     * AC-OUT-008: Execution time <10µs
     */
    void handleTimerInterrupt();
    
    /**
     * @brief Get clock counter (0-23)
     * 
     * Returns current position within quarter note (24 PPQN).
     * Wraps to 0 after 24 clocks.
     * 
     * @return Clock counter (0-23)
     */
    uint8_t getClockCounter() const;
    
    /**
     * @brief Timer callback for synchronized clock output
     * 
     * Called by hardware timer ISR or test framework.
     * Sends MIDI clock and handles relay timing.
     */
    void onTimerCallback();
    
    /**
     * @brief Calculate timer interval from BPM (public for testing)
     * 
     * Calculates microseconds per clock pulse based on BPM and PPQN.
     * Formula: 60,000,000 / BPM / PPQN
     * 
     * @param bpm Beats per minute
     * @param ppqn Pulses per quarter note (24 = standard)
     * @return Timer interval in microseconds
     */
    uint32_t calculateTimerInterval(uint16_t bpm, uint8_t ppqn) const;
    
    // ========== Relay Control (AC-OUT-005, AC-OUT-006, AC-OUT-012) ==========
    
    /**
     * @brief Get relay statistics
     * 
     * Returns pulse count, watchdog triggers, and timing info.
     * 
     * @return Relay statistics
     */
    RelayStats getRelayStats() const;
    
    /**
     * @brief Reset relay statistics
     * 
     * Clears counters and timing data.
     */
    void resetRelayStats();
    
    /**
     * @brief Process relay watchdog timer
     * 
     * Must be called periodically (e.g., in loop()) to check:
     * - Pulse duration completion
     * - Watchdog timeout enforcement
     * 
     * AC-OUT-005: Turns OFF relay after pulse_ms elapsed
     * AC-OUT-006: Forces OFF if exceeds watchdog_ms (stuck relay)
     */
    void processRelayWatchdog();

private:
    // Configuration
    OutputConfig config_;
    
    // State
    OutputState state_;
    float current_bpm_;
    bool syncing_;
    
    // Timing
    uint64_t last_clock_us_;        ///< Last MIDI clock timestamp (microseconds)
    uint32_t clock_interval_us_;    ///< MIDI clock interval in microseconds (24 PPQN)
    uint32_t clocks_sent_;          ///< Number of clocks sent since start
    
    // Relay state
    bool relay_on_;                 ///< True if relay is currently on
    uint64_t relay_on_time_us_;     ///< Time when relay was turned on
    
    // Network state (OUT-02)
    bool network_initialized_;      ///< UDP socket opened
    NetworkStats network_stats_;    ///< Packet transmission statistics
    std::vector<uint8_t> last_packet_;  ///< Last sent RTP-MIDI packet
    uint16_t rtp_sequence_;         ///< RTP sequence number
    uint32_t rtp_timestamp_;        ///< RTP timestamp base
    
    // Test simulation flags
    bool simulate_network_failure_; ///< Simulate network down
    bool simulate_slow_network_;    ///< Simulate latency
    uint32_t simulated_delay_us_;   ///< Artificial network delay
    
    // Timer state (OUT-03)
    bool timer_enabled_;            ///< Hardware timer is active
    uint32_t timer_interval_us_;    ///< Timer period in microseconds
    uint16_t timer_bpm_;            ///< Current BPM for timer
    uint8_t clock_counter_;         ///< Clock position within quarter note (0-23)
    TimerStats timer_stats_;        ///< Performance statistics
    uint64_t last_timer_us_;        ///< Last timer ISR timestamp
    std::vector<uint32_t> interval_samples_;  ///< For jitter calculation
    
    // Relay state (OUT-04)
    RelayStats relay_stats_;        ///< Pulse tracking statistics
    uint64_t relay_pulse_start_us_; ///< When current pulse started
    uint64_t relay_last_off_us_;    ///< When relay last turned OFF (for debounce)
    
    // MIDI statistics (OUT-05)
    MidiStats midi_stats_;          ///< MIDI message tracking
    
    // Helper methods
    void updateOutputInterval();    ///< Recalculate interval from BPM
    void processTimers();           ///< Process MIDI/relay timers (call in loop)
    void updateJitterStats();       ///< Calculate jitter from interval samples
    
    // Network helpers (OUT-02)
    bool initializeNetwork();       ///< Open UDP socket
    bool sendUDPPacket(const std::vector<uint8_t>& packet);  ///< Send raw UDP
    std::vector<uint8_t> buildRTPMIDIPacket(uint8_t midi_message);  ///< Build RTP-MIDI packet
    
    // State machine helpers
    void transitionToState(OutputState new_state);
};

#endif // OUTPUT_CONTROLLER_H
