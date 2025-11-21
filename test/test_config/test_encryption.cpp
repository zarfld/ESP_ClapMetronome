/**
 * @file test_encryption.cpp
 * @brief Configuration Manager Credential Encryption Tests
 * 
 * @component DES-C-006: Configuration Manager
 * @implements AC-CFG-006: Password encryption
 * @requirement REQ-NF-003: Security (credential encryption)
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Testing)
 * @standard IEEE 1012-2016 (Verification and Validation)
 * 
 * @description
 * Tests for credential encryption in NVS storage:
 * - WiFi password encryption/decryption
 * - MQTT password encryption/decryption
 * - Encrypted storage in NVS (not plain text)
 * - Decryption after reboot
 * 
 * TDD Cycle: CFG-05 (Credential Encryption)
 * - RED: Write failing tests for encryption
 * - GREEN: Implement encryption/decryption
 * - REFACTOR: Ensure security and performance
 * 
 * @date 2025-11-21
 */

#include <gtest/gtest.h>
#include "config/ConfigurationManager.h"
#include <cstring>

using namespace clap_metronome;

// ============================================================================
// TEST FIXTURE
// ============================================================================

class ConfigEncryptionTest : public ::testing::Test {
protected:
    ConfigurationManager* config_;
    
    void SetUp() override {
        // Reset NVS storage for test isolation
        ConfigurationManager::resetNVSForTesting();
        
        config_ = new ConfigurationManager();
        config_->init();
    }
    
    void TearDown() override {
        if (config_ != nullptr) {
            delete config_;
            config_ = nullptr;
        }
        ConfigurationManager::resetNVSForTesting();
    }
    
    // Helper: Simulate reboot by deleting and recreating config manager
    // Storage persists (simulating NVS across reboot)
    void simulateReboot() {
        if (config_ != nullptr) {
            delete config_;
            config_ = nullptr;
        }
        config_ = new ConfigurationManager();
        config_->init();
    }
    
    // Helper: Check if data contains plain text password
    bool containsPlainText(const uint8_t* data, size_t size, const char* password) {
        if (std::strlen(password) == 0) {
            return false;  // Empty password, nothing to check
        }
        
        // Search for password substring in data
        size_t pass_len = std::strlen(password);
        for (size_t i = 0; i <= size - pass_len; ++i) {
            if (std::memcmp(data + i, password, pass_len) == 0) {
                return true;  // Found plain text password
            }
        }
        return false;  // Password not found in plain text
    }
};

// ============================================================================
// WIFI PASSWORD ENCRYPTION TESTS
// ============================================================================

