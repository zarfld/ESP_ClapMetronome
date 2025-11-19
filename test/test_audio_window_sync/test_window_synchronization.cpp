/**
 * @file test_window_synchronization.cpp
 * @brief Unit Tests for AC-AUDIO-014 Window Synchronization
 * 
 * TDD Cycle 14: Window Synchronization Validation
 * 
 * Acceptance Criteria:
 *   - AC-AUDIO-014: Dual buffers alternate correctly
 *   - Validates buffer swap mechanism
 *   - Tests thread-safe window operations
 * 
 * Test Strategy:
 *   This is a VALIDATION phase - dual buffer system already implemented.
 *   These unit tests validate:
 *   1. Buffer alternation correctness
 *   2. Write/read buffer independence
 *   3. Buffer swap timing and atomicity
 *   4. Data integrity during swaps
 *   5. No data loss or corruption
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (TDD) - VALIDATION Phase
 * 
 * Traceability:
 *   - Implements: AC-AUDIO-014 (Window synchronization unit tests)
 *   - Design: AudioSampleBuffer.h (DES-D-001)
 *   - Architecture: 04-design/tdd-plan-phase-05.md
 */

#include <gtest/gtest.h>
#include <vector>
#include "../../src/audio/AudioSampleBuffer.h"

using namespace clap_metronome;

/**
 * @brief Window Synchronization Test Fixture
 * 
 * Provides utilities for testing dual buffer alternation.
 */
class WindowSynchronizationTest : public ::testing::Test {
protected:
    DualAudioBuffer dual_buffer_;
    
    void SetUp() override {
        dual_buffer_.init();
    }
    
    /**
     * Fill buffer with test pattern
     */
    void fillBufferWithPattern(AudioSampleBuffer* buffer, uint16_t start_value) {
        for (size_t i = 0; i < AudioSampleBuffer::BUFFER_SIZE; i++) {
            uint64_t timestamp = i * 62;  // 16kHz sample rate
            buffer->addSample(static_cast<uint16_t>(start_value + i), timestamp);
        }
    }
    
    /**
     * Verify buffer contains expected pattern
     */
    bool verifyPattern(const AudioSampleBuffer* buffer, uint16_t start_value) {
        for (size_t i = 0; i < AudioSampleBuffer::BUFFER_SIZE; i++) {
            if (buffer->samples[i] != static_cast<uint16_t>(start_value + i)) {
                return false;
            }
        }
        return true;
    }
};

/**
 * @test InitialState_BufferZeroIsWrite
 * 
 * Validates that initial state has buffer 0 as write buffer.
 * 
 * Given: Freshly initialized dual buffer
 * When: Query write buffer pointer
 * Then: Points to buffer 0
 * 
 * Traceability: AC-AUDIO-014 (Initial state)
 */
TEST_F(WindowSynchronizationTest, InitialState_BufferZeroIsWrite) {
    // Act: Get write buffer
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    
    // Assert: Should be buffer 0
    EXPECT_EQ(&dual_buffer_.buffers[0], write_buf)
        << "Initial write buffer should be buffer 0";
    EXPECT_EQ(0, dual_buffer_.write_buffer_index)
        << "Initial write_buffer_index should be 0";
}

/**
 * @test InitialState_BufferOneIsRead
 * 
 * Validates that initial state has buffer 1 as read buffer.
 * 
 * Given: Freshly initialized dual buffer
 * When: Query read buffer pointer
 * Then: Points to buffer 1
 * 
 * Traceability: AC-AUDIO-014 (Initial state)
 */
TEST_F(WindowSynchronizationTest, InitialState_BufferOneIsRead) {
    // Act: Get read buffer
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    
    // Assert: Should be buffer 1
    EXPECT_EQ(&dual_buffer_.buffers[1], read_buf)
        << "Initial read buffer should be buffer 1";
}

/**
 * @test WriteReadBuffers_AreDistinct
 * 
 * Validates that write and read buffers are different.
 * 
 * Given: Dual buffer system
 * When: Query both buffers
 * Then: Pointers are different
 * 
 * Traceability: AC-AUDIO-014 (Buffer independence)
 */
