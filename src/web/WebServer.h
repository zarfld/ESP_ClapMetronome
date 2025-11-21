/**
 * @file WebServer.h
 * @brief Async Web Server with REST API and WebSocket support
 * 
 * @component DES-C-003: Web Server & WebSocket
 * @implements AC-WEB-001 through AC-WEB-008
 * @requirement REQ-F-003: Web UI display
 * @architecture ADR-WEB-001: ESPAsyncWebServer architecture
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Implementation)
 * @standard ISO/IEC/IEEE 42010:2011 (Architecture Description)
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/49
 * @date 2025-11-21
 */

#ifndef CLAP_METRONOME_WEB_SERVER_H
#define CLAP_METRONOME_WEB_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

#ifdef NATIVE_BUILD
// Native build simulation
#include <chrono>
#else
// ESP32 hardware
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#endif

namespace clap_metronome {

// Forward declarations
class ConfigurationManager;

/**
 * @brief HTTP request method
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE
};

/**
 * @brief HTTP request representation
 */
struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> params;
};

/**
 * @brief HTTP response representation
 */
struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string content_type = "text/plain";
};

/**
 * @brief WebSocket message
 */
struct WebSocketMessage {
    uint32_t client_id;
    std::string data;
    uint64_t timestamp_us;
};

/**
 * @brief WebServer configuration
 * 
 * @implements AC-WEB-007 (rate limiting)
 * @implements AC-WEB-009 (max clients)
 */
struct WebServerConfig {
    uint16_t port = 80;                                   ///< HTTP port
    uint8_t max_clients = 4;                              ///< AC-WEB-009: Max concurrent WebSocket clients
    uint32_t websocket_ping_interval_ms = 30000;          ///< WebSocket keepalive (30s)
    uint32_t bpm_update_rate_limit_ms = 500;              ///< AC-WEB-007: Max 2Hz (500ms interval)
    bool serve_static_files = true;                       ///< AC-WEB-001: Enable static file serving
};

/**
 * @brief Async Web Server with REST API and WebSocket support
 * 
 * Provides non-blocking HTTP server with:
 * - Static file serving from LittleFS (AC-WEB-001)
 * - REST API for configuration (AC-WEB-002, AC-WEB-003)
 * - Factory reset endpoint (AC-WEB-004)
 * - WebSocket connections (AC-WEB-005)
 * - Real-time BPM broadcasting (AC-WEB-006)
 * - Rate limiting (AC-WEB-007)
 * - Error handling (AC-WEB-008)
 * 
 * Architecture:
 * - ESPAsyncWebServer: Non-blocking HTTP server
 * - AsyncWebSocket: Real-time bidirectional messaging
 * - ArduinoJson: JSON serialization
 * - LittleFS: Static file storage
 * 
 * Performance Targets:
 * - Main loop overhead: <10ms per update() call
 * - WebSocket broadcast: <50ms to 4 clients (AC-WEB-009)
 * - CPU usage: <10% average, <15% peak (AC-WEB-010)
 * 
 * @implements #49 DES-C-003 (Web Server & WebSocket)
 * @architecture ADR-WEB-001 (ESPAsyncWebServer)
 */
class WebServer {
public:
    /**
     * @brief Construct Web Server
     * @param config_manager Configuration manager (optional, can be nullptr)
     * @param port HTTP server port
     */
    explicit WebServer(ConfigurationManager* config_manager = nullptr, uint16_t port = 80);
    
    /**
     * @brief Construct Web Server with config struct
     * @param config Server configuration
     */
    explicit WebServer(const WebServerConfig& config);
    
    /**
     * @brief Destructor
     */
    virtual ~WebServer();
    
    /**
     * @brief Initialize web server
     * 
     * - Initializes LittleFS for static files
     * - Sets up HTTP routes
     * - Starts WebSocket handler
     * - Does NOT start server (call start() separately)
     * 
     * @return true if initialization successful
     */
    bool init();
    
    /**
     * @brief Start web server
     * 
     * Starts accepting HTTP requests and WebSocket connections.
     * Must be called after init().
     */
    void start();
    
    /**
     * @brief Stop web server
     * 
     * Stops accepting new connections and closes existing ones.
     */
    void stop();
    
    /**
     * @brief Check if server is running
     * @return true if server is started
     */
    bool isRunning() const { return running_; }
    
    // ========================================================================
    // HTTP REST API (AC-WEB-002, AC-WEB-003, AC-WEB-004)
    // ========================================================================
    
    /**
     * @brief Register HTTP GET handler
     * @param path URL path (e.g., "/api/config")
     * @param handler Callback function receiving request and response
     */
    void onGet(const std::string& path, 
               std::function<void(const HttpRequest&, HttpResponse&)> handler);
    
    /**
     * @brief Register HTTP POST handler
     * @param path URL path (e.g., "/api/config")
     * @param handler Callback function receiving request and response
     */
    void onPost(const std::string& path,
                std::function<void(const HttpRequest&, HttpResponse&)> handler);
    
