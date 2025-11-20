/**
 * @file OutputController.cpp
 * @brief Output Controller Implementation
 * 
 * Component: DES-C-004 (#50)
 * Standards: ISO/IEC/IEEE 12207:2017
 * XP: Test-Driven Development
 */

#include "OutputController.h"
#include <cmath>  // For sqrtf

#ifdef NATIVE_BUILD
#include "test/mocks/time_mock.h"
#else
#include <Arduino.h>
#endif

// Constructor
OutputController::OutputController(const OutputConfig& config)
    : config_(config)
    , state_(OutputState::STOPPED)
    , current_bpm_(config.initial_bpm)
    , syncing_(false)
    , last_clock_us_(0)
    , clock_interval_us_(20833)  // 120 BPM, 24 PPQN → 20.833ms per clock
    , clocks_sent_(0)
    , relay_on_(false)
    , relay_on_time_us_(0)
    , network_initialized_(false)
    , network_stats_{}
    , rtp_sequence_(0)
    , rtp_timestamp_(0)
    , simulate_network_failure_(false)
    , simulate_slow_network_(false)
    , simulated_delay_us_(0)
    , timer_enabled_(false)
    , timer_interval_us_(0)
    , timer_bpm_(config.initial_bpm)
    , clock_counter_(0)
    , timer_stats_{}
    , last_timer_us_(0)
    , relay_stats_{}
    , relay_pulse_start_us_(0)
    , relay_last_off_us_(0)
    , midi_stats_{}
{
    updateOutputInterval();
    initializeNetwork();
    
    // Initialize timer interval
    timer_interval_us_ = calculateTimerInterval(timer_bpm_, config_.midi_ppqn);
    
    // Ensure relay starts LOW (AC-OUT-012)
    relay_on_ = false;
    relay_stats_.currently_on = false;
}

// Destructor
OutputController::~OutputController() {
    stopSync();
    
    // AC-OUT-012: Ensure relay GPIO LOW on destruction
    setRelayGPIO(false);
}

// ========== DES-I-013: Output Trigger Interface ==========

bool OutputController::sendMIDIClock() {
    if (config_.mode == OutputMode::DISABLED || 
        config_.mode == OutputMode::RELAY_ONLY) {
        return false;
    }
    
    // Check network availability
    if (simulate_network_failure_) {
        return false;
    }
    
    bool result = sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::TIMING_CLOCK));
    if (result) {
        clocks_sent_++;
        midi_stats_.clock_messages_sent++;
        midi_stats_.last_message_us = micros();
    }
    return result;
}

bool OutputController::sendMIDIStart() {
    if (config_.mode == OutputMode::DISABLED || 
        config_.mode == OutputMode::RELAY_ONLY) {
        return false;
    }
    
    if (!config_.midi_send_start_stop) {
        return false;
    }
    
    // Check network availability
    if (simulate_network_failure_) {
        return false;
    }
    
    bool result = sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::START));
    if (result) {
        clocks_sent_ = 0;
        midi_stats_.start_messages_sent++;
        midi_stats_.last_message_us = micros();
    }
    return result;
}

bool OutputController::sendMIDIStop() {
    if (config_.mode == OutputMode::DISABLED || 
        config_.mode == OutputMode::RELAY_ONLY) {
        return false;
    }
    
    if (!config_.midi_send_start_stop) {
        return false;
    }
    
    // Check network availability
    if (simulate_network_failure_) {
        return false;
    }
    
    bool result = sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::STOP));
    if (result) {
        midi_stats_.stop_messages_sent++;
        midi_stats_.last_message_us = micros();
    }
    return result;
}

bool OutputController::pulseRelay() {
    // Check if relay is enabled
    if (config_.mode == OutputMode::DISABLED || 
        config_.mode == OutputMode::MIDI_ONLY) {
        return false;
    }
    
    // Check debounce period (AC-OUT-005)
    uint64_t current_us = micros();
    if (relay_last_off_us_ > 0) {
        uint64_t time_since_off = current_us - relay_last_off_us_;
        if (time_since_off < static_cast<uint64_t>(config_.relay_debounce_ms * 1000)) {
            relay_stats_.debounce_rejects++;
            return false;  // Too soon, debounce period not elapsed
        }
    }
    
    // Trigger pulse
    setRelayGPIO(true);
    relay_stats_.pulse_count++;
    relay_stats_.last_pulse_us = current_us;
    
    return true;
}

void OutputController::enableOutput(OutputMode mode) {
    config_.mode = mode;
}

bool OutputController::isEnabled() const {
    return config_.mode != OutputMode::DISABLED;
}

// ========== BPM Synchronization ==========

float OutputController::getBPM() const {
    return current_bpm_;
}