TEST_F(WindowSynchronizationTest, WriteReadBuffers_AreDistinct) {
    // Act: Get both buffers
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    
    // Assert: Should be different
    EXPECT_NE(write_buf, read_buf)
        << "Write and read buffers must be distinct";
}

/**
 * @test SwapBuffers_AlternatesWriteIndex
 * 
 * Validates that swap() alternates the write buffer index.
 * 
 * Given: Initial write index = 0
 * When: Call swap()
 * Then: Write index becomes 1
 * When: Call swap() again
 * Then: Write index becomes 0
 * 
 * Traceability: AC-AUDIO-014 (Alternation correctness)
 */
TEST_F(WindowSynchronizationTest, SwapBuffers_AlternatesWriteIndex) {
    // Arrange: Initial state (write_buffer_index = 0)
    EXPECT_EQ(0, dual_buffer_.write_buffer_index);
    
    // Act: First swap
    dual_buffer_.swap();
    
    // Assert: Should be 1
    EXPECT_EQ(1, dual_buffer_.write_buffer_index)
        << "First swap should change index from 0 to 1";
    
    // Act: Second swap
    dual_buffer_.swap();
    
    // Assert: Should be back to 0
    EXPECT_EQ(0, dual_buffer_.write_buffer_index)
        << "Second swap should change index from 1 to 0";
}

/**
 * @test SwapBuffers_UpdatesWritePointer
 * 
 * Validates that getWriteBuffer() returns new buffer after swap.
 * 
 * Given: Write buffer points to buffer 0
 * When: Swap buffers
 * Then: Write buffer points to buffer 1
 * 
 * Traceability: AC-AUDIO-014 (Pointer consistency)
 */
TEST_F(WindowSynchronizationTest, SwapBuffers_UpdatesWritePointer) {
    // Arrange: Initial write buffer (should be buffer 0)
    AudioSampleBuffer* initial_write = dual_buffer_.getWriteBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[0], initial_write);
    
    // Act: Swap
    dual_buffer_.swap();
    
    // Assert: Write buffer now points to buffer 1
    AudioSampleBuffer* new_write = dual_buffer_.getWriteBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[1], new_write)
        << "After swap, write buffer should be buffer 1";
    EXPECT_NE(initial_write, new_write)
        << "Write buffer pointer should change after swap";
}

/**
 * @test SwapBuffers_UpdatesReadPointer
 * 
 * Validates that getReadBuffer() returns new buffer after swap.
 * 
 * Given: Read buffer points to buffer 1
 * When: Swap buffers
 * Then: Read buffer points to buffer 0
 * 
 * Traceability: AC-AUDIO-014 (Pointer consistency)
 */
TEST_F(WindowSynchronizationTest, SwapBuffers_UpdatesReadPointer) {
    // Arrange: Initial read buffer (should be buffer 1)
    AudioSampleBuffer* initial_read = dual_buffer_.getReadBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[1], initial_read);
    
    // Act: Swap
    dual_buffer_.swap();
    
    // Assert: Read buffer now points to buffer 0
    AudioSampleBuffer* new_read = dual_buffer_.getReadBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[0], new_read)
        << "After swap, read buffer should be buffer 0";
    EXPECT_NE(initial_read, new_read)
        << "Read buffer pointer should change after swap";
}

/**
 * @test SwapBuffers_ResetsNewWriteBuffer
 * 
 * Validates that swap() resets the new write buffer.
 * 
 * Given: Buffer 1 has data
 * When: Swap (making buffer 1 the write buffer)
 * Then: Buffer 1 is reset (write_index=0, is_full=false)
 * 
 * Traceability: AC-AUDIO-014 (Buffer reset after swap)
 */
