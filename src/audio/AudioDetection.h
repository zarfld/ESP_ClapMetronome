/**
 * DES-C-001: Audio Detection Engine
 * 
 * Detects acoustic beats/claps via MAX9814 microphone with adaptive threshold,
 * AGC, and kick-only filtering.
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Interfaces: DES-I-004 (Beat Event), DES-I-005 (Audio Telemetry)
 * Data Models: DES-D-001 (Audio Buffer), DES-D-002 (Detection State)
 * Depends on: DES-I-001 (Timing Manager for timestamps)
 * Requirements: REQ-F-001 (clap/kick detection), REQ-NF-001 (<20ms latency)
 * QA Scenario: QA-SC-001 (>95% detection rate for 100 kicks @ 140 BPM)
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * 
 * TDD Status: RED ⏳ (Tests to be written)
 * 
 * Performance Targets (from design):
 * - Latency: <20ms from mic to beat event (AC-AUDIO-008)
 * - Accuracy: >95% detection rate (AC-AUDIO-009)
 * - CPU: <45% avg, <50% peak (AC-AUDIO-010)
 * - Memory: <400B RAM (AC-AUDIO-011)
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#ifndef AUDIO_DETECTION_H
#define AUDIO_DETECTION_H

#include "../interfaces/IBeatEventPublisher.h"
#include "../interfaces/IAudioTelemetry.h"
#include "../interfaces/ITimingProvider.h"
#include "AudioSampleBuffer.h"
#include "AudioDetectionState.h"

namespace clap_metronome {

/**
 * Audio Detection Engine
 * 
 * Implements beat/clap detection with:
 * - Adaptive threshold (AC-AUDIO-001)
 * - Rising edge state machine (AC-AUDIO-002)
 * - AGC level control (AC-AUDIO-003)
 * - Beat event emission (AC-AUDIO-004)
 * - 50ms debounce (AC-AUDIO-005)
 * - Kick-only filtering (AC-AUDIO-006)
 * - 500ms telemetry (AC-AUDIO-007)
 */
class AudioDetection : public IBeatEventPublisher, public IAudioTelemetry {
public:
    /**
     * Constructor
     * 
     * @param timing_provider Timing Manager for timestamps (DES-I-001)
     */
    explicit AudioDetection(ITimingProvider* timing_provider);
    
    /**
     * Destructor
     */
    virtual ~AudioDetection() = default;
    
    // ===== IBeatEventPublisher Interface (DES-I-004) =====
    
    /**
     * Register callback for beat event notifications
     * 
     * @param callback Function to call when beat detected
     */
    void onBeat(BeatEventCallback callback) override;
    
    /**
     * Check if beat callback registered
     * 
     * @return true if callback set
     */
    bool hasBeatCallback() const override;
    
    // ===== IAudioTelemetry Interface (DES-I-005) =====
    
    /**
     * Register callback for telemetry updates (500ms)
     * 
     * @param callback Function to call with telemetry data
     */
    void onTelemetry(AudioTelemetryCallback callback) override;
    
    /**
     * Check if telemetry callback registered
     * 
     * @return true if callback set
     */
    bool hasTelemetryCallback() const override;
    
    // ===== Component Lifecycle =====
    
    /**
     * Initialize audio detection engine
     * Sets up buffers, state, and prepares for sampling
     * 
     * @return true if initialization successful
     */
    bool init();
    
    /**
     * Process ADC sample (called from main loop or ISR)
     * 
     * This is the main entry point for audio processing.
     * Should be called frequently (e.g., every 1ms) with new ADC reading.
     * 
     * @param adc_value Raw ADC reading (0-4095 for 12-bit ESP32)
     * 
     * State machine progression:
     * 1. Add sample to buffer
     * 2. Update rolling window for threshold
     * 3. Check for threshold crossing (IDLE → RISING)
     * 4. Measure rise time (RISING → TRIGGERED)
     * 5. Emit beat event (TRIGGERED → DEBOUNCE)
     * 6. Wait debounce period (DEBOUNCE → IDLE)
     * 7. Publish telemetry every 500ms
     */
    void processSample(uint16_t adc_value);
    
    // ===== State Queries (for testing) =====
    
    /**
     * Get current detection state
     * 
     * @return Current state (IDLE, RISING, TRIGGERED, DEBOUNCE)
     */
    DetectionState getState() const;
    
    /**
     * Get current adaptive threshold
     * 
     * @return Threshold in ADC units
     */
    uint16_t getThreshold() const;
    
    /**
     * Get current AGC gain level
     * 
     * @return Gain level (GAIN_40DB, GAIN_50DB, GAIN_60DB)
     */
    AGCLevel getGainLevel() const;
    
    /**
     * Get total beats detected
     * 
     * @return Beat count since init()
     */
    uint32_t getBeatCount() const;
    
    /**
     * Get false positive count (noise rejections)
     * 
     * @return Count of below-threshold triggers
     */
    uint32_t getFalsePositiveCount() const;

private:
    // Dependencies
    ITimingProvider* timing_;           ///< Timing Manager for timestamps (DES-I-001)
    
    // Callbacks
    BeatEventCallback beat_callback_;           ///< Beat event subscriber
    AudioTelemetryCallback telemetry_callback_; ///< Telemetry subscriber
    
    // State
    DualAudioBuffer buffers_;           ///< Dual buffer for sampling (DES-D-001)
    AudioDetectionState state_;         ///< Detection state machine (DES-D-002)
    
    // Helper Methods
    
    /**
     * Check for beat trigger (threshold crossing)
     * Implements AC-AUDIO-001, AC-AUDIO-002
     * 
     * @param adc_value Current ADC reading
     * @param timestamp_us Current timestamp
     */
    void checkTrigger(uint16_t adc_value, uint64_t timestamp_us);
    
    /**
     * Emit beat event to callback
     * Implements AC-AUDIO-004
     * 
     * @param timestamp_us Beat timestamp
     * @param amplitude Peak amplitude
     * @param rise_time_us Rise time in microseconds
     */
    void emitBeatEvent(uint64_t timestamp_us, uint16_t amplitude, uint64_t rise_time_us);
    
    /**
     * Publish telemetry if 500ms elapsed
     * Implements AC-AUDIO-007
     * 
     * @param timestamp_us Current timestamp
     * @param current_adc Current ADC reading
     */
    void publishTelemetry(uint64_t timestamp_us, uint16_t current_adc);
    
    /**
     * Update AGC gain level based on clipping detection
     * Implements AC-AUDIO-003, AC-AUDIO-012
     * 
     * @param adc_value Current ADC reading
     */
    void updateAGC(uint16_t adc_value);
};

} // namespace clap_metronome

#endif // AUDIO_DETECTION_H
