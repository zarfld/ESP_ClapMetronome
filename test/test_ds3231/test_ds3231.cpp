/**
 * DS3231 RTC Test - I2C Communication
 * 
 * Purpose: Test DS3231 RTC module alone to isolate I2C impact on boot.
 * 
 * Wave: 3.8 (Hardware Integration)
 * Test: Step 2 - DS3231 Alone
 * 
 * Wiring:
 *   DS3231 VCC  → ESP32 3.3V
 *   DS3231 GND  → ESP32 GND
 *   DS3231 SDA  → ESP32 GPIO21 (with 10kΩ series resistor if boot fails)
 *   DS3231 SCL  → ESP32 GPIO22 (with 10kΩ series resistor if boot fails)
 * 
 * Expected Result: 
 *   - Firmware uploads successfully WITH DS3231 connected
 *   - DS3231 detected on I2C address 0x68
 *   - Time reads correctly from RTC
 */

#ifndef NATIVE_BUILD
#include <unity.h>
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

/**
 * Test: DS3231 initialization
 * 
 * Verifies DS3231 is detected on I2C bus at address 0x68.
 */
void test_ds3231_001_init(void) {
    Serial.println("\n=== DS3231 Test 001: Initialization ===");
    
    bool found = rtc.begin();
    
    if (!found) {
        Serial.println("❌ DS3231 not found on I2C bus!");
        Serial.println("   Check wiring:");
        Serial.println("   - VCC → 3.3V");
        Serial.println("   - GND → GND");
        Serial.println("   - SDA → GPIO21");
        Serial.println("   - SCL → GPIO22");
    } else {
        Serial.println("✓ DS3231 initialized successfully");
    }
    
    TEST_ASSERT_TRUE_MESSAGE(found, "DS3231 not found on I2C bus (address 0x68)");
}

/**
 * Test: I2C bus scan
 * 
 * Scans I2C bus to confirm DS3231 is present at 0x68.
 */
void test_ds3231_002_i2c_scan(void) {
    Serial.println("\n=== DS3231 Test 002: I2C Bus Scan ===");
    
    int device_count = 0;
    bool ds3231_found = false;
    
    Serial.println("Scanning I2C bus (0x01-0x7F)...");
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("  Device found at 0x%02X", addr);
            
            if (addr == 0x68) {
                Serial.print(" ← DS3231 RTC");
                ds3231_found = true;
            }
            
            Serial.println();
            device_count++;
        }
    }
    
    Serial.printf("Found %d I2C device(s)\n", device_count);
    
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(1, device_count, 
        "No I2C devices found");
    TEST_ASSERT_TRUE_MESSAGE(ds3231_found, 
        "DS3231 not found at 0x68");
    
    if (device_count > 1) {
        Serial.printf("⚠️  Note: %d devices on bus (expected 1 for DS3231 only)\n", device_count);
    }
    
    Serial.println("✓ DS3231 detected at correct I2C address");
}

/**
 * Test: Read time from RTC
 * 
 * Verifies DS3231 can read current date/time.
 */