bool OutputController::startSync() {
    if (config_.mode != OutputMode::DISABLED && 
        config_.mode != OutputMode::RELAY_ONLY) {
        if (!sendMIDIStart()) {
            return false;
        }
    }
    
    // Start timer-based clock sending
    if (!startTimerClock()) {
        return false;
    }
    
    state_ = OutputState::RUNNING;
    syncing_ = true;
    return true;
}

bool OutputController::startSync(uint16_t bpm) {
    setBPM(bpm);
    return startSync();
}

void OutputController::updateBPM(uint16_t bpm) {
    if (bpm < 40 || bpm > 240) return;  // Validate range
    
    current_bpm_ = static_cast<float>(bpm);
    timer_bpm_ = bpm;
    
    // Recalculate timer interval
    timer_interval_us_ = calculateTimerInterval(bpm, config_.midi_ppqn);
    timer_stats_.interval_us = timer_interval_us_;
    
    // Update clock interval
    updateOutputInterval();
}

bool OutputController::stopSync() {
    // Stop timer-based clock sending
    stopTimerClock();
    
    if (config_.mode != OutputMode::DISABLED && 
        config_.mode != OutputMode::RELAY_ONLY) {
        if (!sendMIDIStop()) {
            return false;
        }
    }
    
    state_ = OutputState::STOPPED;
    syncing_ = false;
    return true;
}

bool OutputController::isSyncing() const {
    return syncing_;
}

// ========== Configuration ==========

void OutputController::setConfig(const OutputConfig& config) {
    config_ = config;
    updateOutputInterval();
}

OutputConfig OutputController::getConfig() const {
    return config_;
}

// ========== State Machine ==========

OutputState OutputController::getState() const {
    return state_;
}

OutputStateInfo OutputController::getOutputState() const {
    OutputStateInfo info;
    info.midi_enabled = (config_.mode == OutputMode::MIDI_ONLY || 
                         config_.mode == OutputMode::BOTH);
    info.relay_enabled = (config_.mode == OutputMode::RELAY_ONLY || 
                          config_.mode == OutputMode::BOTH);
    info.syncing = syncing_;
    info.clocks_sent = clocks_sent_;
    info.last_clock_time_ms = static_cast<uint32_t>(last_clock_us_ / 1000);
    info.current_bpm = current_bpm_;
    info.mode = config_.mode;
    return info;
}

// ========== Network Interface (OUT-02) ==========

NetworkConfig OutputController::getNetworkConfig() const {
    NetworkConfig config;
    config.socket_open = network_initialized_;
    config.local_port = config_.rtp_port;
    config.data_port = config_.rtp_port + 1;  // Data port = control + 1
    config.protocol = NetworkProtocol::UDP;
    return config;
}

NetworkStats OutputController::getNetworkStats() const {
    return network_stats_;
}

std::vector<uint8_t> OutputController::getLastSentPacket() const {
    return last_packet_;
}

void OutputController::simulateNetworkFailure(bool failed) {
    simulate_network_failure_ = failed;
}

void OutputController::simulateSlowNetwork(bool slow, uint32_t delay_us) {
    simulate_slow_network_ = slow;
    simulated_delay_us_ = delay_us;
}

// ========== Hardware Interface ==========

bool OutputController::sendMIDIRealTime(uint8_t message) {
    // OUT-02: Build and send RTP-MIDI packet
    std::vector<uint8_t> packet = buildRTPMIDIPacket(message);
    return sendUDPPacket(packet);
}

void OutputController::setRelayGPIO(bool high) {
    relay_on_ = high;
    relay_stats_.currently_on = high;
    
    if (high) {
        // Starting new pulse
        relay_pulse_start_us_ = micros();
    }
    
#ifndef NATIVE_BUILD
    // Actual GPIO control for hardware
    // digitalWrite(RELAY_GPIO_PIN, high ? HIGH : LOW);
#endif
}

bool OutputController::getRelayGPIO() const {
    return relay_on_;
}

// ========== Private Helpers ==========

void OutputController::updateOutputInterval() {
    // Calculate MIDI clock interval in microseconds from BPM
    // 24 clocks per quarter note (MIDI standard)
    // Interval = 60 seconds / BPM / 24 PPQN = 2,500,000 µs / BPM
    // Example: 120 BPM → 2,500,000 / 120 = 20,833 µs per clock
    clock_interval_us_ = static_cast<uint32_t>(2500000.0f / current_bpm_);
}

// ========== Network Helpers (OUT-02) ==========

bool OutputController::initializeNetwork() {
    // OUT-02 RED: Stub - will open UDP socket
    network_initialized_ = true;
    return true;
}

