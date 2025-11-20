/**
 * @file OutputController.cpp
 * @brief Output Controller Implementation
 * 
 * Component: DES-C-004 (#50)
 * Standards: ISO/IEC/IEEE 12207:2017
 * XP: Test-Driven Development
 */

#include "OutputController.h"

#ifdef NATIVE_BUILD
#include "mocks/time_mock.h"
#else
#include <Arduino.h>
#endif

// Constructor
OutputController::OutputController(const OutputConfig& config)
    : config_(config)
    , state_(OutputState::STOPPED)
    , current_bpm_(120.0f)
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
{
    updateOutputInterval();
    initializeNetwork();
}

// Destructor
OutputController::~OutputController() {
    stopSync();
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
    
    return sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::STOP));
}

bool OutputController::pulseRelay() {
    // RED: Stub implementation
    return false;
}

void OutputController::enableOutput(OutputMode mode) {
    config_.mode = mode;
}

bool OutputController::isEnabled() const {
    return config_.mode != OutputMode::DISABLED;
}

// ========== BPM Synchronization ==========

void OutputController::setBPM(float bpm) {
    if (bpm >= 50.0f && bpm <= 205.0f) {
        current_bpm_ = bpm;
        updateOutputInterval();
    }
}

float OutputController::getBPM() const {
    return current_bpm_;
}

void OutputController::startSync() {
    if (config_.mode != OutputMode::DISABLED && 
        config_.mode != OutputMode::RELAY_ONLY) {
        sendMIDIStart();
    }
    state_ = OutputState::RUNNING;
    syncing_ = true;
}

void OutputController::stopSync() {
    if (config_.mode != OutputMode::DISABLED && 
        config_.mode != OutputMode::RELAY_ONLY) {
        sendMIDIStop();
    }
    state_ = OutputState::STOPPED;
    syncing_ = false;
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
    // RED: Stub for GPIO control
    relay_on_ = high;
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