TEST_F(WindowSynchronizationTest, SwapBuffers_ResetsNewWriteBuffer) {
    // Arrange: Fill buffer 1 with data (currently read buffer)
    fillBufferWithPattern(&dual_buffer_.buffers[1], 1000);
    dual_buffer_.buffers[1].is_full = true;
    
    // Act: Swap (buffer 1 becomes write buffer)
    dual_buffer_.swap();
    
    // Assert: Buffer 1 should be reset
    AudioSampleBuffer* new_write = dual_buffer_.getWriteBuffer();
    EXPECT_EQ(0, new_write->write_index)
        << "New write buffer should have write_index reset to 0";
    EXPECT_FALSE(new_write->is_full)
        << "New write buffer should have is_full = false";
}

/**
 * @test DataIntegrity_ReadBufferUnaffectedBySwap
 * 
 * Validates that read buffer data remains intact after swap.
 * 
 * Given: Buffer 0 contains pattern data
 * When: Swap buffers (buffer 0 becomes read buffer)
 * Then: Buffer 0 data unchanged
 * 
 * Traceability: AC-AUDIO-014 (Data integrity during swap)
 */
TEST_F(WindowSynchronizationTest, DataIntegrity_ReadBufferUnaffectedBySwap) {
    // Arrange: Fill buffer 0 with pattern
    const uint16_t PATTERN_START = 2000;
    fillBufferWithPattern(&dual_buffer_.buffers[0], PATTERN_START);
    
    // Act: Swap (buffer 0 becomes read buffer)
    dual_buffer_.swap();
    
    // Assert: Buffer 0 data unchanged
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    EXPECT_TRUE(verifyPattern(read_buf, PATTERN_START))
        << "Read buffer data should remain intact after swap";
}

/**
 * @test MultipleSwaps_DataIsolation
 * 
 * Validates that buffers remain isolated across multiple swaps.
 * 
 * Given: Fill buffer 0 with pattern A, swap, fill buffer 1 with pattern B
 * When: Swap again
 * Then: Buffer 0 has pattern A, buffer 1 has pattern B
 * 
 * Traceability: AC-AUDIO-014 (Buffer isolation)
 */
TEST_F(WindowSynchronizationTest, MultipleSwaps_DataIsolation) {
    // Arrange: Fill buffer 0 with pattern A
    const uint16_t PATTERN_A = 1000;
    fillBufferWithPattern(&dual_buffer_.buffers[0], PATTERN_A);
    
    // Act: Swap (buffer 1 becomes write buffer)
    dual_buffer_.swap();
    
    // Arrange: Fill buffer 1 (now write buffer) with pattern B
    const uint16_t PATTERN_B = 3000;
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    fillBufferWithPattern(write_buf, PATTERN_B);
    
    // Act: Swap again (buffer 0 becomes write buffer)
    dual_buffer_.swap();
    
    // Assert: Both buffers should have their respective patterns
    EXPECT_TRUE(verifyPattern(&dual_buffer_.buffers[0], PATTERN_A))
        << "Buffer 0 should retain pattern A";
    EXPECT_TRUE(verifyPattern(&dual_buffer_.buffers[1], PATTERN_B))
        << "Buffer 1 should retain pattern B";
}

/**
 * @test BufferFull_TriggerSwap
 * 
 * Validates typical usage: fill write buffer, swap when full.
 * 
 * Given: Empty write buffer (buffer 0)
 * When: Add 32 samples (fills buffer)
 * Then: Buffer reports full
 * When: Swap buffers
 * Then: Buffer 0 becomes read buffer with data, buffer 1 becomes empty write buffer
 * 
 * Traceability: AC-AUDIO-014 (Typical swap scenario)
 */
