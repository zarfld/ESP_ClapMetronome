/**
 * @file test_simple_http.cpp
 * @brief Simple HTTP GET test to isolate HTTPClient issue
 * 
 * @description
 * Minimal test to verify basic HTTP GET functionality
 * Helps isolate whether problem is in:
 * - HTTPClient library
 * - ESPAsyncWebServer routing
 * - Network stack
 * 
 * @run pio test -e esp32dev --filter test_simple_http
 * @date 2025-11-21
 */

#ifndef NATIVE_BUILD

#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
#include "../credentials.h"

#define TEST_TIMEOUT_MS 10000

// Test state
namespace {
    String server_ip;
    bool wifi_connected = false;
}

void setupWiFi() {
    if (wifi_connected) return;
    
    Serial.println("\n[WiFi] Connecting to: " + String(WIFI_SSID));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < TEST_TIMEOUT_MS) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        server_ip = WiFi.localIP().toString();
        wifi_connected = true;
        Serial.println("[WiFi] Connected! IP: " + server_ip);
    } else {
        Serial.println("[WiFi] Connection FAILED");
        wifi_connected = false;
    }
}

void setUp(void) {
    setupWiFi();
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * Test: Simple HTTP GET to google.com
 * 
 * Verifies HTTPClient works at all
 */
void test_http_client_basic(void) {
    Serial.println("\n=== Test: HTTPClient Basic Functionality ===");
    
    HTTPClient http;
    http.setTimeout(5000);
    
    // Try a known-good endpoint
    Serial.println("GET: http://httpbin.org/get");
    http.begin("http://httpbin.org/get");
    
    int response_code = http.GET();
    Serial.printf("Response code: %d\n", response_code);
    
    if (response_code > 0) {
        String payload = http.getString();
        Serial.printf("Payload length: %d\n", payload.length());
        TEST_ASSERT_EQUAL(200, response_code);
    } else {
        Serial.printf("HTTP GET failed: %d\n", response_code);
        TEST_FAIL_MESSAGE("HTTPClient GET failed");
    }
    
    http.end();
    
    Serial.println("✓ HTTPClient basic test passed");
}

/**
 * Test: Multiple sequential GETs
 * 
 * Verifies HTTPClient can handle multiple requests
 */
void test_http_client_multiple_gets(void) {
    Serial.println("\n=== Test: Multiple HTTP GETs ===");
    
    HTTPClient http;
    http.setTimeout(5000);
    
    for (int i = 0; i < 3; i++) {
        Serial.printf("\nRequest %d:\n", i + 1);
        
        http.begin("http://httpbin.org/get");
        int response_code = http.GET();
        
        Serial.printf("Response code: %d\n", response_code);
        TEST_ASSERT_EQUAL_MESSAGE(200, response_code, 
            String("Request " + String(i+1) + " failed").c_str());
        
        http.end();
        delay(100);  // Small delay between requests
    }
    
    Serial.println("\n✓ Multiple GETs test passed");
}

/**
 * Test: HTTP GET with connection reuse
 * 
 * Verifies connection pooling/reuse
 */
void test_http_client_reuse(void) {
    Serial.println("\n=== Test: HTTP Connection Reuse ===");
    
    HTTPClient http;
    http.setTimeout(5000);
    http.setReuse(true);  // Enable connection reuse
    
    for (int i = 0; i < 3; i++) {
        Serial.printf("\nReused request %d:\n", i + 1);
        
        http.begin("http://httpbin.org/get");
        int response_code = http.GET();
        
        Serial.printf("Response code: %d\n", response_code);
        TEST_ASSERT_EQUAL_MESSAGE(200, response_code,
            String("Reused request " + String(i+1) + " failed").c_str());
        
        // Don't call end() - let it reuse the connection
    }
    
    http.end();  // Finally close
    
    Serial.println("\n✓ Connection reuse test passed");
}

void setup() {
    delay(2000);  // Wait for serial
    
    UNITY_BEGIN();
    
    RUN_TEST(test_http_client_basic);
    RUN_TEST(test_http_client_multiple_gets);
    RUN_TEST(test_http_client_reuse);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}

#else
// Native build - hardware-specific test, no native implementation
// File intentionally empty for native builds to skip compilation

#endif  // NATIVE_BUILD
