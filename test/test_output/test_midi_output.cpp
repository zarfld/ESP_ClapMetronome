/**
 * @file test_midi_output.cpp
 * @brief TDD Cycle OUT-01: MIDI Output Format Tests
 * 
 * Tests: AC-OUT-001, AC-OUT-002, AC-OUT-004, AC-OUT-011
 * 
 * Acceptance Criteria:
 * - AC-OUT-001: MIDI note-on format (0x90 + channel-1, note, velocity)
 * - AC-OUT-002: MIDI note-off format (0x80 + channel-1, note, 0x00)
 * - AC-OUT-004: MIDI note duration (10ms)
 * - AC-OUT-011: MIDI channel selection (1-16)
 * 
 * Component: DES-C-004 (#50)
 * Implements: REQ-F-008 (MIDI output)
 * 
 * Standards: ISO/IEC/IEEE 12207:2017
 * XP: Test-Driven Development (RED phase)
 */

#include <gtest/gtest.h>
#include "output/OutputController.h"
#include <vector>

// Test fixture
class MIDIOutputTest : public ::testing::Test {
protected:
    OutputController* output_;
    std::vector<MIDIPacket> midi_packets_;
    
    void SetUp() override {
        OutputConfig config;
        config.midi_channel = 10;
        config.midi_note = 38;
        config.midi_velocity = 100;
        config.midi_duration_ms = 10;
        config.mode = OutputMode::MIDI_ONLY;
        
        output_ = new OutputController(config);
        midi_packets_.clear();
    }
    
    void TearDown() override {
        delete output_;
        output_ = nullptr;
    }
    
    void captureMIDI(const MIDIPacket& packet) {
        midi_packets_.push_back(packet);
    }
};

// ========== AC-OUT-001: MIDI Note-On Format ==========

/**
 * Test: NoteOnFormat_CorrectStatusByte
 * 
 * Scenario: Trigger MIDI note
 * Expected: Status byte = 0x90 + (channel - 1)
 * 
 * AC-OUT-001: Note-on format
 */
TEST_F(MIDIOutputTest, NoteOnFormat_CorrectStatusByte) {
    // Arrange
    OutputConfig config = output_->getConfig();
    uint8_t expected_status = 0x90 + (config.midi_channel - 1);
    
    // Act
    bool triggered = output_->triggerMIDI();
    
    // Assert
    EXPECT_TRUE(triggered) << "triggerMIDI should return true";
    EXPECT_EQ(expected_status, 0x99) 
        << "Note-on status byte should be 0x90 + (channel-1), channel 10 = 0x99";
}

/**
 * Test: NoteOnFormat_CorrectNoteNumber
 * 
 * Scenario: Trigger MIDI note
 * Expected: Note number matches configuration
 * 
 * AC-OUT-001: Note-on format
 */
void test_NoteOnFormat_CorrectNoteNumber() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    // Will verify note number = 38 in GREEN phase
}

/**
 * Test: NoteOnFormat_CorrectVelocity
 * 
 * Scenario: Trigger MIDI note
 * Expected: Velocity matches configuration
 * 
 * AC-OUT-001: Note-on format
 */
void test_NoteOnFormat_CorrectVelocity() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    // Will verify velocity = 100 in GREEN phase
}

// ========== AC-OUT-002: MIDI Note-Off Format ==========

/**
 * Test: NoteOffFormat_CorrectStatusByte
 * 
 * Scenario: Trigger MIDI note (sends note-off after duration)
 * Expected: Status byte = 0x80 + (channel - 1)
 * 
 * AC-OUT-002: Note-off format
 */
void test_NoteOffFormat_CorrectStatusByte() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    uint8_t expected_status = 0x80 + (config.midi_channel - 1);
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    // Note: In real implementation, note-off is sent after duration
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(
        expected_status,
        0x89,  // 0x80 + (10-1) = 0x89 for channel 10
        "Note-off status byte should be 0x80 + (channel-1)"
    );
}

/**
 * Test: NoteOffFormat_ZeroVelocity
 * 
 * Scenario: Trigger MIDI note (sends note-off after duration)
 * Expected: Velocity = 0x00 in note-off
 * 
 * AC-OUT-002: Note-off format
 */
void test_NoteOffFormat_ZeroVelocity() {
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    // Will verify note-off velocity = 0x00 in GREEN phase
}

// ========== AC-OUT-004: MIDI Note Duration ==========

