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

#ifdef ARDUINO
#include <Arduino.h>  // For Serial debug output on hardware
#endif

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
        
        // If locked tempo exists, check if beat matches rhythmic pattern
        // Don't reject beats, but track mismatches to detect tempo changes
        if (state_.locked_bpm > 0.0f) {
            uint64_t expected_interval = (uint64_t)(60000000.0f / state_.locked_bpm);
            float ratio = (float)interval / (float)expected_interval;
            
            // Check against common note subdivisions (10% tolerance each)
            bool matches_pattern = 
                (ratio >= 0.90f && ratio <= 1.10f) ||      // 1× quarter
                (ratio >= 0.45f && ratio <= 0.55f) ||      // 0.5× eighth
                (ratio >= 1.80f && ratio <= 2.20f) ||      // 2× half
                (ratio >= 0.60f && ratio <= 0.73f) ||      // ⅔× triplet
                (ratio >= 0.225f && ratio <= 0.275f) ||    // 0.25× sixteenth
                (ratio >= 0.30f && ratio <= 0.37f);        // ⅓× triplet eighth
            
            if (!matches_pattern) {
                // Beat doesn't match locked pattern - count consecutive mismatches
                if (state_.pattern_mismatch_count < 255) {
                    state_.pattern_mismatch_count++;
                }
                
                // Force unlock after consecutive mismatches (different tempo being played)
                if (state_.pattern_mismatch_count >= BPMCalculationState::PATTERN_MISMATCH_UNLOCK) {
                    state_.locked_bpm = 0.0f;
                    state_.stable_count = 0;
                    state_.pattern_mismatch_count = 0;
                    state_.tap_count = 0;  // Clear buffer for fresh start
                    state_.write_index = 0;
                    state_.last_tap_us = 0;  // Reset last tap
                    // Zero out buffer to prevent stale timestamps
                    for (uint8_t i = 0; i < BPMCalculationState::MAX_TAPS; ++i) {
                        state_.tap_buffer[i] = 0;
                    }
                    // Don't add this mismatched tap - let next tap start fresh
                    return;
                }
            } else {
                // Beat matches pattern - reset mismatch counter
                state_.pattern_mismatch_count = 0;
            }
        }
    }
    
    // Add tap to circular buffer
    state_.tap_buffer[state_.write_index] = timestamp_us;
    state_.write_index = (state_.write_index + 1) % BPMCalculationState::MAX_TAPS;
    
    // Increment tap count (max 64)
    if (state_.tap_count < BPMCalculationState::MAX_TAPS) {
        state_.tap_count++;
    }
    
    // Update shadow tracker (runs in parallel, independent of primary)
    updateShadowTracker(timestamp_us);
    
    // Calculate BPM if we have at least MIN_TAPS_BASIC (need 2 intervals minimum)
    // For evenly-spaced: MIN_TAPS_EVEN (4), for subdivisions: MIN_TAPS_SUBDIVISION (8)
    if (state_.tap_count >= BPMCalculationState::MIN_TAPS_BASIC) {
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
    // Return locked tempo if available (stable metronome behavior)
    // Otherwise return calculated BPM
    return (state_.locked_bpm > 0.0f) ? state_.locked_bpm : state_.current_bpm;
}

float BPMCalculation::getLockedBPM() const {
    return state_.locked_bpm;
}

