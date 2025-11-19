/**
 * DES-D-001: Audio Sample Buffer Data Model
 * 
 * Dual-buffer system for audio sample collection and processing.
 * Implements window synchronization (AC-AUDIO-014).
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Data Model: DES-D-001 (Audio Sample Buffer)
 * Requirement: REQ-F-001 (Audio sampling)
 * Standards: IEEE 1016-2009 (Data Design), ISO/IEC/IEEE 12207:2017
 * 
 * TDD Status: RED ⏳ (Data model definition)
 * 
 * Memory Budget: 160 bytes per buffer × 2 = 320 bytes (AC-AUDIO-011)
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#ifndef AUDIO_SAMPLE_BUFFER_H
#define AUDIO_SAMPLE_BUFFER_H

#include <cstdint>
#include <cstring>  // for memset

namespace clap_metronome {

/**
 * Audio Sample Buffer
 * 
 * Implements dual-buffer design for non-blocking ADC sampling:
 * - One buffer actively collecting samples (write buffer)
 * - Other buffer being processed for beat detection (read buffer)
 * - Buffers swap when write buffer full
 * 
 * Size: 32 samples × 2 bytes + metadata = 80 bytes per buffer
 */
struct AudioSampleBuffer {
    static constexpr size_t BUFFER_SIZE = 32;  ///< Samples per buffer (from legacy code)
    
    uint16_t samples[BUFFER_SIZE];  ///< ADC readings (0-4095 for 12-bit ESP32 ADC)
    uint8_t  write_index;           ///< Current write position (0-31)
    bool     is_full;               ///< True when buffer ready for processing
    uint64_t start_timestamp_us;    ///< Timestamp of first sample in buffer
    uint64_t end_timestamp_us;      ///< Timestamp of last sample in buffer
    
    /**
     * Initialize buffer to empty state
     */
    void init() {
        std::memset(samples, 0, sizeof(samples));
        write_index = 0;
        is_full = false;
        start_timestamp_us = 0;
        end_timestamp_us = 0;
    }
    
    /**
     * Add sample to buffer
     * 
     * @param sample ADC reading to add
     * @param timestamp_us Timestamp of sample
     * @return true if buffer now full, false if still collecting
     */
    bool addSample(uint16_t sample, uint64_t timestamp_us) {
        if (write_index == 0) {
            start_timestamp_us = timestamp_us;
        }
        
        samples[write_index] = sample;
        write_index++;
        
        if (write_index >= BUFFER_SIZE) {
            end_timestamp_us = timestamp_us;
            is_full = true;
            write_index = 0;  // Ready for next cycle
            return true;
        }
        
        return false;
    }
    
    /**
     * Find minimum value in buffer
     * 
     * @return Minimum ADC value
     */
    uint16_t findMin() const {
        uint16_t min_val = samples[0];
        for (size_t i = 1; i < BUFFER_SIZE; i++) {
            if (samples[i] < min_val) {
                min_val = samples[i];
            }
        }
        return min_val;
    }
    
    /**
     * Find maximum value in buffer
     * 
     * @return Maximum ADC value
     */
    uint16_t findMax() const {
        uint16_t max_val = samples[0];
        for (size_t i = 1; i < BUFFER_SIZE; i++) {
            if (samples[i] > max_val) {
                max_val = samples[i];
            }
        }
        return max_val;
    }
    
    /**
     * Clear buffer and mark as not full
     */
    void reset() {
        is_full = false;
        write_index = 0;
    }
};

/**
 * Dual Buffer Manager
 * 
 * Manages two buffers for non-blocking operation:
 * - write_buffer: Actively collecting samples
 * - read_buffer: Being processed for beat detection
 * 
 * Total Size: 160 bytes × 2 = 320 bytes
 */
struct DualAudioBuffer {
    AudioSampleBuffer buffers[2];  ///< Two buffers for ping-pong operation
    uint8_t write_buffer_index;    ///< Index of buffer currently being written (0 or 1)
    
    /**
     * Initialize dual buffer system
     */
    void init() {
        buffers[0].init();
        buffers[1].init();
        write_buffer_index = 0;
    }
    
    /**
     * Get pointer to current write buffer
     * 
     * @return Pointer to buffer being written
     */
    AudioSampleBuffer* getWriteBuffer() {
        return &buffers[write_buffer_index];
    }
    
    /**
     * Get pointer to current read buffer (not being written)
     * 
     * @return Pointer to buffer available for processing
     */
    AudioSampleBuffer* getReadBuffer() {
        return &buffers[1 - write_buffer_index];
    }
    
    /**
     * Swap buffers when write buffer full (AC-AUDIO-014)
     * Called after write buffer fills
     */
    void swap() {
        write_buffer_index = 1 - write_buffer_index;
        getWriteBuffer()->reset();  // Prepare new write buffer
    }
};

} // namespace clap_metronome

#endif // AUDIO_SAMPLE_BUFFER_H
