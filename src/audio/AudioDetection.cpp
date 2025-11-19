/**
 * DES-C-001: Audio Detection Engine - Implementation
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * 
 * TDD Progress Tracking:
 * 
 * Cycle 1 - AC-AUDIO-001 Adaptive Threshold: RED ✅, GREEN ✅, REFACTOR ✅
 *   Tests: test_adaptive_threshold.cpp (5/5 passing)
 *   - Threshold calculation implemented in AudioDetectionState::addToWindow()
 *   - Clean, minimal implementation following YAGNI
 * 
 * Cycle 2 - AC-AUDIO-002 State Machine: RED ✅, GREEN ✅, REFACTOR ✅
 *   Tests: test_state_machine.cpp (8/8 passing)
 *   - 4-state FSM: IDLE → RISING → TRIGGERED → DEBOUNCE → IDLE
 *   - Beat event emission with kick detection (rise time > 4ms)
 *   - Fixed: Window initialization with zeros causing threshold issues
 *   - Solution: Initialize window to 2000 (12-bit ADC midpoint)
 * 
 * Cycle 3 - AC-AUDIO-003 AGC Level Transitions: RED ✅, GREEN ✅, REFACTOR ✅
 *   Tests: test_agc_transitions.cpp (8/9 passing, 1 skipped)
 *   - Clipping detection at 4000 ADC threshold
 *   - Automatic gain reduction: 60dB → 50dB → 40dB on clipping
 *   - Prevents reduction below minimum (40dB)
 *   - AGC level included in beat events
 * 
 * Cycle 4 - AC-AUDIO-004 Beat Event Emission: VALIDATION ✅ (already implemented in Cycle 2)
 *   Tests: test_beat_event_emission.cpp (12/12 passing)
 *   - Callback invocation on beat detection
 *   - Timestamp field accuracy (microsecond precision)
 *   - Amplitude field (peak ADC value)
 *   - Threshold field (current adaptive threshold)
 *   - Gain level field (AGC state: 0/1/2)
 *   - Kick-only flag (rise_time > 4ms)
 *   - Multiple beats, callback replacement, debounce validation
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#include "AudioDetection.h"

namespace clap_metronome {

// Constructor
AudioDetection::AudioDetection(ITimingProvider* timing_provider)
    : timing_(timing_provider)
    , beat_callback_(nullptr)
    , telemetry_callback_(nullptr) {
}

// ===== IBeatEventPublisher Interface =====

void AudioDetection::onBeat(BeatEventCallback callback) {
    beat_callback_ = callback;
}

bool AudioDetection::hasBeatCallback() const {
    return beat_callback_ != nullptr;
}

// ===== IAudioTelemetry Interface =====

void AudioDetection::onTelemetry(AudioTelemetryCallback callback) {
    telemetry_callback_ = callback;
}

bool AudioDetection::hasTelemetryCallback() const {
    return telemetry_callback_ != nullptr;
}

// ===== Component Lifecycle =====

bool AudioDetection::init() {
    buffers_.init();
    state_.init();
    return true;
}

void AudioDetection::processSample(uint16_t adc_value) {
    uint64_t timestamp_us = timing_->getTimestampUs();
    
    // Add sample to rolling window for threshold calculation (AC-AUDIO-001)
    state_.addToWindow(adc_value);
    
    // State machine for beat detection (AC-AUDIO-002)
    switch (state_.state) {
        case DetectionState::IDLE:
            // Check for threshold crossing
            if (adc_value > state_.threshold) {
                // Transition to RISING
                state_.state = DetectionState::RISING;
                state_.rising_edge_start_us = timestamp_us;
                state_.rising_edge_start_value = adc_value;
                state_.rising_edge_peak_value = adc_value;
            }
            break;
            
        case DetectionState::RISING:
            // Track peak value
            if (adc_value > state_.rising_edge_peak_value) {
                state_.rising_edge_peak_value = adc_value;
            }
            
            // Check if signal started falling (peak detected)
            if (adc_value < state_.rising_edge_peak_value) {
                // Transition to TRIGGERED
                state_.state = DetectionState::TRIGGERED;
                uint64_t rise_time_us = timestamp_us - state_.rising_edge_start_us;
                
                // Emit beat event (AC-AUDIO-004)
                emitBeatEvent(timestamp_us, state_.rising_edge_peak_value, rise_time_us);
                
                // Record beat timestamp for debounce
                state_.last_beat_timestamp_us = timestamp_us;
                state_.beat_count++;
            }
            break;
            
        case DetectionState::TRIGGERED:
            // Immediately transition to DEBOUNCE
            state_.state = DetectionState::DEBOUNCE;
            break;
            
        case DetectionState::DEBOUNCE:
            // Check if debounce period expired (AC-AUDIO-005: 50ms)
            if (!state_.inDebouncePeriod(timestamp_us)) {
                // Return to IDLE
                state_.state = DetectionState::IDLE;
            }
            // Else stay in DEBOUNCE (ignore samples)
            break;
    }
    
    // Update AGC based on clipping detection (AC-AUDIO-003)
    updateAGC(adc_value);
    
    // TODO: Implement telemetry in next cycle
}

// ===== State Queries =====

DetectionState AudioDetection::getState() const {
    return state_.state;
}

uint16_t AudioDetection::getThreshold() const {
    return state_.threshold;
}

AGCLevel AudioDetection::getGainLevel() const {
    return state_.gain_level;
}

uint32_t AudioDetection::getBeatCount() const {
    return state_.beat_count;
}

uint32_t AudioDetection::getFalsePositiveCount() const {
    return state_.false_positive_count;
}

// ===== Private Helper Methods =====

void AudioDetection::checkTrigger(uint16_t adc_value, uint64_t timestamp_us) {
    // TODO: Implement in GREEN phase
    (void)adc_value;
    (void)timestamp_us;
}

void AudioDetection::emitBeatEvent(uint64_t timestamp_us, uint16_t amplitude, uint64_t rise_time_us) {
    if (!beat_callback_) {
        return;  // No callback registered
    }
    
    // Create beat event (AC-AUDIO-004)
    BeatEvent event;
    event.timestamp_us = timestamp_us;
    event.amplitude = amplitude;
    event.threshold = state_.threshold;
    event.gain_level = static_cast<uint8_t>(state_.gain_level);
    
    // Kick-only detection (AC-AUDIO-006): rise time >4ms
    event.kick_only = (rise_time_us > AudioDetectionState::KICK_RISE_TIME_US);
    
    // Invoke callback
    beat_callback_(event);
}

void AudioDetection::publishTelemetry(uint64_t timestamp_us, uint16_t current_adc) {
    // TODO: Implement in GREEN phase
    (void)timestamp_us;
    (void)current_adc;
}

void AudioDetection::updateAGC(uint16_t adc_value) {
    // AC-AUDIO-003: AGC Level Transitions
    // Detect clipping and reduce gain to prevent saturation
    
    // Check if sample exceeds clipping threshold (4000 ADC units)
    if (adc_value > AudioDetectionState::CLIPPING_THRESHOLD) {
        // Clipping detected - set flag
        state_.clipping_detected = true;
        
        // Reduce gain level (if not already at minimum)
        if (state_.gain_level == AGCLevel::GAIN_60DB) {
            // Reduce from 60dB to 50dB
            state_.gain_level = AGCLevel::GAIN_50DB;
        } else if (state_.gain_level == AGCLevel::GAIN_50DB) {
            // Reduce from 50dB to 40dB
            state_.gain_level = AGCLevel::GAIN_40DB;
        }
        // If already at GAIN_40DB (minimum), stay there
    }
}

} // namespace clap_metronome