bool OutputController::sendUDPPacket(const std::vector<uint8_t>& packet) {
    // OUT-02: Send via UDP with timeout enforcement
    
    // Test simulation: network failure
    if (simulate_network_failure_) {
        network_stats_.send_failures++;
        return false;
    }
    
    // Measure send latency
    uint64_t start_us = micros();
    
    // Test simulation: slow network (enforce timeout)
    if (simulate_slow_network_) {
        // Check if we would hit timeout (AC-OUT-008: <10ms)
        if (simulated_delay_us_ > 10000) {
            // Advance time to timeout point (for testing)
            advance_time_us(10000);
            network_stats_.send_failures++;
            return false;
        }
        // Simulate actual delay (up to timeout limit)
        advance_time_us(simulated_delay_us_);
    }
    
    // Store last packet for test inspection
    last_packet_ = packet;
    
    // Calculate actual latency
    uint64_t end_us = micros();
    network_stats_.last_latency_us = static_cast<uint32_t>(end_us - start_us);
    
    // Update stats
    network_stats_.packets_sent++;
    network_stats_.bytes_sent += static_cast<uint32_t>(packet.size());
    
    return true;
}

std::vector<uint8_t> OutputController::buildRTPMIDIPacket(uint8_t midi_message) {
    // OUT-02 RED: Stub - will build proper RFC 6295 packet
    std::vector<uint8_t> packet;
    
    // RTP Header (12 bytes minimum)
    // Byte 0: V=2, P=0, X=0, CC=0
    packet.push_back(0x80);  // V=2 (10 in binary), rest zeros
    
    // Byte 1: M=0, PT=97 (MIDI over RTP)
    packet.push_back(97);
    
    // Bytes 2-3: Sequence number
    packet.push_back((rtp_sequence_ >> 8) & 0xFF);
    packet.push_back(rtp_sequence_ & 0xFF);
    rtp_sequence_++;
    
    // Bytes 4-7: RTP timestamp (32-bit, microseconds)
    uint32_t timestamp = static_cast<uint32_t>(micros() & 0xFFFFFFFF);
    packet.push_back((timestamp >> 24) & 0xFF);
    packet.push_back((timestamp >> 16) & 0xFF);
    packet.push_back((timestamp >> 8) & 0xFF);
    packet.push_back(timestamp & 0xFF);
    
    // Bytes 8-11: SSRC (synchronization source identifier)
    // Stub: use fixed SSRC
    packet.push_back(0x12);
    packet.push_back(0x34);
    packet.push_back(0x56);
    packet.push_back(0x78);
    
    // MIDI Command Section (RFC 6295 Section 3)
    // System Real-Time messages are single-byte
    packet.push_back(midi_message);
    
    return packet;
}

void OutputController::processTimers() {
    // RED: Stub for timer processing
}

void OutputController::transitionToState(OutputState new_state) {
    state_ = new_state;
}

// ========== Timer Control (OUT-03 RED) ==========

bool OutputController::startTimerClock() {
    // RED: Stub - will initialize hardware timer
    timer_enabled_ = true;
    return true;
}

bool OutputController::stopTimerClock() {
    // RED: Stub - will disable hardware timer
    timer_enabled_ = false;
    return true;
}

bool OutputController::setBPM(uint16_t bpm) {
    // RED: Stub - will reconfigure timer interval
    timer_bpm_ = bpm;
    timer_interval_us_ = calculateTimerInterval(bpm, config_.midi_ppqn);
    return true;
}

TimerConfig OutputController::getTimerConfig() const {
    TimerConfig config;
    config.interval_us = timer_interval_us_;
    config.enabled = timer_enabled_;
    config.bpm = timer_bpm_;
    config.ppqn = config_.midi_ppqn;
    return config;
}

bool OutputController::setTimerInterval(uint32_t interval_us) {
    // RED: Stub - will set timer interval directly
    timer_interval_us_ = interval_us;
    return true;
}

TimerStats OutputController::getTimerStats() const {
    return timer_stats_;
}

void OutputController::resetTimerStats() {
    timer_stats_ = TimerStats{};
    interval_samples_.clear();
}