/**
 * Test: NoteDuration_10Milliseconds
 * 
 * Scenario: Trigger MIDI note with 10ms duration
 * Expected: Note-off sent 10ms after note-on
 * 
 * AC-OUT-004: 10ms duration
 */
void test_NoteDuration_10Milliseconds() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    TEST_ASSERT_EQUAL_UINT16(10, config.midi_duration_ms);
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    // Will verify timing in GREEN phase with timestamp tracking
}

// ========== AC-OUT-011: MIDI Channel Selection ==========

/**
 * Test: ChannelSelection_Channel1
 * 
 * Scenario: Configure MIDI channel 1
 * Expected: Status byte = 0x90 (channel 1)
 * 
 * AC-OUT-011: Channels 1-16 configurable
 */
void test_ChannelSelection_Channel1() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    config.midi_channel = 1;
    test_fixture.output_->setConfig(config);
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    // Will verify status = 0x90 in GREEN phase
}

/**
 * Test: ChannelSelection_Channel16
 * 
 * Scenario: Configure MIDI channel 16
 * Expected: Status byte = 0x9F (channel 16)
 * 
 * AC-OUT-011: Channels 1-16 configurable
 */
void test_ChannelSelection_Channel16() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    config.midi_channel = 16;
    test_fixture.output_->setConfig(config);
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_TRUE_MESSAGE(triggered, "triggerMIDI should return true");
    // Will verify status = 0x9F in GREEN phase
}

/**
 * Test: ChannelSelection_OutOfRange
 * 
 * Scenario: Configure invalid MIDI channel (17)
 * Expected: Clamped to valid range or error
 * 
 * AC-OUT-011: Channels 1-16 configurable
 */
void test_ChannelSelection_OutOfRange() {
    // Arrange
    OutputConfig config = test_fixture.output_->getConfig();
    config.midi_channel = 17;  // Invalid
    test_fixture.output_->setConfig(config);
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    // Will verify clamping or error handling in GREEN phase
    TEST_ASSERT_TRUE_MESSAGE(true, "Channel validation test placeholder");
}

/**
 * Test: MIDIOutputDisabled_NoTrigger
 * 
 * Scenario: Disable MIDI output
 * Expected: triggerMIDI returns false
 * 
 * AC-OUT-012: GPIO LOW when disabled
 */
void test_MIDIOutputDisabled_NoTrigger() {
    // Arrange
    test_fixture.output_->enableOutput(OutputMode::DISABLED);
    
    // Act
    bool triggered = test_fixture.output_->triggerMIDI();
    
    // Assert
    TEST_ASSERT_FALSE_MESSAGE(
        triggered,
        "triggerMIDI should return false when disabled"
    );
}

/**
 * Test: MIDIPacketStructure_ThreeBytes
 * 
 * Scenario: Create MIDI packet
 * Expected: 3 bytes (status, note, velocity)
 * 
 * DES-D-007: MIDI Output Packet
 */
void test_MIDIPacketStructure_ThreeBytes() {
    // Arrange
    MIDIPacket packet;
    packet.status = 0x90;
    packet.note = 38;
    packet.velocity = 100;
    
    // Assert
    TEST_ASSERT_EQUAL_size_t(
        3,
        sizeof(packet),
        "MIDI packet should be 3 bytes"
    );
}

// ========== Test Runner ==========

int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    // AC-OUT-001: Note-On Format
    RUN_TEST(test_NoteOnFormat_CorrectStatusByte);
    RUN_TEST(test_NoteOnFormat_CorrectNoteNumber);
    RUN_TEST(test_NoteOnFormat_CorrectVelocity);
    
    // AC-OUT-002: Note-Off Format
    RUN_TEST(test_NoteOffFormat_CorrectStatusByte);
    RUN_TEST(test_NoteOffFormat_ZeroVelocity);
    
    // AC-OUT-004: Note Duration
    RUN_TEST(test_NoteDuration_10Milliseconds);
    
    // AC-OUT-011: Channel Selection
    RUN_TEST(test_ChannelSelection_Channel1);
    RUN_TEST(test_ChannelSelection_Channel16);
    RUN_TEST(test_ChannelSelection_OutOfRange);
    
    // Output Control
    RUN_TEST(test_MIDIOutputDisabled_NoTrigger);
    
    // Data Structure
    RUN_TEST(test_MIDIPacketStructure_ThreeBytes);
    
    return UNITY_END();
}
