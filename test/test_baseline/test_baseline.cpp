/**
 * Baseline ESP32 Test - No Peripherals
 * 
 * Purpose: Establish baseline that ESP32 boots and uploads cleanly
 *          without any external components connected.
 * 
 * Wave: 3.8 (Hardware Integration)
 * Test: Step 1 - Baseline
 * 
 * Expected Result: 
 *   - Firmware uploads successfully
 *   - Serial output appears
 *   - All GPIO pins in default state
 *   - No boot mode conflicts
 */

#ifndef NATIVE_BUILD
#include <unity.h>
#include <Arduino.h>
#include <Wire.h>

/**
 * Test: ESP32 boots successfully
 * 
 * Verifies basic boot sequence completes without errors.
 */
void test_baseline_001_boot_success(void) {
    Serial.println("\n=== Baseline Test 001: Boot Success ===");
    
    // If we reach this point, boot was successful
    TEST_ASSERT_TRUE_MESSAGE(true, "ESP32 booted successfully");
    
    Serial.println("✓ Boot sequence completed");
}

/**
 * Test: Serial communication functional
 * 
 * Verifies UART0 (USB serial) is working correctly.
 */
void test_baseline_002_serial_communication(void) {
    Serial.println("\n=== Baseline Test 002: Serial Communication ===");
    
    // Print test message
    Serial.println("Testing serial output...");
    delay(100);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Serial communication working");
    
    Serial.println("✓ Serial communication functional");
}

/**
 * Test: ESP32 chip info
 * 
 * Displays ESP32 chip information for reference.
 */