float BPMCalculation::getCalculatedBPM() const {
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

float BPMCalculation::getRelativeDeviation() const {
    return state_.relative_deviation_percent;
}

uint8_t BPMCalculation::getStableCount() const {
    return state_.stable_count;
}

void BPMCalculation::checkTimeout(uint64_t current_time_us) {
    // Check for lock timeout - unlock if no beats for 3 seconds (applies to both locked and locking states)
    if (state_.current_bpm > 0.0f && state_.last_tap_us > 0) {
        uint64_t time_since_last_beat = current_time_us - state_.last_tap_us;
        if (time_since_last_beat > BPMCalculationState::LOCK_TIMEOUT_US) {
            // No beats for too long - unlock tempo BUT keep locked_bpm for validation fallback
            // This allows the system to return to the same tempo after a pause
            // state_.locked_bpm preserved intentionally - only cleared on manual reset
            state_.stable_count = 0;    // Reset stability - need to re-lock
            state_.tap_count = 0;       // Clear tap buffer - start fresh with new beats
            state_.write_index = 0;     // Reset buffer position
            state_.last_tap_us = 0;     // Clear last tap to allow first beat after timeout
            // Zero out buffer to prevent mixing old timestamps with new ones
            // This is safe because tap_count=0 means no reads until new beats arrive
            for (uint8_t i = 0; i < BPMCalculationState::MAX_TAPS; ++i) {
                state_.tap_buffer[i] = 0;
            }
            // Fire callback to notify unlock (locked_bpm still set, so shows "[tracking]")
            fireBPMUpdateCallback();
        }
    }
}

void BPMCalculation::onBPMUpdate(std::function<void(const BPMUpdateEvent&)> callback) {
    bpm_update_callback_ = callback;
}

// ============================================================================
// Private Methods (STUBS)
// ============================================================================

void BPMCalculation::calculateBPM() {
    // GREEN: Calculate BPM from average interval with tempo correction
    
    // Adaptive minimum taps based on rhythm pattern:
    // - Evenly-spaced (quarter notes): MIN_TAPS_EVEN (4) - simple, consistent rhythm
    // - Subdivisions (eighth notes): MIN_TAPS_SUBDIVISION (8) - need more samples for pattern detection
    // - Absolute minimum: MIN_TAPS_BASIC (2) - for initial detection (1 interval)
    if (state_.tap_count < BPMCalculationState::MIN_TAPS_BASIC) {
#ifdef ARDUINO
        Serial.print("[BPM-DEBUG] tap_count=");
        Serial.print(state_.tap_count);
        Serial.print(" < MIN_TAPS_BASIC (");
        Serial.print(BPMCalculationState::MIN_TAPS_BASIC);
        Serial.println("), skipping BPM calc");
#endif
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
        
        // Validate BPM range (prevent false positives from noise or bad rhythm)
        if (state_.current_bpm < BPMCalculationState::MIN_BPM || 
            state_.current_bpm > BPMCalculationState::MAX_BPM) {
            // Invalid tempo - ignore this calculation
            // If we have a locked tempo, keep it; otherwise keep current_bpm as-is for next calculation
            if (state_.locked_bpm > 0.0f) {
                state_.current_bpm = state_.locked_bpm;  // Keep locked tempo when available
            }
            // If unlocked (locked_bpm == 0), leave current_bpm unchanged so next beat can try again
            state_.is_stable = false;
            state_.coefficient_of_variation = 100.0f;  // Mark as unstable
            state_.stable_count = 0;
            return;
        }
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
    
    // Calculate stability (need MIN_TAPS_SUBDIVISION beats for reliable stddev with mixed subdivisions)
    // With fewer beats, subdivision patterns create false instability
    bool currently_stable = false;  // Declare here for use in tempo locking logic
    if (state_.tap_count >= BPMCalculationState::MIN_TAPS_SUBDIVISION) {
        float stddev = calculateStandardDeviation(state_.average_interval_us);
        
        // Coefficient of Variation = (stddev / mean) × 100%
        if (state_.average_interval_us > 0) {
            state_.coefficient_of_variation = (stddev / static_cast<float>(state_.average_interval_us)) * 100.0f;
        } else {
            state_.coefficient_of_variation = 0.0f;
        }
        
        // Stable if CV < 5%
        currently_stable = (state_.coefficient_of_variation < BPMCalculationState::STABILITY_CV_THRESHOLD);
        
        // Increment or reset stable counter ONLY when not locked
        // When locked, we use deviation counting instead (see below)
        if (state_.locked_bpm == 0.0f) {
            if (currently_stable) {
                if (state_.stable_count < 255) {
                    state_.stable_count++;
                }
            } else {
                state_.stable_count = 0;
            }
        }
        
        state_.is_stable = currently_stable;
    } else {
        // Not enough data for stability - need MIN_TAPS beats minimum
        state_.is_stable = false;
        state_.coefficient_of_variation = 0.0f;
        if (state_.locked_bpm == 0.0f) {  // Only reset when not locked
            state_.stable_count = 0;
        }
    }
    
    // Check if shadow tracker suggests tempo change (high confidence, different tempo)
    if (state_.locked_bpm > 0.0f && state_.shadow_confidence >= BPMCalculationState::SHADOW_CONFIDENCE_THRESHOLD) {
        float shadow_diff = (state_.shadow_bpm > state_.locked_bpm) 
                           ? (state_.shadow_bpm - state_.locked_bpm)
                           : (state_.locked_bpm - state_.shadow_bpm);
        
        if (shadow_diff >= BPMCalculationState::SHADOW_SWITCH_MIN_BPM_DIFF) {
            // Shadow tracker has high confidence in different tempo - force unlock
            state_.locked_bpm = 0.0f;
            state_.stable_count = 0;
        }
    }
    
    // Tempo locking logic (metronome behavior)
    if (state_.locked_bpm == 0.0f) {
        // No locked tempo yet - require stable readings AND sufficient time duration
        // CV now calculated on normalized intervals, so stability is meaningful
        state_.relative_deviation_percent = 0.0f;  // No locked tempo = no deviation
        
        // Need MIN_TAPS beats covering at least 2 bars with stable tempo (CV < 5%)
        if (state_.current_bpm >= BPMCalculationState::MIN_BPM && 
            state_.current_bpm <= BPMCalculationState::MAX_BPM &&
            state_.tap_count >= BPMCalculationState::MIN_TAPS_SUBDIVISION &&
            state_.stable_count >= BPMCalculationState::STABLE_COUNT_LOCK) {
            
            // Calculate time span of buffer (oldest to newest tap)
            uint8_t oldest_idx = (state_.write_index + BPMCalculationState::MAX_TAPS - state_.tap_count) % BPMCalculationState::MAX_TAPS;
            uint8_t newest_idx = (state_.write_index == 0) ? (BPMCalculationState::MAX_TAPS - 1) : (state_.write_index - 1);
            uint64_t time_span_us = state_.tap_buffer[newest_idx] - state_.tap_buffer[oldest_idx];
            
            // Minimum time = 2 bars = 8 quarter notes = (2 * 4 * 60) / BPM seconds
            static constexpr float BARS_FOR_LOCK = 2.0f;
            static constexpr float BEATS_PER_BAR = 4.0f;
            uint64_t min_time_us = (uint64_t)((BARS_FOR_LOCK * BEATS_PER_BAR * 60000000.0f) / state_.current_bpm);
            
            if (time_span_us >= min_time_us) {
                // Have sufficient time duration - LOCK tempo
                state_.locked_bpm = state_.current_bpm;
                // Initialize deviation counter (used to detect tempo changes)
                state_.stable_count = BPMCalculationState::DEVIATION_COUNT_UNLOCK;
            }
        }
    } else {
        // Have locked tempo - validate calculated BPM against locked value
        float tolerance = state_.locked_bpm * (BPMCalculationState::TEMPO_TOLERANCE_PERCENT / 100.0f);
        float deviation = (state_.current_bpm > state_.locked_bpm) 
                         ? (state_.current_bpm - state_.locked_bpm)
                         : (state_.locked_bpm - state_.current_bpm);
        
        // Calculate relative deviation percentage for telemetry
        state_.relative_deviation_percent = (deviation / state_.locked_bpm) * 100.0f;
        
        if (deviation <= tolerance) {
            // Within tolerance - keep locked tempo stable, don't update
            // This is the key: once locked, we HOLD the tempo unless clearly changed
            // Reset deviation counter (stable_count used as deviation counter when locked)
            state_.stable_count = BPMCalculationState::DEVIATION_COUNT_UNLOCK;
        } else {
            // Deviation exceeds tolerance - possible tempo change
            // Decrement deviation counter (requires DEVIATION_COUNT_UNLOCK consecutive deviations to unlock)
            if (state_.stable_count > 0) {
                state_.stable_count--;
            }
            
            // Unlock after consecutive deviations exhaust the counter
            if (state_.stable_count == 0) {
                state_.locked_bpm = 0.0f;  // Unlock
                state_.stable_count = 0;   // Reset counter for re-locking
                // Clear tap buffer to start fresh with new tempo
                state_.tap_count = 0;
                state_.write_index = 0;
                // Zero out buffer to prevent stale timestamps
                for (uint8_t i = 0; i < BPMCalculationState::MAX_TAPS; ++i) {
                    state_.tap_buffer[i] = 0;
                }
                // Will re-lock when new tempo shows STABLE_COUNT_LOCK consecutive stable readings
            }
        }
    }
    
    // Fire callback when BPM changes (AC-BPM-014)
    fireBPMUpdateCallback();
}

uint64_t BPMCalculation::calculateAverageInterval() {
    // Calculate base beat interval using rhythmic quantization
    // Handles mixed note values (quarter, 8th, 16th notes)
    
    if (state_.tap_count < 2) {
        return 0;
    }
    
    // Step 1: Calculate all intervals from circular buffer
    uint64_t intervals[BPMCalculationState::MAX_TAPS];
    uint8_t interval_count = 0;
    
    // Use only RECENT taps for interval calculation (sliding window approach)
    // This allows system to adapt to tempo changes and prevents old data from dominating
    const uint8_t RECENT_TAPS = 16;  // Use last 16 taps max
    uint8_t taps_to_use = (state_.tap_count < RECENT_TAPS) ? state_.tap_count : RECENT_TAPS;
    
    // Calculate starting index for recent taps
    uint8_t oldest_idx = (state_.write_index + BPMCalculationState::MAX_TAPS - taps_to_use) % BPMCalculationState::MAX_TAPS;
    
    for (uint8_t i = 1; i < taps_to_use; ++i) {
        uint8_t prev_idx = (oldest_idx + i - 1) % BPMCalculationState::MAX_TAPS;
        uint8_t curr_idx = (oldest_idx + i) % BPMCalculationState::MAX_TAPS;
        
        uint64_t prev_tap = state_.tap_buffer[prev_idx];
        uint64_t curr_tap = state_.tap_buffer[curr_idx];
        
        // Skip if timestamps are invalid or out of order
        // Note: timestamp 0 is valid (tests start at T=0), so only skip if curr <= prev
        if (curr_tap <= prev_tap) {
            continue;
        }
        
        uint64_t interval = curr_tap - prev_tap;
        // Sanity check: reject intervals outside reasonable BPM range (30-600 BPM)
        // This prevents using stale data that creates huge intervals
        if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
            interval <= BPMCalculationState::MAX_INTERVAL_US) {
            intervals[interval_count++] = interval;
        }
    }
    
    if (interval_count == 0) {
        return 0;
    }
    
    // Step 1.5: Filter intervals - DISABLED FOR TESTING
    // The sliding window (RECENT_TAPS=16) above already limits data to recent beats
    // Additional filtering may be too aggressive
    if (false && interval_count >= BPMCalculationState::MIN_TAPS_SUBDIVISION) {
        // Use sliding window: last 12 intervals (or all if fewer)
        const uint8_t WINDOW_SIZE = 12;
        uint8_t window_start = (interval_count > WINDOW_SIZE) ? (interval_count - WINDOW_SIZE) : 0;
        uint8_t window_count = interval_count - window_start;
        
        // Step 1: Calculate median from sliding window only
        uint64_t sorted[BPMCalculationState::MAX_TAPS];
        for (uint8_t i = 0; i < window_count; ++i) {
            sorted[i] = intervals[window_start + i];
        }
        
        // Bubble sort the window
        for (uint8_t i = 0; i < window_count - 1; ++i) {
            for (uint8_t j = 0; j < window_count - i - 1; ++j) {
                if (sorted[j] > sorted[j + 1]) {
                    uint64_t temp = sorted[j];
                    sorted[j] = sorted[j + 1];
                    sorted[j + 1] = temp;
                }
            }
        }
        
        uint64_t median = sorted[window_count / 2];
        
        // Step 2: Filter intervals - keep only those within 50% of median
        // Increased tolerance to allow tempo changes while still rejecting noise
        const float TOLERANCE = 0.50f;  // 50% tolerance
        uint64_t min_interval = static_cast<uint64_t>(median * (1.0f - TOLERANCE));
        uint64_t max_interval = static_cast<uint64_t>(median * (1.0f + TOLERANCE));
        
        uint64_t filtered[BPMCalculationState::MAX_TAPS];
        uint8_t filtered_count = 0;
        
        // Filter the ENTIRE interval array (not just window)
        for (uint8_t i = 0; i < interval_count; ++i) {
            if (intervals[i] >= min_interval && intervals[i] <= max_interval) {
                filtered[filtered_count++] = intervals[i];
            }
        }
        
        // Replace original intervals with filtered set (if we have enough)
        if (filtered_count >= 3) {
            for (uint8_t i = 0; i < filtered_count; ++i) {
                intervals[i] = filtered[i];
            }
            interval_count = filtered_count;
        }
    }
    
#ifdef ARDUINO
    // DEBUG: Print first 5 intervals to understand what's being calculated
    if (state_.tap_count >= BPMCalculationState::MIN_TAPS_SUBDIVISION) {
        Serial.print("[DEBUG] Intervals (us): ");
        uint8_t debug_count = (interval_count < 5) ? interval_count : 5;
        for (uint8_t i = 0; i < debug_count; ++i) {
            Serial.print(intervals[i] / 1000);
            Serial.print("ms ");
        }
        Serial.print("(count=");
        Serial.print(interval_count);
        Serial.println(")");
    }
#endif
    
    // Step 2: Check if we have enough taps for subdivision detection
    // For evenly-spaced rhythms (MIN_TAPS_EVEN=4), skip subdivision detection
    // For complex rhythms (MIN_TAPS_SUBDIVISION=8), apply subdivision detection
    const bool can_detect_subdivisions = (state_.tap_count >= BPMCalculationState::MIN_TAPS_SUBDIVISION) && 
                                         (interval_count >= 5);
    
    if (!can_detect_subdivisions) {
        // Not enough data for subdivision detection - use simple average
        // This handles evenly-spaced quarter note patterns efficiently
        uint64_t sum = 0;
        for (uint8_t i = 0; i < interval_count; ++i) {
            sum += intervals[i];
        }
        return sum / interval_count;
    }
    
    // Step 3: Calculate median and check for subdivision pattern
    // Sort intervals to find median (robust to outliers)
    uint64_t sorted[BPMCalculationState::MAX_TAPS];
    for (uint8_t i = 0; i < interval_count; ++i) {
        sorted[i] = intervals[i];
    }
    
    // Bubble sort
    for (uint8_t i = 0; i < interval_count - 1; ++i) {
        for (uint8_t j = 0; j < interval_count - i - 1; ++j) {
            if (sorted[j] > sorted[j + 1]) {
                uint64_t temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    // Use median instead of average (more robust to outliers)
    uint64_t median_interval = sorted[interval_count / 2];
    
    // Check if MOST intervals are suspiciously similar (within 40% of median)
    // This suggests user is tapping on EVERY beat (downbeat + upbeat)
    uint8_t similar_count = 0;
    uint64_t sum_similar = 0;
    uint64_t min_similar = static_cast<uint64_t>(median_interval * 0.6f);
    uint64_t max_similar = static_cast<uint64_t>(median_interval * 1.4f);
    
    for (uint8_t i = 0; i < interval_count; ++i) {
        if (intervals[i] >= min_similar && intervals[i] <= max_similar) {
            similar_count++;
            sum_similar += intervals[i];
        }
    }
    
    // Check similarity ratio: >70% means consistent rhythm pattern
    float similar_ratio = static_cast<float>(similar_count) / static_cast<float>(interval_count);
    
    // Detect evenly-spaced rhythm (all intervals similar)
    // Threshold: >90% similarity = evenly-spaced quarter notes (no subdivisions)
    const float EVENLY_SPACED_THRESHOLD = 0.90f;
    if (similar_ratio >= EVENLY_SPACED_THRESHOLD) {
        // Evenly-spaced pattern detected - use simple average (no doubling)
        uint64_t avg_similar = sum_similar / similar_count;
#ifdef ARDUINO
        Serial.print("[BPM-DEBUG] Evenly-spaced rhythm detected: ");
        Serial.print(static_cast<int>(similar_ratio * 100));
        Serial.print("% similar (~");
        Serial.print(avg_similar / 1000);
        Serial.println("ms intervals)");
#endif
        return avg_similar;
    }
    
    // Check for subdivision pattern: 70-90% similarity with fast intervals
    const float SUBDIVISION_THRESHOLD = 0.7f;
    if (similar_ratio >= SUBDIVISION_THRESHOLD && similar_count >= 5) {
        // Use average of similar intervals only (excludes outliers)
        uint64_t avg_similar = sum_similar / similar_count;
        
        // CRITICAL: Only consider subdivision if intervals are FAST (< 400ms = 150 BPM)
        // Quarter notes at 120 BPM are ~500ms - we don't want to double those!
        // Eighth notes at 92 BPM are ~320ms - these should be doubled
        const uint64_t SUBDIVISION_MAX_INTERVAL_US = 400000;  // 400ms = 150 BPM
        if (avg_similar < SUBDIVISION_MAX_INTERVAL_US) {
            // Double it to get quarter note (assumes these are eighth notes)
            uint64_t doubled = avg_similar * 2;
            
            // Only apply if result is within reasonable BPM range (40-200 BPM)
            // 40 BPM = 1500ms, 200 BPM = 300ms
            if (doubled >= 300000 && doubled <= 1500000) {
#ifdef ARDUINO
                Serial.print("[BPM-DEBUG] Subdivision detected! ");
                Serial.print(similar_count);
                Serial.print("/");
                Serial.print(interval_count);
                Serial.print(" intervals similar (~");
                Serial.print(avg_similar / 1000);
                Serial.print("ms), doubled to ");
                Serial.print(doubled / 1000);
                Serial.println("ms");
#endif
                return doubled;
            }
        }
    }
    
    // Step 3: Test if longest interval is the base by checking if all others are subdivisions
    // Use the sorted array to get the longest interval (most likely quarter note)
    uint64_t max_interval = sorted[interval_count - 1];
    
    // Tolerance: ±15% for each subdivision level
    uint8_t match_count = 0;
    uint64_t normalized_sum = 0;
    
    for (uint8_t i = 0; i < interval_count; ++i) {
        uint64_t interval = intervals[i];
        
        // Try to match binary subdivisions AND triplets
        float ratio_to_base = static_cast<float>(interval) / static_cast<float>(max_interval);
        
        // Binary subdivisions (powers of 2)
        // Check if within 15% of 2.0 (half note - max_interval was actually quarter)
        if (ratio_to_base >= 1.85f && ratio_to_base <= 2.15f) {
            normalized_sum += (interval / 2);  // Normalize to quarter note
            match_count++;
        }
        // Check if within 15% of 1.0 (quarter note)
        else if (ratio_to_base >= 0.85f && ratio_to_base <= 1.15f) {
            normalized_sum += interval;  // Use as-is
            match_count++;
        }
        // Check if within 15% of 0.667 (quarter note triplet - 2/3 of quarter)
        else if (ratio_to_base >= 0.567f && ratio_to_base <= 0.767f) {
            normalized_sum += (interval * 3 / 2);  // Normalize to quarter note
            match_count++;
        }
        // Check if within 15% of 0.5 (8th note)
        else if (ratio_to_base >= 0.425f && ratio_to_base <= 0.575f) {
            normalized_sum += (interval * 2);  // Normalize to quarter note
            match_count++;
        }
        // Check if within 15% of 0.333 (8th note triplet - 1/3 of quarter)
        else if (ratio_to_base >= 0.283f && ratio_to_base <= 0.383f) {
            normalized_sum += (interval * 3);  // Normalize to quarter note
            match_count++;
        }
        // Check if within 15% of 0.25 (16th note)
        else if (ratio_to_base >= 0.2125f && ratio_to_base <= 0.2875f) {
            normalized_sum += (interval * 4);  // Normalize to quarter note
            match_count++;
        }
        // Check if within 15% of 0.167 (16th note triplet - 1/6 of quarter)
        else if (ratio_to_base >= 0.142f && ratio_to_base <= 0.192f) {
            normalized_sum += (interval * 6);  // Normalize to quarter note
            match_count++;
        }
        // Check if within 15% of 0.125 (32nd note)
        else if (ratio_to_base >= 0.10625f && ratio_to_base <= 0.14375f) {
            normalized_sum += (interval * 8);  // Normalize to quarter note
            match_count++;
        }
        // If doesn't match any, try half of max_interval as base instead
    }
    
    // If most intervals match subdivisions, use normalized average
    if (match_count >= (interval_count * 3 / 4)) {  // 75% match threshold
        return normalized_sum / match_count;
    }
    
    // Fallback: Try half of max_interval as base (maybe max was actually a half note)
    uint64_t half_max = max_interval / 2;
    match_count = 0;
    normalized_sum = 0;
    
    for (uint8_t i = 0; i < interval_count; ++i) {
        uint64_t interval = intervals[i];
        float ratio_to_half = static_cast<float>(interval) / static_cast<float>(half_max);
        
        // Try matching with half_max as base quarter note (including triplets)
        if (ratio_to_half >= 1.85f && ratio_to_half <= 2.15f) {
            normalized_sum += (interval / 2);  // Half note
            match_count++;
        }
        else if (ratio_to_half >= 0.85f && ratio_to_half <= 1.15f) {
            normalized_sum += interval;  // Quarter note
            match_count++;
        }
        else if (ratio_to_half >= 0.567f && ratio_to_half <= 0.767f) {
            normalized_sum += (interval * 3 / 2);  // Quarter triplet
            match_count++;
        }
        else if (ratio_to_half >= 0.425f && ratio_to_half <= 0.575f) {
            normalized_sum += (interval * 2);  // 8th note
            match_count++;
        }
        else if (ratio_to_half >= 0.283f && ratio_to_half <= 0.383f) {
            normalized_sum += (interval * 3);  // 8th triplet
            match_count++;
        }
        else if (ratio_to_half >= 0.2125f && ratio_to_half <= 0.2875f) {
            normalized_sum += (interval * 4);  // 16th note
            match_count++;
        }
        else if (ratio_to_half >= 0.142f && ratio_to_half <= 0.192f) {
            normalized_sum += (interval * 6);  // 16th triplet
            match_count++;
        }
        else if (ratio_to_half >= 0.10625f && ratio_to_half <= 0.14375f) {
            normalized_sum += (interval * 8);  // 32nd note
            match_count++;
        }
    }
    
    if (match_count >= (interval_count * 3 / 4)) {
        return normalized_sum / match_count;
    }
    
    // Last resort: simple average (original naive approach)
    uint64_t total_interval = 0;
    for (uint8_t i = 0; i < interval_count; ++i) {
        total_interval += intervals[i];
    }
    return total_interval / interval_count;
}

float BPMCalculation::calculateStandardDeviation(uint64_t avg_interval) {
    // Calculate standard deviation of NORMALIZED intervals (quarter-note basis)
    // This allows proper stability detection with mixed subdivisions (8ths, 16ths)
    
    if (state_.tap_count < 3) {
        return 0.0f;  // Need at least 3 taps (2 intervals) for stddev
    }
    
    // Step 1: Calculate raw intervals from circular buffer
    uint64_t intervals[BPMCalculationState::MAX_TAPS];
    uint8_t interval_count = 0;
    
    // Calculate oldest tap index in circular buffer
    uint8_t oldest_idx = (state_.write_index + BPMCalculationState::MAX_TAPS - state_.tap_count) % BPMCalculationState::MAX_TAPS;
    
    for (uint8_t i = 1; i < state_.tap_count; ++i) {
        uint8_t prev_idx = (oldest_idx + i - 1) % BPMCalculationState::MAX_TAPS;
        uint8_t curr_idx = (oldest_idx + i) % BPMCalculationState::MAX_TAPS;
        
        uint64_t prev_tap = state_.tap_buffer[prev_idx];
        uint64_t curr_tap = state_.tap_buffer[curr_idx];
        
        // Skip if timestamps are invalid or out of order
        // Note: timestamp 0 is valid (tests start at T=0), so only skip if curr <= prev
        if (curr_tap <= prev_tap) {
            continue;
        }
        
        uint64_t interval = curr_tap - prev_tap;
        // Sanity check: reject intervals outside reasonable BPM range
        if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
            interval <= BPMCalculationState::MAX_INTERVAL_US) {
            intervals[interval_count++] = interval;
        }
    }
    
    if (interval_count < 2) {
        return 0.0f;  // Need at least 2 intervals for stddev
    }
    
    // Step 2: Normalize intervals to quarter-note basis
    uint64_t max_interval = 0;
    for (uint8_t i = 0; i < interval_count; ++i) {
        if (intervals[i] > max_interval) {
            max_interval = intervals[i];
        }
    }
    
    // Step 3: Normalize intervals to quarter notes
    uint64_t normalized_intervals[BPMCalculationState::MAX_TAPS];
    uint8_t normalized_count = 0;
    
    for (uint8_t i = 0; i < interval_count; ++i) {
        uint64_t interval = intervals[i];
        float ratio_to_base = static_cast<float>(interval) / static_cast<float>(max_interval);
        
        // Normalize based on subdivision ratio
        if (ratio_to_base >= 1.85f && ratio_to_base <= 2.15f) {
            normalized_intervals[normalized_count++] = interval / 2;  // Half note
        }
        else if (ratio_to_base >= 0.85f && ratio_to_base <= 1.15f) {
            normalized_intervals[normalized_count++] = interval;  // Quarter note
        }
        else if (ratio_to_base >= 0.567f && ratio_to_base <= 0.767f) {
            normalized_intervals[normalized_count++] = interval * 3 / 2;  // Quarter triplet
        }
        else if (ratio_to_base >= 0.425f && ratio_to_base <= 0.575f) {
            normalized_intervals[normalized_count++] = interval * 2;  // 8th note
        }
        else if (ratio_to_base >= 0.283f && ratio_to_base <= 0.383f) {
            normalized_intervals[normalized_count++] = interval * 3;  // 8th triplet
        }
        else if (ratio_to_base >= 0.2125f && ratio_to_base <= 0.2875f) {
            normalized_intervals[normalized_count++] = interval * 4;  // 16th note
        }
        else if (ratio_to_base >= 0.142f && ratio_to_base <= 0.192f) {
            normalized_intervals[normalized_count++] = interval * 6;  // 16th triplet
        }
        else if (ratio_to_base >= 0.10625f && ratio_to_base <= 0.14375f) {
            normalized_intervals[normalized_count++] = interval * 8;  // 32nd note
        }
        // Skip intervals that don't match known subdivisions
    }
    
    if (normalized_count < 2) {
        return 0.0f;  // Not enough normalized intervals
    }
    
    // Step 4: Calculate variance on normalized intervals
    uint64_t sum_squared_diff = 0;
    
    for (uint8_t i = 0; i < normalized_count; ++i) {
        int64_t diff = static_cast<int64_t>(normalized_intervals[i]) - static_cast<int64_t>(avg_interval);
        sum_squared_diff += static_cast<uint64_t>(diff * diff);
    }
    
    // Variance = sum_squared_diff / (n - 1)  [sample variance]
    float variance = static_cast<float>(sum_squared_diff) / static_cast<float>(normalized_count - 1);
    
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
    
    // Calculate oldest tap index in circular buffer
    uint8_t oldest_idx = (state_.write_index + BPMCalculationState::MAX_TAPS - state_.tap_count) % BPMCalculationState::MAX_TAPS;
    
    // Use first 6 taps (gives 5 intervals) for baseline
    for (uint8_t i = 1; i < 6 && i < state_.tap_count; ++i) {
        uint8_t prev_idx = (oldest_idx + i - 1) % BPMCalculationState::MAX_TAPS;
        uint8_t curr_idx = (oldest_idx + i) % BPMCalculationState::MAX_TAPS;
        
        uint64_t prev_tap = state_.tap_buffer[prev_idx];
        uint64_t curr_tap = state_.tap_buffer[curr_idx];
        
        // Skip if timestamps are invalid or out of order
        if (curr_tap <= prev_tap) {
            continue;
        }
        
        uint64_t interval = curr_tap - prev_tap;
        // Sanity check: reject intervals outside reasonable BPM range
        if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
            interval <= BPMCalculationState::MAX_INTERVAL_US) {
            baseline_sum += interval;
            baseline_count++;
        }
    }
    
    if (baseline_count == 0) return;
    
    uint64_t baseline_avg = baseline_sum / baseline_count;
    
    // Count how many of last 5 intervals are >= 1.8× baseline (HALF_TEMPO_RATIO)
    uint8_t slow_interval_count = 0;
    
    for (uint8_t i = state_.tap_count - 5; i < state_.tap_count; ++i) {
        if (i > 0) {
            uint8_t prev_idx = (oldest_idx + i - 1) % BPMCalculationState::MAX_TAPS;
            uint8_t curr_idx = (oldest_idx + i) % BPMCalculationState::MAX_TAPS;
            
            uint64_t prev_tap = state_.tap_buffer[prev_idx];
            uint64_t curr_tap = state_.tap_buffer[curr_idx];
            
            // Skip if timestamps are invalid or out of order
            if (curr_tap <= prev_tap) {
                continue;
            }
            
            uint64_t interval = curr_tap - prev_tap;
            // Sanity check: reject intervals outside reasonable BPM range
            if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
                interval <= BPMCalculationState::MAX_INTERVAL_US) {
                float ratio = static_cast<float>(interval) / static_cast<float>(baseline_avg);
                
                if (ratio >= BPMCalculationState::HALF_TEMPO_RATIO) {
                    slow_interval_count++;
                }
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
    
    // Calculate oldest tap index in circular buffer
    uint8_t oldest_idx = (state_.write_index + BPMCalculationState::MAX_TAPS - state_.tap_count) % BPMCalculationState::MAX_TAPS;
    
    // Use first 6 taps (gives 5 intervals) for baseline
    for (uint8_t i = 1; i < 6 && i < state_.tap_count; ++i) {
        uint8_t prev_idx = (oldest_idx + i - 1) % BPMCalculationState::MAX_TAPS;
        uint8_t curr_idx = (oldest_idx + i) % BPMCalculationState::MAX_TAPS;
        
        uint64_t prev_tap = state_.tap_buffer[prev_idx];
        uint64_t curr_tap = state_.tap_buffer[curr_idx];
        
        // Skip if timestamps are invalid or out of order
        if (curr_tap <= prev_tap) {
            continue;
        }
        
        uint64_t interval = curr_tap - prev_tap;
        // Sanity check: reject intervals outside reasonable BPM range
        if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
            interval <= BPMCalculationState::MAX_INTERVAL_US) {
            baseline_sum += interval;
            baseline_count++;
        }
    }
    
    if (baseline_count == 0) return;
    
    uint64_t baseline_avg = baseline_sum / baseline_count;
    
    // Count how many of last 5 intervals are <= 0.6× baseline (DOUBLE_TEMPO_RATIO)
    uint8_t fast_interval_count = 0;
    
    for (uint8_t i = state_.tap_count - 5; i < state_.tap_count; ++i) {
        if (i > 0) {
            uint8_t prev_idx = (oldest_idx + i - 1) % BPMCalculationState::MAX_TAPS;
            uint8_t curr_idx = (oldest_idx + i) % BPMCalculationState::MAX_TAPS;
            
            uint64_t prev_tap = state_.tap_buffer[prev_idx];
            uint64_t curr_tap = state_.tap_buffer[curr_idx];
            
            // Skip if timestamps are invalid or out of order
            if (curr_tap <= prev_tap) {
                continue;
            }
            
            uint64_t interval = curr_tap - prev_tap;
            // Sanity check: reject intervals outside reasonable BPM range
            if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
                interval <= BPMCalculationState::MAX_INTERVAL_US) {
                float ratio = static_cast<float>(interval) / static_cast<float>(baseline_avg);
                
                if (ratio <= BPMCalculationState::DOUBLE_TEMPO_RATIO) {
                    fast_interval_count++;
                }
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

// ============================================================================
// Shadow Tracker (Parallel Tempo Detection)
// ============================================================================

void BPMCalculation::updateShadowTracker(uint64_t timestamp_us) {
    // Add tap to shadow buffer (smaller, faster-responding buffer)
    state_.shadow_buffer[state_.shadow_write_index] = timestamp_us;
    state_.shadow_write_index = (state_.shadow_write_index + 1) % BPMCalculationState::SHADOW_TAPS;
    
    if (state_.shadow_count < BPMCalculationState::SHADOW_TAPS) {
        state_.shadow_count++;
    }
    
    // Calculate shadow BPM when we have enough data
    if (state_.shadow_count >= BPMCalculationState::MIN_TAPS_SUBDIVISION) {
        calculateShadowBPM();
        
        // Check if we should switch to shadow tempo
        if (shouldSwitchTempo()) {
            switchToShadowTempo();
        }
    }
}

void BPMCalculation::calculateShadowBPM() {
    // Simplified calculation for faster response (no rhythm quantization)
    if (state_.shadow_count < 2) {
        state_.shadow_bpm = 0.0f;
        state_.shadow_cv = 100.0f;
        state_.shadow_confidence = 0;
        return;
    }
    
    // Calculate average interval from shadow buffer
    uint64_t interval_sum = 0;
    uint8_t interval_count = 0;
    
    for (uint8_t i = 1; i < state_.shadow_count; ++i) {
        uint64_t prev_tap = state_.shadow_buffer[i - 1];
        uint64_t curr_tap = state_.shadow_buffer[i];
        
        if (curr_tap > prev_tap) {
            uint64_t interval = curr_tap - prev_tap;
            
            // Validate interval
            if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
                interval <= BPMCalculationState::MAX_INTERVAL_US) {
                interval_sum += interval;
                interval_count++;
            }
        }
    }
    
    if (interval_count == 0) {
        state_.shadow_bpm = 0.0f;
        state_.shadow_cv = 100.0f;
        state_.shadow_confidence = 0;
        return;
    }
    
    uint64_t avg_interval = interval_sum / interval_count;
    state_.shadow_bpm = 60000000.0f / static_cast<float>(avg_interval);
    
    // Validate BPM range
    if (state_.shadow_bpm < BPMCalculationState::MIN_BPM || 
        state_.shadow_bpm > BPMCalculationState::MAX_BPM) {
        state_.shadow_bpm = 0.0f;
        state_.shadow_cv = 100.0f;
        state_.shadow_confidence = 0;
        return;
    }
    
    // Calculate standard deviation and CV
    float variance_sum = 0.0f;
    for (uint8_t i = 1; i < state_.shadow_count; ++i) {
        uint64_t prev_tap = state_.shadow_buffer[i - 1];
        uint64_t curr_tap = state_.shadow_buffer[i];
        
        if (curr_tap > prev_tap) {
            uint64_t interval = curr_tap - prev_tap;
            
            if (interval >= BPMCalculationState::MIN_INTERVAL_US && 
                interval <= BPMCalculationState::MAX_INTERVAL_US) {
                float deviation = static_cast<float>(interval) - static_cast<float>(avg_interval);
                variance_sum += (deviation * deviation);
            }
        }
    }
    
    float stddev = std::sqrt(variance_sum / static_cast<float>(interval_count));
    state_.shadow_cv = (stddev / static_cast<float>(avg_interval)) * 100.0f;
    
    // Update confidence: increment if stable, reset if unstable
    if (state_.shadow_cv < BPMCalculationState::STABILITY_CV_THRESHOLD) {
        if (state_.shadow_confidence < 10) {
            state_.shadow_confidence++;
        }
    } else {
        state_.shadow_confidence = 0;
    }
}

bool BPMCalculation::shouldSwitchTempo() {
    // Shadow tracker needs high confidence
    if (state_.shadow_confidence < BPMCalculationState::SHADOW_CONFIDENCE_THRESHOLD) {
        return false;
    }
    
    // Shadow BPM must be valid
    if (state_.shadow_bpm < BPMCalculationState::MIN_BPM || 
        state_.shadow_bpm > BPMCalculationState::MAX_BPM) {
        return false;
    }
    
    // Shadow must be stable
    if (state_.shadow_cv >= BPMCalculationState::STABILITY_CV_THRESHOLD) {
        return false;
    }
    
    // Case 1: No locked tempo yet - shadow can force lock if significantly different from current
    if (state_.locked_bpm == 0.0f) {
        // Shadow must differ significantly from current calculated BPM
        float bpm_diff = (state_.shadow_bpm > state_.current_bpm) 
                        ? (state_.shadow_bpm - state_.current_bpm)
                        : (state_.current_bpm - state_.shadow_bpm);
        
        // Require significant difference AND shadow more stable
        return (bpm_diff >= BPMCalculationState::SHADOW_SWITCH_MIN_BPM_DIFF) &&
               (state_.shadow_cv < state_.coefficient_of_variation);
    }
    
    // Case 2: Locked tempo exists - shadow can replace it
    float bpm_diff = (state_.shadow_bpm > state_.locked_bpm) 
                    ? (state_.shadow_bpm - state_.locked_bpm)
                    : (state_.locked_bpm - state_.shadow_bpm);
    
    // Require significant difference (avoid switching on noise)
    if (bpm_diff < BPMCalculationState::SHADOW_SWITCH_MIN_BPM_DIFF) {
        return false;
    }
    
    // Switch if shadow tracker is more stable than primary
    // OR if primary is showing deviation while shadow is stable
    bool primary_deviating = (state_.stable_count < BPMCalculationState::DEVIATION_COUNT_UNLOCK / 2);
    
    return (state_.shadow_cv < state_.coefficient_of_variation) || primary_deviating;
}

void BPMCalculation::switchToShadowTempo() {
    // Adopt shadow tempo as new locked tempo
    state_.locked_bpm = state_.shadow_bpm;
    // DO NOT overwrite current_bpm - let primary rhythm-quantized calculation handle it
    
    // Reset deviation counter (stable at new tempo)
    state_.stable_count = BPMCalculationState::DEVIATION_COUNT_UNLOCK;
    
    // Reset shadow confidence to detect next tempo change
    state_.shadow_confidence = 0;
    
    // Clear shadow buffer to rebuild at new tempo
    state_.shadow_count = 0;
    state_.shadow_write_index = 0;
    
    // Fire callback to notify tempo change
    fireBPMUpdateCallback();
}
