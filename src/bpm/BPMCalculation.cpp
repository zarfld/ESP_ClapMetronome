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
    // GREEN: Add tap with interval validation and tempo correction
    
    // Validate interval (skip first tap, no interval to check)
    if (state_.last_tap_us > 0 && timestamp_us > state_.last_tap_us) {
        uint64_t interval = timestamp_us - state_.last_tap_us;
        
        // Reject intervals outside valid range (AC-BPM-013)
        if (interval < BPMCalculationState::MIN_INTERVAL_US || 
            interval > BPMCalculationState::MAX_INTERVAL_US) {
            // Invalid interval - reject tap, don't update state
            return;
        }
    }
    
    // Add tap to circular buffer
    state_.tap_buffer[state_.write_index] = timestamp_us;
    state_.write_index = (state_.write_index + 1) % BPMCalculationState::MAX_TAPS;
    
    // Increment tap count (max 64)
    if (state_.tap_count < BPMCalculationState::MAX_TAPS) {
        state_.tap_count++;
    }
    
    // Calculate BPM if we have at least 2 taps (includes tempo correction)
    if (state_.tap_count >= 2) {
        calculateBPM();
    }
    
    // Update last tap timestamp
    state_.last_tap_us = timestamp_us;
}

void BPMCalculation::clear() {
    // GREEN: Reset all BPM calculation state (AC-BPM-012)
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
    // GREEN: Calculate BPM from average interval with tempo correction
    
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
    
    // Check for tempo correction patterns (need recent intervals)
    if (state_.tap_count >= 6 && state_.last_tap_us > 0) {
        // Get last interval
        uint8_t last_idx = (state_.write_index == 0) ? (BPMCalculationState::MAX_TAPS - 1) : (state_.write_index - 1);
        uint8_t prev_idx = (last_idx == 0) ? (BPMCalculationState::MAX_TAPS - 1) : (last_idx - 1);
        uint64_t last_interval = state_.tap_buffer[last_idx] - state_.tap_buffer[prev_idx];
        
        detectHalfTempo(last_interval);
        detectDoubleTempo(last_interval);
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
    
    // Fire callback when BPM changes (AC-BPM-014)
    fireBPMUpdateCallback();
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
    // GREEN: Detect if last 5 intervals are EACH ~2× the baseline average
    (void)current_interval;  // Not used - we analyze buffer instead
    
    if (state_.tap_count < 10) {
        return;  // Need baseline + detection intervals
    }
    
    // Calculate baseline from FIRST 5 intervals (fixed window to avoid drift)
    // This ensures baseline isn't polluted by previous tempo anomalies
    uint64_t baseline_sum = 0;
    uint8_t baseline_count = 0;
    
    // Use first 6 taps (gives 5 intervals) for baseline
    for (uint8_t i = 1; i < 6 && i < state_.tap_count; ++i) {
        if (state_.tap_buffer[i] > state_.tap_buffer[i-1]) {
            baseline_sum += (state_.tap_buffer[i] - state_.tap_buffer[i-1]);
            baseline_count++;
        }
    }
    
    if (baseline_count == 0) return;
    
    uint64_t baseline_avg = baseline_sum / baseline_count;
    
    // Count how many of last 5 intervals are >= 1.8× baseline (HALF_TEMPO_RATIO)
    uint8_t slow_interval_count = 0;
    
    for (uint8_t i = state_.tap_count - 5; i < state_.tap_count; ++i) {
        if (i > 0 && state_.tap_buffer[i] > state_.tap_buffer[i-1]) {
            uint64_t interval = state_.tap_buffer[i] - state_.tap_buffer[i-1];
            float ratio = static_cast<float>(interval) / static_cast<float>(baseline_avg);
            
            if (ratio >= BPMCalculationState::HALF_TEMPO_RATIO) {
                slow_interval_count++;
            }
        }
    }
    
    // Require AT LEAST 5 consecutive slow intervals to trigger correction
    if (slow_interval_count >= BPMCalculationState::TEMPO_CORRECTION_THRESHOLD) {
        // Only apply correction once
        if (!state_.tempo_correction_applied) {
            applyHalfTempoCorrection(baseline_avg);
            state_.tempo_correction_applied = true;
        }
    } else {
        // Pattern broken - reset flag to allow future corrections
        state_.tempo_correction_applied = false;
    }
}

void BPMCalculation::detectDoubleTempo(uint64_t current_interval) {
    // GREEN: Detect if last 5 intervals are EACH ~0.5× the baseline average
    (void)current_interval;  // Not used - we analyze buffer instead
    
    if (state_.tap_count < 10) {
        return;  // Need baseline + detection intervals
    }
    
    // Calculate baseline from FIRST 5 intervals (fixed window to avoid drift)
    // This ensures baseline isn't polluted by previous tempo anomalies
    uint64_t baseline_sum = 0;
    uint8_t baseline_count = 0;
    
    // Use first 6 taps (gives 5 intervals) for baseline
    for (uint8_t i = 1; i < 6 && i < state_.tap_count; ++i) {
        if (state_.tap_buffer[i] > state_.tap_buffer[i-1]) {
            baseline_sum += (state_.tap_buffer[i] - state_.tap_buffer[i-1]);
            baseline_count++;
        }
    }
    
    if (baseline_count == 0) return;
    
    uint64_t baseline_avg = baseline_sum / baseline_count;
    
    // Count how many of last 5 intervals are <= 0.6× baseline (DOUBLE_TEMPO_RATIO)
    uint8_t fast_interval_count = 0;
    
    for (uint8_t i = state_.tap_count - 5; i < state_.tap_count; ++i) {
        if (i > 0 && state_.tap_buffer[i] > state_.tap_buffer[i-1]) {
            uint64_t interval = state_.tap_buffer[i] - state_.tap_buffer[i-1];
            float ratio = static_cast<float>(interval) / static_cast<float>(baseline_avg);
            
            if (ratio <= BPMCalculationState::DOUBLE_TEMPO_RATIO) {
                fast_interval_count++;
            }
        }
    }
    
    // Require AT LEAST 5 consecutive fast intervals to trigger correction
    if (fast_interval_count >= BPMCalculationState::TEMPO_CORRECTION_THRESHOLD) {
        // Only apply correction once
        if (!state_.tempo_correction_applied) {
            applyDoubleTempoCorrection(baseline_avg);
            state_.tempo_correction_applied = true;
        }
    } else {
        // Pattern broken - reset flag to allow future corrections
        state_.tempo_correction_applied = false;
    }
}

void BPMCalculation::applyHalfTempoCorrection(uint64_t baseline_interval_us) {
    // GREEN: Halve baseline BPM (user tapping every other beat)
    // Baseline tempo is correct, but current taps are 2× slower
    float baseline_bpm = 60000000.0f / static_cast<float>(baseline_interval_us);
    state_.current_bpm = baseline_bpm / 2.0f;
}

void BPMCalculation::applyDoubleTempoCorrection(uint64_t baseline_interval_us) {
    // GREEN: Double baseline BPM (user tapping twice as fast)
    // Baseline tempo is correct, but current taps are 2× faster
    float baseline_bpm = 60000000.0f / static_cast<float>(baseline_interval_us);
    state_.current_bpm = baseline_bpm * 2.0f;
}

void BPMCalculation::fireBPMUpdateCallback() {
    // GREEN: Fire callback if registered (AC-BPM-014)
    if (!bpm_update_callback_) {
        return; // No callback registered
    }
    
    // Create event with current state
    BPMUpdateEvent event;
    event.bpm = state_.current_bpm;
    event.is_stable = state_.is_stable;
    event.timestamp_us = state_.last_tap_us;
    event.tap_count = state_.tap_count;
    
    // Invoke callback
    bpm_update_callback_(event);
}
