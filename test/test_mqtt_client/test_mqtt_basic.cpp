#ifdef NATIVE_BUILD
// Native build - GoogleTest
#include <gtest/gtest.h>
#include "../../src/mqtt/MQTTClient.h"

using namespace clap_metronome;

TEST(MQTTBasicTest, Initialization) {
    // Arrange
    MQTTConfig mqtt_config;
    mqtt_config.enabled = false;
    mqtt_config.broker_host = "";
    mqtt_config.broker_port = 1883;
    mqtt_config.device_id = "test-device";
    
    // Act
    MQTTClient mqtt_client(&mqtt_config);
    
    // Assert
    EXPECT_FALSE(mqtt_client.isConnected()) 
        << "Client should not be connected on initialization";
    EXPECT_STREQ("", mqtt_client.getLastError());
}

#else
// ESP32/ESP8266 build - Unity framework
#include <unity.h>
#include "mqtt/MQTTClient.h"

using namespace clap_metronome;

void test_mqtt_001_initialization(void) {
    // Arrange
    MQTTConfig mqtt_config;
    mqtt_config.enabled = false;
    mqtt_config.broker_host = "";
    mqtt_config.broker_port = 1883;
    mqtt_config.device_id = "test-device";
    
    // Act
    MQTTClient mqtt_client(&mqtt_config);
    
    // Assert
    TEST_ASSERT_FALSE_MESSAGE(mqtt_client.isConnected(), 
        "Client should not be connected on initialization");
    TEST_ASSERT_EQUAL_STRING("", mqtt_client.getLastError());
}

void setUp(void) {
    // Setup runs before each test
}

void tearDown(void) {
    // Cleanup runs after each test
}

void setup() {
    delay(2000);  // Allow serial monitor to open
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_001_initialization);
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}

#endif  // NATIVE_BUILD
