/**
 * @file test_network_transport.cpp
 * @brief Unit tests for RTP-MIDI Network Transport (OUT-02)
 * 
 * TDD Cycle: OUT-02 (RED Phase)
 * Component: DES-C-004 (Output Controller)
 * Verifies: AC-OUT-003 (RTP-MIDI network transport)
 *           AC-OUT-008 (Network latency <10ms)
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (TDD)
 * 
 * @see RFC 6295 - RTP Payload Format for MIDI
 * @see 04-design/component-designs/DES-C-004-output-controller.md
 */

#include <gtest/gtest.h>
#include "output/OutputController.h"
#include "mocks/time_mock.h"

/**
 * @brief Test fixture for network transport tests
 */
class NetworkTransportTest : public ::testing::Test {
protected:
    OutputController* controller;
    OutputConfig config;
    
    void SetUp() override {
        reset_mock_time();
        
        // Configure for MIDI output via RTP-MIDI
        config.mode = OutputMode::MIDI_ONLY;
        config.rtp_port = 5004;  // Standard RTP-MIDI control port
        config.midi_ppqn = 24;
        
        controller = new OutputController(config);
    }
    
    void TearDown() override {
        delete controller;
        controller = nullptr;
    }
};

// ========== UDP Socket Tests ==========

/**
 * Test: UDP socket creation on port 5004
 * Verifies: AC-OUT-003 (UDP transport)
 * Expected: Socket bound to RTP-MIDI control port
 */
TEST_F(NetworkTransportTest, UDPSocket_CreatedOnPort5004) {
    // Given: Controller initialized with port 5004
    // (configured in SetUp)
    
    // When: Query network configuration
    auto net_config = controller->getNetworkConfig();
    
    // Then: UDP socket is bound to port 5004
    EXPECT_TRUE(net_config.socket_open);
    EXPECT_EQ(net_config.local_port, 5004);
    EXPECT_EQ(net_config.protocol, NetworkProtocol::UDP);
}

/**
 * Test: Data socket on port 5005
 * Verifies: AC-OUT-003 (RTP-MIDI uses dual ports)
 * Expected: Data socket available on port 5005
 */
TEST_F(NetworkTransportTest, DataSocket_AvailableOnPort5005) {
    // Given: Controller with RTP-MIDI enabled
    
    // When: Query data port configuration
    auto net_config = controller->getNetworkConfig();
    
    // Then: Data port is 5005 (control + 1)
    EXPECT_EQ(net_config.data_port, 5005);
}

/**
 * Test: Socket creation failure handling
 * Verifies: Graceful degradation when network unavailable
 * Expected: Returns false but doesn't crash
 */
TEST_F(NetworkTransportTest, SocketFailure_HandledGracefully) {
    // Given: Network unavailable (simulated)
    controller->simulateNetworkFailure(true);
    
    // When: Attempt to send MIDI message
    bool result = controller->sendMIDIClock();
    
    // Then: Returns false but doesn't crash
    EXPECT_FALSE(result);
    
    // And: State remains consistent
    auto state = controller->getOutputState();
    EXPECT_FALSE(state.syncing);
}

// ========== RTP-MIDI Packet Format Tests (RFC 6295) ==========

/**
 * Test: RTP header format
 * Verifies: RFC 6295 Section 2.1 (RTP Header)
 * Expected: V=2, PT=97, proper sequence number
 */
TEST_F(NetworkTransportTest, RTPHeader_CorrectFormat) {
    // Given: Fresh controller (sequence = 0)
    
    // When: Send first MIDI clock
    controller->sendMIDIClock();
    
    // Then: RTP header has correct format
    auto last_packet = controller->getLastSentPacket();
    
    // RTP version = 2 (bits 0-1 of byte 0)
    EXPECT_EQ((last_packet[0] >> 6) & 0x03, 2);
    
    // Payload type = 97 (MIDI over RTP)
    EXPECT_EQ(last_packet[1] & 0x7F, 97);
    
    // Sequence number = 0 (first packet)
    uint16_t seq = (last_packet[2] << 8) | last_packet[3];
    EXPECT_EQ(seq, 0);
}

/**
 * Test: MIDI command section encoding
 * Verifies: RFC 6295 Section 3 (MIDI Command Section)
 * Expected: Proper encapsulation of System Real-Time messages
 */
TEST_F(NetworkTransportTest, MIDICommandSection_CorrectEncoding) {
    // Given: Controller ready
    
    // When: Send MIDI Beat Clock (0xF8)
    controller->sendMIDIClock();
    
    // Then: Packet contains 0xF8 in MIDI command section
    auto packet = controller->getLastSentPacket();
    
    // Find MIDI command section (after RTP header)
    // RTP header = 12 bytes, then MIDI section
    size_t midi_offset = 12;
    
    // Command section should contain 0xF8
    bool found_0xF8 = false;
    for (size_t i = midi_offset; i < packet.size(); i++) {
        if (packet[i] == 0xF8) {
            found_0xF8 = true;
            break;
        }
    }
    EXPECT_TRUE(found_0xF8);
}

/**
 * Test: System Real-Time message encapsulation
 * Verifies: Single-byte messages properly wrapped
 * Expected: Minimal packet with 0xF8/0xFA/0xFC
 */