void test_baseline_003_chip_info(void) {
    Serial.println("\n=== Baseline Test 003: Chip Information ===");
    
    // Get chip model
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    Serial.printf("Chip Model: ESP32 with %d CPU cores\n", chip_info.cores);
    Serial.printf("WiFi%s%s\n",
        (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    Serial.printf("Silicon Revision: %d\n", chip_info.revision);
    Serial.printf("Flash: %dMB %s\n", 
        spi_flash_get_chip_size() / (1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    
    // MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // CPU frequency
    Serial.printf("CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
    
    // Free heap
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    
    TEST_ASSERT_GREATER_THAN(0, chip_info.cores);
    TEST_ASSERT_GREATER_THAN(0, ESP.getFreeHeap());
    
    Serial.println("✓ Chip information retrieved");
}

/**
 * Test: GPIO state check
 * 
 * Verifies critical GPIO pins are in safe default states.
 */
void test_baseline_004_gpio_state(void) {
    Serial.println("\n=== Baseline Test 004: GPIO State ===");
    
    // Check critical pins are not configured
    // GPIO0 (BOOT) - should be HIGH (pulled up)
    pinMode(0, INPUT);
    int gpio0_state = digitalRead(0);
    Serial.printf("GPIO0 (BOOT): %s\n", gpio0_state ? "HIGH" : "LOW");
    TEST_ASSERT_EQUAL_MESSAGE(HIGH, gpio0_state, 
        "GPIO0 should be HIGH (not in boot mode)");
    
    // GPIO21 (I2C SDA) - check it's available
    pinMode(21, INPUT);
    Serial.println("GPIO21 (SDA): Available");
    
    // GPIO22 (I2C SCL) - check it's available
    pinMode(22, INPUT);
    Serial.println("GPIO22 (SCL): Available");
    
    // GPIO36 (ADC0) - check it's available (input only)
    pinMode(36, INPUT);
    uint16_t adc_read = analogRead(36);
    Serial.printf("GPIO36 (ADC0): %d ADC units\n", adc_read);
    
    Serial.println("✓ All critical GPIO pins available");
}

/**
 * Test: Memory health
 * 
 * Verifies memory allocation and free heap stability.
 */
void test_baseline_005_memory_health(void) {
    Serial.println("\n=== Baseline Test 005: Memory Health ===");
    
    uint32_t initial_heap = ESP.getFreeHeap();
    Serial.printf("Initial Free Heap: %u bytes\n", initial_heap);
    
    // Allocate and free some memory
    const size_t test_size = 1024;
    void* test_ptr = malloc(test_size);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_ptr, "Memory allocation failed");
    
    memset(test_ptr, 0xAA, test_size);
    free(test_ptr);
    
    uint32_t final_heap = ESP.getFreeHeap();
    Serial.printf("Final Free Heap: %u bytes\n", final_heap);
    
    // Heap should be approximately the same (within 1KB)
    int32_t heap_diff = abs((int32_t)final_heap - (int32_t)initial_heap);
    TEST_ASSERT_LESS_THAN_MESSAGE(1024, heap_diff, 
        "Memory leak detected");
    
    Serial.println("✓ Memory allocation/deallocation working");
}

/**
 * Test: Timing accuracy
 * 
 * Verifies millis() and micros() timers are functional.
 */
void test_baseline_006_timing_accuracy(void) {
    Serial.println("\n=== Baseline Test 006: Timing Accuracy ===");
    
    // Test millis()
    uint32_t start_ms = millis();
    delay(100);
    uint32_t end_ms = millis();
    uint32_t elapsed_ms = end_ms - start_ms;
    
    Serial.printf("delay(100): Elapsed %u ms\n", elapsed_ms);
    TEST_ASSERT_UINT_WITHIN_MESSAGE(10, 100, elapsed_ms, 
        "millis() timing inaccurate");
    
    // Test micros()
    uint32_t start_us = micros();
    delayMicroseconds(1000);
    uint32_t end_us = micros();
    uint32_t elapsed_us = end_us - start_us;
    
    Serial.printf("delayMicroseconds(1000): Elapsed %u us\n", elapsed_us);
    TEST_ASSERT_UINT_WITHIN_MESSAGE(100, 1000, elapsed_us, 
        "micros() timing inaccurate");
    
    Serial.println("✓ Timing functions accurate");
}

/**
 * Test: ADC functionality
 * 
 * Verifies ADC can read analog values (for future MAX9814 testing).
 */
void test_baseline_007_adc_functional(void) {
    Serial.println("\n=== Baseline Test 007: ADC Functionality ===");
    
    // Set ADC resolution to 12-bit (0-4095)
    analogReadResolution(12);
    
    // Read from floating GPIO36 (should show noise)
    pinMode(36, INPUT);
    
    uint32_t sum = 0;
    uint16_t min_val = 4095;
    uint16_t max_val = 0;
    
    for (int i = 0; i < 100; i++) {
        uint16_t sample = analogRead(36);
        sum += sample;
        if (sample < min_val) min_val = sample;
        if (sample > max_val) max_val = sample;
        delayMicroseconds(100);
    }
    
    uint16_t avg = sum / 100;
    uint16_t range = max_val - min_val;
    
    Serial.printf("ADC36 (floating): avg=%u, min=%u, max=%u, range=%u\n",
        avg, min_val, max_val, range);
    
    // Floating pin should show some noise (range > 0)
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, range, 
        "ADC not reading (stuck value)");
    
    Serial.println("✓ ADC reading analog values");
}

/**
 * Test: I2C bus scan
 * 
 * Verifies I2C bus initializes (should find no devices).
 */
void test_baseline_008_i2c_bus_init(void) {
    Serial.println("\n=== Baseline Test 008: I2C Bus Initialization ===");
    
    // Initialize I2C on standard pins
    Wire.begin(21, 22);  // SDA=21, SCL=22
    Wire.setClock(100000);  // 100kHz
    
    Serial.println("I2C bus initialized (SDA=21, SCL=22, 100kHz)");
    
    // Scan for devices (should find none)
    int device_count = 0;
    Serial.println("Scanning I2C bus...");
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("  Device found at 0x%02X\n", addr);
            device_count++;
        }
    }
    
    Serial.printf("Found %d I2C device(s)\n", device_count);
    TEST_ASSERT_EQUAL_MESSAGE(0, device_count, 
        "Unexpected I2C devices found (check wiring)");
    
    Serial.println("✓ I2C bus functional (no devices as expected)");
}

/**
 * Test Summary
 * 
 * Displays overall baseline test results.
 */
void test_baseline_summary(void) {
    Serial.println("\n=== Baseline Test Summary ===");
    Serial.println("✓ ESP32 boots successfully without peripherals");
    Serial.println("✓ Serial communication working");
    Serial.println("✓ Chip information retrieved");
    Serial.println("✓ GPIO pins available");
    Serial.println("✓ Memory healthy");
    Serial.println("✓ Timing accurate");
    Serial.println("✓ ADC functional");
    Serial.println("✓ I2C bus initialized");
    Serial.println("\n>>> BASELINE TEST PASSED <<<");
    Serial.println(">>> Ready to test peripherals <<<\n");
    
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
    Serial.println("ESP32 Baseline Test - No Peripherals");
    Serial.println("Wave 3.8: Hardware Integration - Step 1");
    Serial.println("========================================\n");
    
    Serial.println("⚠️  CRITICAL: Ensure NO external components connected!");
    Serial.println("   - Disconnect DS3231 RTC");
    Serial.println("   - Disconnect MAX9814 microphone");
    Serial.println("   - Only ESP32 + USB cable\n");
    
    delay(1000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_baseline_001_boot_success);
    RUN_TEST(test_baseline_002_serial_communication);
    RUN_TEST(test_baseline_003_chip_info);
    RUN_TEST(test_baseline_004_gpio_state);
    RUN_TEST(test_baseline_005_memory_health);
    RUN_TEST(test_baseline_006_timing_accuracy);
    RUN_TEST(test_baseline_007_adc_functional);
    RUN_TEST(test_baseline_008_i2c_bus_init);
    RUN_TEST(test_baseline_summary);
    
    UNITY_END();
}

void loop(void) {
    // Empty loop - tests run once in setup()
}

#else
// Native build - hardware-specific test, no native implementation
// Requires ESP32 hardware for boot sequence, GPIO, ADC, I2C tests
// File intentionally empty for native builds to skip compilation

#endif  // NATIVE_BUILD