TEST_F(WindowSynchronizationTest, BufferFull_TriggerSwap) {
    // Arrange: Get initial write buffer
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[0], write_buf);
    
    // Act: Fill write buffer
    bool is_full = false;
    for (size_t i = 0; i < AudioSampleBuffer::BUFFER_SIZE; i++) {
        uint64_t timestamp = i * 62;
        is_full = write_buf->addSample(static_cast<uint16_t>(2000 + i), timestamp);
    }
    
    // Assert: Should be full on last sample
    EXPECT_TRUE(is_full) << "Buffer should be full after 32 samples";
    EXPECT_TRUE(write_buf->is_full) << "Buffer is_full flag should be true";
    
    // Act: Swap buffers
    dual_buffer_.swap();
    
    // Assert: Old write buffer (buffer 0) is now read buffer with data
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[0], read_buf)
        << "Buffer 0 should now be read buffer";
    EXPECT_TRUE(verifyPattern(read_buf, 2000))
        << "Read buffer should contain filled data";
    
    // Assert: New write buffer (buffer 1) is empty
    AudioSampleBuffer* new_write_buf = dual_buffer_.getWriteBuffer();
    EXPECT_EQ(&dual_buffer_.buffers[1], new_write_buf)
        << "Buffer 1 should now be write buffer";
    EXPECT_EQ(0, new_write_buf->write_index)
        << "New write buffer should be empty";
    EXPECT_FALSE(new_write_buf->is_full)
        << "New write buffer should not be full";
}

/**
 * @test ContinuousOperation_AlternatingPattern
 * 
 * Validates continuous alternation over multiple fill/swap cycles.
 * 
 * Given: Dual buffer system
 * When: Fill → swap → fill → swap (multiple cycles)
 * Then: Buffers alternate correctly, data integrity maintained
 * 
 * Traceability: AC-AUDIO-014 (Sustained operation)
 */
TEST_F(WindowSynchronizationTest, ContinuousOperation_AlternatingPattern) {
    const int NUM_CYCLES = 4;
    
    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        // Arrange: Get current write buffer
        AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
        uint8_t expected_write_index = cycle % 2;
        
        EXPECT_EQ(expected_write_index, dual_buffer_.write_buffer_index)
            << "Cycle " << cycle << ": write_buffer_index should be " << static_cast<int>(expected_write_index);
        
        // Act: Fill write buffer with cycle-specific pattern
        uint16_t pattern_start = static_cast<uint16_t>(1000 + (cycle * 100));
        fillBufferWithPattern(write_buf, pattern_start);
        
        // Assert: Data written correctly
        EXPECT_TRUE(verifyPattern(write_buf, pattern_start))
            << "Cycle " << cycle << ": write buffer should contain pattern";
        
        // Act: Swap
        dual_buffer_.swap();
        
        // Assert: Previous write buffer is now read buffer with data intact
        AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
        EXPECT_TRUE(verifyPattern(read_buf, pattern_start))
            << "Cycle " << cycle << ": read buffer should contain pattern after swap";
    }
}

/**
 * @test Timestamps_PreservedAcrossSwap
 * 
 * Validates that buffer timestamps are preserved during swap.
 * 
 * Given: Fill write buffer (captures start/end timestamps)
 * When: Swap buffers
 * Then: Timestamps remain in now-read buffer
 * 
 * Traceability: AC-AUDIO-014 (Metadata integrity)
 */
TEST_F(WindowSynchronizationTest, Timestamps_PreservedAcrossSwap) {
    // Arrange: Fill write buffer with known timestamps
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    const uint64_t START_TIME = 1000000;  // 1 second
    const uint64_t TIME_STEP = 62;        // 16kHz samples
    
    for (size_t i = 0; i < AudioSampleBuffer::BUFFER_SIZE; i++) {
        uint64_t timestamp = START_TIME + (i * TIME_STEP);
        write_buf->addSample(2000, timestamp);
    }
    
    uint64_t expected_start = write_buf->start_timestamp_us;
    uint64_t expected_end = write_buf->end_timestamp_us;
    
    EXPECT_EQ(START_TIME, expected_start);
    EXPECT_EQ(START_TIME + (31 * TIME_STEP), expected_end);
    
    // Act: Swap
    dual_buffer_.swap();
    
    // Assert: Timestamps preserved in read buffer
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    EXPECT_EQ(expected_start, read_buf->start_timestamp_us)
        << "Start timestamp should be preserved after swap";
    EXPECT_EQ(expected_end, read_buf->end_timestamp_us)
        << "End timestamp should be preserved after swap";
}

/**
 * @test EdgeCase_SwapBeforeFull
 * 
 * Validates swap behavior when buffer not yet full.
 * 
 * Given: Partially filled write buffer
 * When: Swap buffers
 * Then: Swap completes, new write buffer is ready
 * 
 * Traceability: AC-AUDIO-014 (Edge case handling)
 */