TEST_F(NetworkTransportTest, SystemRealTime_MinimalEncapsulation) {
    // Given: Controller ready
    
    // When: Send START message (0xFA)
    controller->sendMIDIStart();
    
    // Then: Packet is minimal size (RTP header + MIDI section)
    auto packet = controller->getLastSentPacket();
    
    // RTP-MIDI packet should be compact (<50 bytes)
    EXPECT_LT(packet.size(), 50ULL);
    
    // Contains 0xFA
    bool contains_start = false;
    for (auto byte : packet) {
        if (byte == 0xFA) {
            contains_start = true;
            break;
        }
    }
    EXPECT_TRUE(contains_start);
}

/**
 * Test: RTP timestamp field
 * Verifies: Timestamp represents microseconds since epoch
 * Expected: 32-bit timestamp in RTP header
 */
TEST_F(NetworkTransportTest, RTPTimestamp_UsesMicroseconds) {
    // Given: Advance time to non-zero value
    advance_time_us(1000000);  // 1 second
    
    // When: Send MIDI clock
    controller->sendMIDIClock();
    
    // Then: RTP timestamp field populated
    auto packet = controller->getLastSentPacket();
    
    // RTP timestamp is bytes 4-7 (32-bit)
    uint32_t timestamp = (packet[4] << 24) | (packet[5] << 16) | 
                         (packet[6] << 8) | packet[7];
    
    // Should be non-zero and match current time
    EXPECT_GT(timestamp, 0U);
    EXPECT_GE(timestamp, 1000000U);  // At least 1 second
}

// ========== Network Transmission Tests ==========

/**
 * Test: Send single MIDI Beat Clock
 * Verifies: AC-OUT-003 (Network transmission)
 * Expected: UDP packet sent successfully
 */
TEST_F(NetworkTransportTest, SendClock_TransmitsUDPPacket) {
    // Given: Network available
    
    // When: Send MIDI clock
    bool result = controller->sendMIDIClock();
    
    // Then: Transmission succeeds
    EXPECT_TRUE(result);
    
    // And: Network stats updated
    auto stats = controller->getNetworkStats();
    EXPECT_EQ(stats.packets_sent, 1U);
    EXPECT_GT(stats.bytes_sent, 0U);
}

/**
 * Test: Send START and STOP messages
 * Verifies: All message types transmit
 * Expected: Multiple packets sent
 */
TEST_F(NetworkTransportTest, SendStartStop_TransmitsMultiplePackets) {
    // Given: Network available
    
    // When: Send START, clock, STOP
    controller->sendMIDIStart();
    controller->sendMIDIClock();
    controller->sendMIDIStop();
    
    // Then: Three packets sent
    auto stats = controller->getNetworkStats();
    EXPECT_EQ(stats.packets_sent, 3U);
}

/**
 * Test: Packet size efficiency
 * Verifies: Minimal network overhead
 * Expected: Small packets (<50 bytes)
 */
TEST_F(NetworkTransportTest, PacketSize_MinimalOverhead) {
    // Given: Controller ready
    
    // When: Send various messages
    controller->sendMIDIStart();
    controller->sendMIDIClock();
    controller->sendMIDIStop();
    
    // Then: Average packet size is small
    auto stats = controller->getNetworkStats();
    size_t avg_size = stats.bytes_sent / stats.packets_sent;
    
    EXPECT_LT(avg_size, 50UL);  // RFC 6295 allows compact encoding
}

// ========== Error Handling Tests ==========

/**
 * Test: Network unavailable handling
 * Verifies: Graceful failure when WiFi down
 * Expected: Returns false, no crash
 */
TEST_F(NetworkTransportTest, NetworkDown_ReturnsFailure) {
    // Given: Network goes down
    controller->simulateNetworkFailure(true);
    
    // When: Attempt multiple sends
    bool r1 = controller->sendMIDIStart();
    bool r2 = controller->sendMIDIClock();
    bool r3 = controller->sendMIDIStop();
    
    // Then: All return false
    EXPECT_FALSE(r1);
    EXPECT_FALSE(r2);
    EXPECT_FALSE(r3);
    
    // And: No packets sent
    auto stats = controller->getNetworkStats();
    EXPECT_EQ(stats.packets_sent, 0U);
}

/**
 * Test: Send timeout handling
 * Verifies: AC-OUT-008 (Latency <10ms)
 * Expected: Timeout after 10ms, returns false
 */
TEST_F(NetworkTransportTest, SendTimeout_EnforcedAt10ms) {
    // Given: Slow network (simulated with 15ms delay)
    controller->simulateSlowNetwork(true, 15000);  // 15ms delay > 10ms timeout
    
    // When: Attempt to send message
    bool result = controller->sendMIDIClock();
    
    // Then: Operation times out and returns failure
    EXPECT_FALSE(result);  // Timeout = failure (AC-OUT-008: <10ms enforced)
    
    // And: Network stats reflect the failure
    auto stats = controller->getNetworkStats();
    EXPECT_GT(stats.send_failures, 0U);  // At least one failure recorded
}

/**
 * Test: Buffer overflow protection
 * Verifies: Burst sends don't crash
 * Expected: Handles rapid sends gracefully
 */
TEST_F(NetworkTransportTest, BufferOverflow_ProtectedAgainstBurst) {
    // Given: Controller ready
    
    // When: Send burst of 1000 clocks rapidly
    int success_count = 0;
    for (int i = 0; i < 1000; i++) {
        if (controller->sendMIDIClock()) {
            success_count++;
        }
    }
    
    // Then: Either all succeed or gracefully fail
    // (no crash, reasonable success rate)
    EXPECT_GT(success_count, 990);  // At least 99% success
    
    // And: Stats are consistent
    auto stats = controller->getNetworkStats();
    EXPECT_EQ(stats.packets_sent, success_count);
}