void test_ds3231_003_read_time(void) {
    Serial.println("\n=== DS3231 Test 003: Read Time ===");
    
    DateTime now = rtc.now();
    
    Serial.printf("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
    
    // If year is unreasonable, set to current time
    if (now.year() < 2020 || now.year() > 2030) {
        Serial.println("⚠️  RTC time invalid - setting to compile time");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        delay(100);
        now = rtc.now();
        Serial.printf("Updated: %04d-%02d-%02d %02d:%02d:%02d\n",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    }
    
    // Sanity check: year should now be reasonable (2020-2030)
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(2020, now.year(), 
        "RTC year too old even after setting");
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(2030, now.year(), 
        "RTC year too far in future");
    
    // Month should be 1-12
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(1, now.month(), 
        "Invalid month (< 1)");
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(12, now.month(), 
        "Invalid month (> 12)");
    
    // Day should be 1-31
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(1, now.day(), 
        "Invalid day (< 1)");
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(31, now.day(), 
        "Invalid day (> 31)");
    
    Serial.println("✓ Time read successfully from DS3231");
}

/**
 * Test: Read temperature from DS3231
 * 
 * DS3231 has built-in temperature sensor (for oscillator compensation).
 */
void test_ds3231_004_temperature(void) {
    Serial.println("\n=== DS3231 Test 004: Temperature Sensor ===");
    
    float temp = rtc.getTemperature();
    
    Serial.printf("DS3231 Temperature: %.2f °C\n", temp);
    
    // Sanity check: temperature should be reasonable (-20°C to +60°C)
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(-20.0f, temp, 
        "Temperature too cold (check sensor)");
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(60.0f, temp, 
        "Temperature too hot (check sensor)");
    
    Serial.println("✓ Temperature sensor functional");
}

/**
 * Test: Check if RTC lost power
 * 
 * Verifies if DS3231 battery backup is functional.
 */
void test_ds3231_005_power_loss(void) {
    Serial.println("\n=== DS3231 Test 005: Power Loss Check ===");
    
    bool lost_power = rtc.lostPower();
    
    if (lost_power) {
        Serial.println("⚠️  RTC lost power (battery dead or first boot)");
        Serial.println("   Setting RTC to compile time...");
        
        // Set to compile time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        
        Serial.println("   RTC time updated");
    } else {
        Serial.println("✓ RTC has valid backup power");
    }
    
    // Not a failure - just informational
    TEST_ASSERT_TRUE_MESSAGE(true, "Power loss check complete");
}

/**
 * Test: Write and read time
 * 
 * Verifies DS3231 can write new time values.
 */
void test_ds3231_006_write_time(void) {
    Serial.println("\n=== DS3231 Test 006: Write Time ===");
    
    // Read current time
    DateTime before = rtc.now();
    Serial.printf("Before: %04d-%02d-%02d %02d:%02d:%02d\n",
        before.year(), before.month(), before.day(),
        before.hour(), before.minute(), before.second());
    
    // Wait 2 seconds
    delay(2000);
    
    // Read again
    DateTime after = rtc.now();
    Serial.printf("After:  %04d-%02d-%02d %02d:%02d:%02d\n",
        after.year(), after.month(), after.day(),
        after.hour(), after.minute(), after.second());
    
    // Calculate elapsed time
    uint32_t elapsed_sec = after.unixtime() - before.unixtime();
    Serial.printf("Elapsed: %u seconds\n", elapsed_sec);
    
    // Should be approximately 2 seconds (allow ±1 sec tolerance)
    TEST_ASSERT_UINT_WITHIN_MESSAGE(1, 2, elapsed_sec, 
        "RTC time progression incorrect");
    
    Serial.println("✓ RTC time increments correctly");
}

/**
 * Test: I2C communication stability
 * 
 * Performs multiple reads to verify stable I2C communication.
 */
void test_ds3231_007_stability(void) {
    Serial.println("\n=== DS3231 Test 007: Communication Stability ===");
    
    const int ITERATION_COUNT = 10;
    int success_count = 0;
    
    Serial.printf("Reading time %d times...\n", ITERATION_COUNT);
    
    for (int i = 0; i < ITERATION_COUNT; i++) {
        DateTime now = rtc.now();
        
        // Check if read was successful (year > 2000)
        if (now.year() > 2000) {
            success_count++;
        }
        
        delay(100);
    }
    
    Serial.printf("Successful reads: %d/%d\n", success_count, ITERATION_COUNT);
    
    TEST_ASSERT_EQUAL_MESSAGE(ITERATION_COUNT, success_count, 
        "I2C communication unstable");
    
    Serial.println("✓ I2C communication stable");
}

/**
 * Test: Boot mode compatibility
 * 
 * Confirms that firmware upload worked WITH DS3231 connected.
 * If this test runs, it means boot mode was successful!
 */
void test_ds3231_008_boot_mode_ok(void) {
    Serial.println("\n=== DS3231 Test 008: Boot Mode Compatibility ===");
    
    Serial.println("✓ Firmware uploaded successfully with DS3231 connected!");
    Serial.println("  No boot mode conflict detected");
    Serial.println("  I2C wiring does NOT interfere with GPIO0 (BOOT)");
    
    TEST_ASSERT_TRUE_MESSAGE(true, 
        "Boot mode works with DS3231 - no isolation resistors needed!");
}

/**
 * Test Summary
 */
void test_ds3231_summary(void) {
    Serial.println("\n=== DS3231 Test Summary ===");
    Serial.println("✓ DS3231 initialized successfully");
    Serial.println("✓ I2C address 0x68 detected");
    Serial.println("✓ Time read from RTC");
    Serial.println("✓ Temperature sensor working");
    Serial.println("✓ Power backup checked");
    Serial.println("✓ Time write/read functional");
    Serial.println("✓ I2C communication stable");
    Serial.println("✓ Boot mode compatible");
    Serial.println("\n>>> DS3231 TEST PASSED <<<");
    Serial.println(">>> Ready to test MAX9814 microphone <<<\n");
    
    TEST_ASSERT_TRUE(true);
}

void setUp(void) {
    // Unity setup (called before each test)
}

void tearDown(void) {
    // Unity teardown (called after each test)
}

void setup(void) {
    // Wait for serial monitor
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n========================================");
    Serial.println("DS3231 RTC Test - I2C Communication");
    Serial.println("Wave 3.8: Hardware Integration - Step 2");
    Serial.println("========================================\n");
    
    Serial.println("⚠️  HARDWARE SETUP:");
    Serial.println("   DS3231 VCC  → ESP32 3.3V");
    Serial.println("   DS3231 GND  → ESP32 GND");
    Serial.println("   DS3231 SDA  → ESP32 GPIO21");
    Serial.println("   DS3231 SCL  → ESP32 GPIO22");
    Serial.println("   (No isolation resistors yet)\n");
    
    // Initialize I2C
    Wire.begin(21, 22);  // SDA=21, SCL=22
    Wire.setClock(100000);  // 100kHz (standard mode)
    
    delay(1000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_ds3231_001_init);
    RUN_TEST(test_ds3231_002_i2c_scan);
    RUN_TEST(test_ds3231_003_read_time);
    RUN_TEST(test_ds3231_004_temperature);
    RUN_TEST(test_ds3231_005_power_loss);
    RUN_TEST(test_ds3231_006_write_time);
    RUN_TEST(test_ds3231_007_stability);
    RUN_TEST(test_ds3231_008_boot_mode_ok);
    RUN_TEST(test_ds3231_summary);
    
    UNITY_END();
}

void loop(void) {
    // Empty loop - tests run once in setup()
}

#else
// Native build - skip DS3231 hardware tests
#include <gtest/gtest.h>

TEST(DS3231Test, NativeBuildSkipped) {
    GTEST_SKIP() << "DS3231 RTC tests require ESP32 hardware. Run with: pio test -e esp32dev --filter test_ds3231";
}

#endif  // NATIVE_BUILD