TEST_F(WindowSynchronizationTest, EdgeCase_SwapBeforeFull) {
    // Arrange: Partially fill write buffer (only 10 samples)
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    for (size_t i = 0; i < 10; i++) {
        write_buf->addSample(static_cast<uint16_t>(2000 + i), i * 62);
    }
    
    EXPECT_EQ(10, write_buf->write_index)
        << "Should have 10 samples before swap";
    EXPECT_FALSE(write_buf->is_full)
        << "Buffer should not be full";
    
    // Act: Swap anyway
    dual_buffer_.swap();
    
    // Assert: Swap completed, new write buffer is ready
    AudioSampleBuffer* new_write_buf = dual_buffer_.getWriteBuffer();
    EXPECT_EQ(0, new_write_buf->write_index)
        << "New write buffer should be reset";
    EXPECT_FALSE(new_write_buf->is_full)
        << "New write buffer should not be full";
    
    // Assert: Old buffer (now read) retains partial data
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    EXPECT_EQ(2009, read_buf->samples[9])
        << "Read buffer should still have partial data (2000+9=2009)";
}

/**
 * @test EdgeCase_MultipleSwapsWithoutWriting
 * 
 * Validates that multiple swaps without writing don't cause issues.
 * 
 * Given: Empty dual buffer
 * When: Swap multiple times without writing
 * Then: Buffers alternate correctly, no corruption
 * 
 * Traceability: AC-AUDIO-014 (Edge case handling)
 */
TEST_F(WindowSynchronizationTest, EdgeCase_MultipleSwapsWithoutWriting) {
    // Act: Swap 10 times without writing data
    for (int i = 0; i < 10; i++) {
        uint8_t expected_index = i % 2;
        EXPECT_EQ(expected_index, dual_buffer_.write_buffer_index)
            << "Iteration " << i << ": write_buffer_index should alternate";
        
        dual_buffer_.swap();
    }
    
    // Assert: Buffers remain valid and ready
    AudioSampleBuffer* write_buf = dual_buffer_.getWriteBuffer();
    AudioSampleBuffer* read_buf = dual_buffer_.getReadBuffer();
    
    EXPECT_NE(write_buf, read_buf)
        << "Buffers should still be distinct";
    EXPECT_EQ(0, write_buf->write_index)
        << "Write buffer should be ready";
}

/**
 * @test BufferSize_Constant
 * 
 * Validates buffer size constant is as expected (32 samples).
 * 
 * Given: AudioSampleBuffer definition
 * When: Query BUFFER_SIZE
 * Then: Equals 32 (from legacy design)
 * 
 * Traceability: AC-AUDIO-014 (Buffer size validation)
 */
TEST_F(WindowSynchronizationTest, BufferSize_Constant) {
    EXPECT_EQ(32U, AudioSampleBuffer::BUFFER_SIZE)
        << "Buffer size should be 32 samples (legacy design)";
}

/**
 * @test MemoryLayout_TwoBuffersIndependent
 * 
 * Validates that the two buffers are separate in memory.
 * 
 * Given: DualAudioBuffer with two buffers
 * When: Compare memory addresses
 * Then: Buffers are at different addresses
 * 
 * Traceability: AC-AUDIO-014 (Memory independence)
 */
TEST_F(WindowSynchronizationTest, MemoryLayout_TwoBuffersIndependent) {
    // Act: Get addresses of both buffers
    void* addr0 = &dual_buffer_.buffers[0];
    void* addr1 = &dual_buffer_.buffers[1];
    
    // Assert: Different addresses
    EXPECT_NE(addr0, addr1)
        << "Buffers should be at different memory addresses";
    
    // Assert: Address difference equals buffer size
    size_t expected_offset = sizeof(AudioSampleBuffer);
    size_t actual_offset = reinterpret_cast<char*>(addr1) - reinterpret_cast<char*>(addr0);
    EXPECT_EQ(expected_offset, actual_offset)
        << "Buffers should be contiguous in memory with correct offset";
}
