/**
 * BPM Calculation Engine - Implementation (RED Phase)
 * 
 * @file BPMCalculation.cpp
 * @component DES-C-002: BPM Calculation Engine
 * @implements #46 REQ-F-002: Calculate BPM from tap timestamps
 * 
 * @description
 * Minimal stub implementation to make tests compile and FAIL.
 * Will be implemented in GREEN phase following TDD.
 * 
 * @phase RED - Tests should fail
 * @date 2025-11-20
 */

#include "BPMCalculation.h"
#include <cmath>

// ============================================================================
// Constructor & Initialization
// ============================================================================

BPMCalculation::BPMCalculation(ITimingProvider* timing_provider)
    : timing_(timing_provider) {
    // Constructor body empty - init() does the work
}

void BPMCalculation::init() {
    state_.init();
}

// ============================================================================
// DES-I-007: Tap Addition Interface (STUBS - will fail tests)
// ============================================================================

void BPMCalculation::addTap(uint64_t timestamp_us) {
    // GREEN: Basic implementation to pass tests
    
    // Add tap to circular buffer
    state_.tap_buffer[state_.write_index] = timestamp_us;
    state_.write_index = (state_.write_index + 1) % BPMCalculationState::MAX_TAPS;
    
    // Increment tap count (max 64)
    if (state_.tap_count < BPMCalculationState::MAX_TAPS) {
        state_.tap_count++;
    }
    
    // Calculate BPM if we have at least 2 taps
    if (state_.tap_count >= 2) {
        calculateBPM();
    }
    
    // Update last tap timestamp
    state_.last_tap_us = timestamp_us;
}

void BPMCalculation::clear() {
    // RED: Stub implementation
    state_.reset();
}

// ============================================================================
// DES-I-006: BPM Update Interface (STUBS - will fail tests)
// ============================================================================

float BPMCalculation::getBPM() const {
    // GREEN: Return calculated BPM
    return state_.current_bpm;
}

bool BPMCalculation::isStable() const {
    // GREEN: Return actual stability state
    return state_.is_stable;
}

uint8_t BPMCalculation::getTapCount() const {
    // GREEN: Return actual tap count
    return state_.tap_count;
}

float BPMCalculation::getCoefficientOfVariation() const {
    return state_.coefficient_of_variation;
}

void BPMCalculation::onBPMUpdate(std::function<void(const BPMUpdateEvent&)> callback) {
    bpm_update_callback_ = callback;
}

// ============================================================================
// Private Methods (STUBS)
// ============================================================================

void BPMCalculation::calculateBPM() {
    // GREEN: Calculate BPM from average interval
    
    // Need at least 2 taps for an interval
    if (state_.tap_count < 2) {
        state_.current_bpm = 0.0f;
        state_.is_stable = false;
        state_.coefficient_of_variation = 0.0f;
        return;
    }
    
    // Calculate average interval
    state_.average_interval_us = calculateAverageInterval();
    
    // Calculate BPM: 60,000,000 µs/min / avg_interval_us
    if (state_.average_interval_us > 0) {
        state_.current_bpm = 60000000.0f / static_cast<float>(state_.average_interval_us);
    } else {
        state_.current_bpm = 0.0f;
        state_.is_stable = false;
        state_.coefficient_of_variation = 0.0f;
        return;
    }
    
    // Calculate stability (need at least 3 taps for stddev)
    if (state_.tap_count >= 3) {
        float stddev = calculateStandardDeviation(state_.average_interval_us);
        
        // Coefficient of Variation = (stddev / mean) × 100%
        if (state_.average_interval_us > 0) {
            state_.coefficient_of_variation = (stddev / static_cast<float>(state_.average_interval_us)) * 100.0f;
        } else {
            state_.coefficient_of_variation = 0.0f;
        }
        
        // Stable if CV < 5%
        state_.is_stable = (state_.coefficient_of_variation < 5.0f);
    } else {
        // Not enough data for stability
        state_.is_stable = false;
        state_.coefficient_of_variation = 0.0f;
    }
}

uint64_t BPMCalculation::calculateAverageInterval() {
    // GREEN: Calculate average from intervals in buffer
    
    if (state_.tap_count < 2) {
        return 0;
    }
    
    // Calculate intervals between consecutive taps
    uint64_t total_interval = 0;
    uint8_t interval_count = 0;
    
    for (uint8_t i = 1; i < state_.tap_count; ++i) {
        uint64_t prev_tap = state_.tap_buffer[i - 1];
        uint64_t curr_tap = state_.tap_buffer[i];
        
        if (curr_tap > prev_tap) {
            total_interval += (curr_tap - prev_tap);
            interval_count++;
        }
    }
    
    if (interval_count > 0) {
        return total_interval / interval_count;
    }
    
    return 0;
}

float BPMCalculation::calculateStandardDeviation(uint64_t avg_interval) {
    // GREEN: Calculate standard deviation of intervals
    
    if (state_.tap_count < 3) {
        return 0.0f;  // Need at least 3 taps (2 intervals) for stddev
    }
    
    // Calculate variance: sum of squared differences from mean
    uint64_t sum_squared_diff = 0;
    uint8_t interval_count = 0;
    
    for (uint8_t i = 1; i < state_.tap_count; ++i) {
        uint64_t prev_tap = state_.tap_buffer[i - 1];
        uint64_t curr_tap = state_.tap_buffer[i];
        
        if (curr_tap > prev_tap) {
            uint64_t interval = curr_tap - prev_tap;
            
            // Calculate squared difference from mean
            int64_t diff = static_cast<int64_t>(interval) - static_cast<int64_t>(avg_interval);
            sum_squared_diff += static_cast<uint64_t>(diff * diff);
            interval_count++;
        }
    }
    
    if (interval_count < 2) {
        return 0.0f;  // Need at least 2 intervals
    }
    
    // Variance = sum_squared_diff / (n - 1)  [sample variance]
    float variance = static_cast<float>(sum_squared_diff) / static_cast<float>(interval_count - 1);
    
    // Standard deviation = sqrt(variance)
    return sqrtf(variance);
}

void BPMCalculation::detectHalfTempo(uint64_t current_interval) {
    (void)current_interval;
}

void BPMCalculation::detectDoubleTempo(uint64_t current_interval) {
    (void)current_interval;
}

void BPMCalculation::applyHalfTempoCorrection() {
    // Stub
}

void BPMCalculation::applyDoubleTempoCorrection() {
    // Stub
}

void BPMCalculation::fireBPMUpdateCallback() {
    // Stub
}