void OutputController::handleTimerInterrupt() {
    // RED: Stub - will handle ISR
    uint64_t isr_start = micros();
    
    // Increment interrupt counter
    timer_stats_.total_interrupts++;
    
    // Send MIDI clock
    if (sendMIDIClock()) {
        timer_stats_.clocks_sent++;
        
        // Increment clock counter (wraps at 24 PPQN)
        clock_counter_++;
        if (clock_counter_ >= config_.midi_ppqn) {
            clock_counter_ = 0;
        }
    } else {
        timer_stats_.missed_clocks++;
    }
    
    // Record timestamp for jitter calculation
    uint64_t current_us = micros();
    if (last_timer_us_ > 0) {
        uint32_t actual_interval = static_cast<uint32_t>(current_us - last_timer_us_);
        interval_samples_.push_back(actual_interval);
        
        // Keep last 100 samples for jitter calculation
        if (interval_samples_.size() > 100) {
            interval_samples_.erase(interval_samples_.begin());
        }
        
        updateJitterStats();
    }
    last_timer_us_ = current_us;
    
    // Measure ISR execution time
    uint64_t isr_end = micros();
    uint32_t isr_time = static_cast<uint32_t>(isr_end - isr_start);
    
    // Update ISR time stats
    if (timer_stats_.total_interrupts == 1) {
        timer_stats_.avg_isr_time_us = isr_time;
        timer_stats_.max_isr_time_us = isr_time;
    } else {
        // Running average
        timer_stats_.avg_isr_time_us = 
            (timer_stats_.avg_isr_time_us * (timer_stats_.total_interrupts - 1) + isr_time) 
            / timer_stats_.total_interrupts;
        
        if (isr_time > timer_stats_.max_isr_time_us) {
            timer_stats_.max_isr_time_us = isr_time;
        }
    }
}

uint8_t OutputController::getClockCounter() const {
    return clock_counter_;
}

uint32_t OutputController::calculateTimerInterval(uint16_t bpm, uint8_t ppqn) const {
    // Calculate interval in microseconds:
    // 60 seconds/minute × 1,000,000 µs/second ÷ BPM ÷ PPQN
    // = 60,000,000 / BPM / PPQN
    
    if (bpm == 0 || ppqn == 0) {
        return 20833;  // Default: 120 BPM, 24 PPQN
    }
    
    uint32_t interval = 60000000UL / bpm / ppqn;
    return interval;
}

void OutputController::updateJitterStats() {
    if (interval_samples_.empty()) {
        timer_stats_.jitter_ms = 0.0f;
        return;
    }
    
    // Calculate mean
    uint64_t sum = 0;
    for (uint32_t sample : interval_samples_) {
        sum += sample;
    }
    float mean = static_cast<float>(sum) / interval_samples_.size();
    
    // Calculate standard deviation
    float variance = 0.0f;
    for (uint32_t sample : interval_samples_) {
        float diff = static_cast<float>(sample) - mean;
        variance += diff * diff;
    }
    variance /= interval_samples_.size();
    
    float std_dev_us = sqrtf(variance);
    timer_stats_.jitter_ms = std_dev_us / 1000.0f;  // Convert to milliseconds
}

// ========== Relay Control (OUT-04 RED) ==========

RelayStats OutputController::getRelayStats() const {
    return relay_stats_;
}

void OutputController::resetRelayStats() {
    relay_stats_ = RelayStats{};
}

MidiStats OutputController::getMidiStats() const {
    return midi_stats_;
}

void OutputController::onTimerCallback() {
    // Send MIDI clock if enabled
    if (syncing_ && (config_.mode == OutputMode::MIDI_ONLY || config_.mode == OutputMode::BOTH)) {
        sendMIDIClock();
    }
    
    // Update clock counter
    clock_counter_++;
    if (clock_counter_ >= config_.midi_ppqn) {
        clock_counter_ = 0;
    }
    
    // Track timing for jitter calculation
    uint64_t current_us = micros();
    if (last_timer_us_ > 0) {
        uint32_t interval = static_cast<uint32_t>(current_us - last_timer_us_);
        interval_samples_.push_back(interval);
        
        // Keep only last 100 samples
        if (interval_samples_.size() > 100) {
            interval_samples_.erase(interval_samples_.begin());
        }
        
        updateJitterStats();
    }
    last_timer_us_ = current_us;
    
    timer_stats_.total_interrupts++;
    timer_stats_.callbacks_processed = timer_stats_.total_interrupts;  // Keep in sync
    timer_stats_.clocks_sent++;
}

void OutputController::processRelayWatchdog() {
    if (!relay_on_) {
        return;  // Nothing to process
    }
    
    uint64_t current_us = micros();
    uint64_t elapsed_us = current_us - relay_pulse_start_us_;
    
    // Check if pulse duration completed
    if (elapsed_us >= static_cast<uint64_t>(config_.relay_pulse_ms * 1000)) {
        setRelayGPIO(false);
        relay_last_off_us_ = current_us;
        return;
    }
    
    // Check watchdog timeout (AC-OUT-006)
    if (elapsed_us >= static_cast<uint64_t>(config_.relay_watchdog_ms * 1000)) {
        // Watchdog fired - force OFF
        setRelayGPIO(false);
        relay_stats_.watchdog_triggers++;
        relay_last_off_us_ = current_us;
        transitionToState(OutputState::WATCHDOG);
    }
}