    /**
     * @brief Serve static file from LittleFS
     * 
     * @implements AC-WEB-001: Static file serving
     * 
     * @param path URL path (e.g., "/")
     * @param file_path LittleFS path (e.g., "/data/index.html")
     */
    void serveStatic(const std::string& path, const std::string& file_path);
    
    // ========================================================================
    // WebSocket (AC-WEB-005, AC-WEB-006)
    // ========================================================================
    
    /**
     * @brief Register WebSocket connection handler
     * @param handler Callback receiving client ID
     */
    void onWebSocketConnect(std::function<void(uint32_t client_id)> handler);
    
    /**
     * @brief Register WebSocket disconnection handler
     * @param handler Callback receiving client ID
     */
    void onWebSocketDisconnect(std::function<void(uint32_t client_id)> handler);
    
    /**
     * @brief Register WebSocket message handler
     * @param handler Callback receiving WebSocketMessage
     */
    void onWebSocketMessage(std::function<void(const WebSocketMessage&)> handler);
    
    /**
     * @brief Broadcast message to all connected WebSocket clients
     * 
     * @implements AC-WEB-006: BPM updates to all clients
     * @implements AC-WEB-009: Broadcast to 4 clients in <50ms
     * 
     * @param message JSON string to broadcast
     */
    void broadcastWebSocket(const std::string& message);
    
    /**
     * @brief Broadcast BPM update to all WebSocket clients
     * 
     * Convenience method that creates JSON message and broadcasts.
     * Respects rate limiting (AC-WEB-007).
     * 
     * @param bpm Current BPM value
     * @param stable Stability flag
     */
    void broadcastBPM(float bpm, bool stable);
    
    /**
     * @brief Send message to specific WebSocket client
     * @param client_id Target client ID
     * @param message JSON string to send
     */
    void sendWebSocket(uint32_t client_id, const std::string& message);
    
    /**
     * @brief Get number of connected WebSocket clients
     * @return Connected client count
     */
    uint32_t getWebSocketClientCount() const;
    
    // ========================================================================
    // Rate Limiting (AC-WEB-007)
    // ========================================================================
    
    /**
     * @brief Check if BPM update can be sent (rate limiting)
     * 
     * @implements AC-WEB-007: Max 2Hz (500ms interval)
     * 
     * @return true if enough time has passed since last update
     */
    bool canSendBPMUpdate();
    
    /**
     * @brief Record BPM update timestamp
     * 
     * Called after sending BPM update to enforce rate limit
     */
    void recordBPMUpdate();
    
    // ========================================================================
    // Server Management
    // ========================================================================
    
    /**
     * @brief Process pending requests (non-blocking)
     * 
     * Must be called frequently from main loop.
     * Target: <10ms execution time per call
     */
    void update();
    
    /**
     * @brief Alias for update() (Arduino convention)
     */
    void loop() { update(); }
    
    /**
     * @brief Server statistics
     */
    struct Stats {
        uint32_t http_requests = 0;          ///< Total HTTP requests handled
        uint32_t websocket_messages = 0;     ///< Total WebSocket messages received
        uint32_t websocket_broadcasts = 0;   ///< Total broadcasts sent
        uint32_t connected_clients = 0;      ///< Current connected WebSocket clients
        uint64_t last_bpm_update_us = 0;     ///< Timestamp of last BPM update (microseconds)
    };
    
    /**
     * @brief Get server statistics
     * @return Current stats
     */
    const Stats& getStats() const { return stats_; }
    
    /**
     * @brief Reset statistics counters
     */
    void resetStats();

private:
    WebServerConfig config_;
    Stats stats_;
    bool initialized_ = false;
    bool running_ = false;
    ConfigurationManager* config_manager_ = nullptr;
    
    // HTTP handlers
    std::map<std::string, std::function<void(const HttpRequest&, HttpResponse&)>> get_handlers_;
    std::map<std::string, std::function<void(const HttpRequest&, HttpResponse&)>> post_handlers_;
    
    // WebSocket handlers
    std::function<void(uint32_t)> ws_connect_handler_;
    std::function<void(uint32_t)> ws_disconnect_handler_;
    std::function<void(const WebSocketMessage&)> ws_message_handler_;
    
    // Connected WebSocket clients
    std::vector<uint32_t> ws_clients_;
    
#ifndef NATIVE_BUILD
    // ESP32 async server instances
    AsyncWebServer* async_server_ = nullptr;
    AsyncWebSocket* async_websocket_ = nullptr;
#endif

    /**
     * @brief Get current time in microseconds
     * @return Microseconds since boot (ESP32) or epoch (native)
     */
    uint64_t micros() const;
    
    /**
     * @brief Setup REST API routes
     * 
     * Registers all HTTP endpoints:
     * - GET/POST /api/config
     * - POST /api/factory-reset
     * - Static file serving
     */
    void setupRoutes();
};

} // namespace clap_metronome

#endif // CLAP_METRONOME_WEB_SERVER_H