TEST_F(ConfigEncryptionTest, WifiPassword_SetAndRetrieve_WorksCorrectly) {
    // Set WiFi password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_ssid, "TestNetwork", sizeof(network.wifi_ssid) - 1);
    std::strncpy(network.wifi_password, "SecretPass123", sizeof(network.wifi_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    
    // Retrieve and verify
    NetworkConfig retrieved = config_->getNetworkConfig();
    EXPECT_STREQ(retrieved.wifi_password, "SecretPass123");
}

TEST_F(ConfigEncryptionTest, WifiPassword_SaveAndLoad_PreservesPassword) {
    // Set and save WiFi password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_ssid, "TestNetwork", sizeof(network.wifi_ssid) - 1);
    std::strncpy(network.wifi_password, "MyWiFiPassword", sizeof(network.wifi_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify password restored correctly
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.wifi_ssid, "TestNetwork");
    EXPECT_STREQ(loaded.wifi_password, "MyWiFiPassword");
}

TEST_F(ConfigEncryptionTest, WifiPassword_StoredEncrypted_NotPlainText) {
    // Set WiFi password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_password, "PlainTextPassword", sizeof(network.wifi_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Get raw NVS data (test-only access)
    std::vector<uint8_t> raw_data = config_->getNVSRawData("network");
    ASSERT_FALSE(raw_data.empty());
    
    // Verify password is NOT stored in plain text
    EXPECT_FALSE(containsPlainText(raw_data.data(), raw_data.size(), "PlainTextPassword"))
        << "WiFi password should be encrypted in NVS, not stored as plain text";
}

TEST_F(ConfigEncryptionTest, WifiPassword_EmptyPassword_HandledCorrectly) {
    // Set empty WiFi password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_ssid, "OpenNetwork", sizeof(network.wifi_ssid) - 1);
    network.wifi_password[0] = '\0';  // Empty password
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify empty password restored
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.wifi_password, "");
}

// ============================================================================
// MQTT PASSWORD ENCRYPTION TESTS
// ============================================================================

TEST_F(ConfigEncryptionTest, MqttPassword_SetAndRetrieve_WorksCorrectly) {
    // Set MQTT password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.mqtt_username, "admin", sizeof(network.mqtt_username) - 1);
    std::strncpy(network.mqtt_password, "MqttSecret456", sizeof(network.mqtt_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    
    // Retrieve and verify
    NetworkConfig retrieved = config_->getNetworkConfig();
    EXPECT_STREQ(retrieved.mqtt_password, "MqttSecret456");
}

TEST_F(ConfigEncryptionTest, MqttPassword_SaveAndLoad_PreservesPassword) {
    // Set and save MQTT password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.mqtt_broker, "mqtt.example.com", sizeof(network.mqtt_broker) - 1);
    std::strncpy(network.mqtt_username, "user", sizeof(network.mqtt_username) - 1);
    std::strncpy(network.mqtt_password, "MyMqttPassword", sizeof(network.mqtt_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify password restored correctly
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.mqtt_username, "user");
    EXPECT_STREQ(loaded.mqtt_password, "MyMqttPassword");
}

TEST_F(ConfigEncryptionTest, MqttPassword_StoredEncrypted_NotPlainText) {
    // Set MQTT password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.mqtt_password, "SecretMqttPass", sizeof(network.mqtt_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Get raw NVS data (test-only access)
    std::vector<uint8_t> raw_data = config_->getNVSRawData("network");
    ASSERT_FALSE(raw_data.empty());
    
    // Verify password is NOT stored in plain text
    EXPECT_FALSE(containsPlainText(raw_data.data(), raw_data.size(), "SecretMqttPass"))
        << "MQTT password should be encrypted in NVS, not stored as plain text";
}

// ============================================================================
// BOTH PASSWORDS ENCRYPTION TESTS
// ============================================================================

TEST_F(ConfigEncryptionTest, BothPasswords_SaveAndLoad_PreserveBoth) {
    // Set both WiFi and MQTT passwords
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_ssid, "TestWiFi", sizeof(network.wifi_ssid) - 1);
    std::strncpy(network.wifi_password, "WiFiPass123", sizeof(network.wifi_password) - 1);
    std::strncpy(network.mqtt_broker, "mqtt.test.com", sizeof(network.mqtt_broker) - 1);
    std::strncpy(network.mqtt_username, "mqttuser", sizeof(network.mqtt_username) - 1);
    std::strncpy(network.mqtt_password, "MqttPass456", sizeof(network.mqtt_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify both passwords restored
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.wifi_password, "WiFiPass123");
    EXPECT_STREQ(loaded.mqtt_password, "MqttPass456");
}

TEST_F(ConfigEncryptionTest, BothPasswords_StoredEncrypted_NeitherPlainText) {
    // Set both passwords
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_password, "WiFiSecret", sizeof(network.wifi_password) - 1);
    std::strncpy(network.mqtt_password, "MqttSecret", sizeof(network.mqtt_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Get raw NVS data
    std::vector<uint8_t> raw_data = config_->getNVSRawData("network");
    ASSERT_FALSE(raw_data.empty());
    
    // Verify neither password is in plain text
    EXPECT_FALSE(containsPlainText(raw_data.data(), raw_data.size(), "WiFiSecret"))
        << "WiFi password should be encrypted";
    EXPECT_FALSE(containsPlainText(raw_data.data(), raw_data.size(), "MqttSecret"))
        << "MQTT password should be encrypted";
}

// ============================================================================
// SPECIAL CHARACTERS AND EDGE CASES
// ============================================================================

TEST_F(ConfigEncryptionTest, Password_WithSpecialCharacters_HandledCorrectly) {
    // Set password with special characters
    NetworkConfig network = config_->getNetworkConfig();
    const char* special_pass = "P@ss!W0rd#$%^&*()";
    std::strncpy(network.wifi_password, special_pass, sizeof(network.wifi_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify special characters preserved
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.wifi_password, special_pass);
}

TEST_F(ConfigEncryptionTest, Password_MaxLength_HandledCorrectly) {
    // Set maximum length password (64 chars)
    NetworkConfig network = config_->getNetworkConfig();
    const char* max_pass = "1234567890123456789012345678901234567890123456789012345678901234";  // 64 chars
    std::strncpy(network.wifi_password, max_pass, sizeof(network.wifi_password) - 1);
    network.wifi_password[64] = '\0';  // Ensure null termination
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify max length password preserved
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.wifi_password, max_pass);
}

// ============================================================================
// FACTORY RESET AND PASSWORD CLEARING
// ============================================================================

TEST_F(ConfigEncryptionTest, FactoryReset_ClearsPasswords) {
    // Set passwords
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_password, "WillBeCleared", sizeof(network.wifi_password) - 1);
    std::strncpy(network.mqtt_password, "AlsoCleared", sizeof(network.mqtt_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Factory reset
    config_->factoryReset();
    
    // Verify passwords cleared
    NetworkConfig reset = config_->getNetworkConfig();
    EXPECT_STREQ(reset.wifi_password, "");
    EXPECT_STREQ(reset.mqtt_password, "");
}

TEST_F(ConfigEncryptionTest, PasswordChange_UpdatesEncryptedValue) {
    // Set initial password
    NetworkConfig network = config_->getNetworkConfig();
    std::strncpy(network.wifi_password, "OldPassword", sizeof(network.wifi_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Change password
    std::strncpy(network.wifi_password, "NewPassword", sizeof(network.wifi_password) - 1);
    EXPECT_TRUE(config_->setNetworkConfig(network));
    EXPECT_TRUE(config_->saveConfig());
    
    // Simulate reboot
    simulateReboot();
    
    // Verify new password is loaded (not old one)
    NetworkConfig loaded = config_->getNetworkConfig();
    EXPECT_STREQ(loaded.wifi_password, "NewPassword");
    EXPECT_STRNE(loaded.wifi_password, "OldPassword");
}
