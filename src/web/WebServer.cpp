/**
 * @file WebServer.cpp
 * @brief Async Web Server implementation
 * 
 * @component DES-C-003: Web Server & WebSocket
 * @implements AC-WEB-001 through AC-WEB-008
 * 
 * @date 2025-11-21
 */

#include "WebServer.h"
#include "../config/ConfigurationManager.h"
#include <algorithm>

#ifdef NATIVE_BUILD
#include <chrono>
#else
#include <Arduino.h>
#include <ArduinoJson.h>
#endif

namespace clap_metronome {

// ============================================================================
// Constructor / Destructor
// ============================================================================

WebServer::WebServer(ConfigurationManager* config_manager, uint16_t port)
    : initialized_(false)
    , running_(false)
    , config_manager_(config_manager)
{
    config_.port = port;
}

WebServer::WebServer(const WebServerConfig& config)
    : config_(config)
    , initialized_(false)
    , running_(false)
    , config_manager_(nullptr)
{
}

WebServer::~WebServer() {
#ifndef NATIVE_BUILD
    if (async_server_) {
        delete async_server_;
        async_server_ = nullptr;
    }
    if (async_websocket_) {
        delete async_websocket_;
        async_websocket_ = nullptr;
    }
#endif
}

// ============================================================================
// Initialization
// ============================================================================

bool WebServer::init() {
    if (initialized_) {
        return true;
    }
    
#ifndef NATIVE_BUILD
    // Initialize LittleFS for static files (AC-WEB-001)
    if (!LittleFS.begin(true)) {
        return false;
    }
    
    // Create async web server
    async_server_ = new AsyncWebServer(config_.port);
    
    // Create WebSocket handler
    async_websocket_ = new AsyncWebSocket("/ws");
    
    // Setup WebSocket event handler
    async_websocket_->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, 
                                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            uint32_t client_id = client->id();
            ws_clients_.push_back(client_id);
            stats_.connected_clients = ws_clients_.size();
            
            if (ws_connect_handler_) {
                ws_connect_handler_(client_id);
            }
        }
        else if (type == WS_EVT_DISCONNECT) {
            uint32_t client_id = client->id();
            auto it = std::find(ws_clients_.begin(), ws_clients_.end(), client_id);
            if (it != ws_clients_.end()) {
                ws_clients_.erase(it);
            }
            stats_.connected_clients = ws_clients_.size();
            
            if (ws_disconnect_handler_) {
                ws_disconnect_handler_(client_id);
            }
        }
        else if (type == WS_EVT_DATA) {
            WebSocketMessage msg;
            msg.client_id = client->id();
            msg.data = std::string((char*)data, len);
            msg.timestamp_us = micros();
            
            stats_.websocket_messages++;
            
            if (ws_message_handler_) {
                ws_message_handler_(msg);
            }
        }
    });
    
    // Attach WebSocket to server
    async_server_->addHandler(async_websocket_);
    
    // Register REST API routes
    setupRoutes();
#endif
    
    initialized_ = true;
    return true;
}

void WebServer::start() {
    if (!initialized_ || running_) {
        return;
    }
    
#ifndef NATIVE_BUILD
    if (async_server_) {
        async_server_->begin();
        running_ = true;
    }
#else
    running_ = true;
#endif
}

void WebServer::stop() {
    if (!running_) {
        return;
    }
    
#ifndef NATIVE_BUILD
    if (async_server_) {
        async_server_->end();
        running_ = false;
    }
#else
    running_ = false;
#endif
}

// ============================================================================
// Private Helper: Setup Routes
// ============================================================================

void WebServer::setupRoutes() {
#ifndef NATIVE_BUILD
    if (!async_server_) return;
    
    // IMPORTANT: Register API routes BEFORE static file serving
    // This ensures /api/* paths are handled by REST handlers, not file system
    
    // AC-WEB-002: GET /api/config - Return configuration as JSON
    async_server_->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        Serial.println("[WebServer] GET /api/config received");
        if (!config_manager_) {
            Serial.println("[WebServer] ERROR: Config manager is null");
            request->send(500, "application/json", "{\"error\":\"Config manager not available\"}");
            return;
        }
        
        DynamicJsonDocument doc(2048);
        
        // Audio config
        AudioConfig audio = config_manager_->getAudioConfig();
        doc["audio"]["sample_rate"] = audio.sample_rate;
        doc["audio"]["threshold_margin"] = audio.threshold_margin;
        doc["audio"]["debounce_ms"] = audio.debounce_ms;
        doc["audio"]["gain_level"] = audio.gain_level;
        doc["audio"]["kick_only_mode"] = audio.kick_only_mode;
        
        // BPM config
        BPMConfig bpm = config_manager_->getBPMConfig();
        doc["bpm"]["min_bpm"] = bpm.min_bpm;
        doc["bpm"]["max_bpm"] = bpm.max_bpm;
        doc["bpm"]["stability_threshold"] = bpm.stability_threshold;
        doc["bpm"]["tempo_correction_enabled"] = bpm.tempo_correction_enabled;
        
        // Output config
        OutputConfig output = config_manager_->getOutputConfig();
        doc["output"]["midi_enabled"] = output.midi_enabled;
        doc["output"]["midi_channel"] = output.midi_channel;
        doc["output"]["midi_note"] = output.midi_note;
        doc["output"]["midi_velocity"] = output.midi_velocity;
        doc["output"]["relay_enabled"] = output.relay_enabled;
        doc["output"]["relay_pulse_ms"] = output.relay_pulse_ms;
        
        // Network config (without passwords for security)
        NetworkConfig network = config_manager_->getNetworkConfig();
        doc["mqtt"]["enabled"] = network.mqtt_enabled;
        doc["mqtt"]["broker"] = network.mqtt_broker;
        doc["mqtt"]["port"] = network.mqtt_port;
        doc["mqtt"]["username"] = network.mqtt_username;
        
        String response;
        serializeJson(doc, response);
        Serial.println("[WebServer] Sending config response, length: " + String(response.length()));
        request->send(200, "application/json", response);
        stats_.http_requests++;
    });
    
    // AC-WEB-003: POST /api/config - Update configuration
    async_server_->on("/api/config", HTTP_POST, 
        [this](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!config_manager_) {
                request->send(500, "application/json", "{\"error\":\"Config manager not available\"}");
                return;
            }
            
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, (const char*)data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            
            // Update audio config
            if (doc.containsKey("audio")) {
                AudioConfig audio = config_manager_->getAudioConfig();
                if (doc["audio"].containsKey("sample_rate")) audio.sample_rate = doc["audio"]["sample_rate"];
                if (doc["audio"].containsKey("threshold_margin")) audio.threshold_margin = doc["audio"]["threshold_margin"];
                if (doc["audio"].containsKey("debounce_ms")) audio.debounce_ms = doc["audio"]["debounce_ms"];
                if (doc["audio"].containsKey("gain_level")) audio.gain_level = doc["audio"]["gain_level"];
                if (doc["audio"].containsKey("kick_only_mode")) audio.kick_only_mode = doc["audio"]["kick_only_mode"];
                config_manager_->setAudioConfig(audio);
            }
            
            // Update BPM config
            if (doc.containsKey("bpm")) {
                BPMConfig bpm = config_manager_->getBPMConfig();
                if (doc["bpm"].containsKey("min_bpm")) bpm.min_bpm = doc["bpm"]["min_bpm"];
                if (doc["bpm"].containsKey("max_bpm")) bpm.max_bpm = doc["bpm"]["max_bpm"];
                if (doc["bpm"].containsKey("stability_threshold")) bpm.stability_threshold = doc["bpm"]["stability_threshold"];
                if (doc["bpm"].containsKey("tempo_correction_enabled")) bpm.tempo_correction_enabled = doc["bpm"]["tempo_correction_enabled"];
                config_manager_->setBPMConfig(bpm);
            }
            
            // Update output config
            if (doc.containsKey("output")) {
                OutputConfig output = config_manager_->getOutputConfig();
                if (doc["output"].containsKey("midi_enabled")) output.midi_enabled = doc["output"]["midi_enabled"];
                if (doc["output"].containsKey("midi_channel")) output.midi_channel = doc["output"]["midi_channel"];
                if (doc["output"].containsKey("midi_note")) output.midi_note = doc["output"]["midi_note"];
                if (doc["output"].containsKey("midi_velocity")) output.midi_velocity = doc["output"]["midi_velocity"];
                if (doc["output"].containsKey("relay_enabled")) output.relay_enabled = doc["output"]["relay_enabled"];
                if (doc["output"].containsKey("relay_pulse_ms")) output.relay_pulse_ms = doc["output"]["relay_pulse_ms"];
                config_manager_->setOutputConfig(output);
            }
            
            // Save to NVS to persist changes
            config_manager_->saveConfig();
            
            request->send(200, "application/json", "{\"success\":true}");
            stats_.http_requests++;
        }
    );
    
    // AC-WEB-004: POST /api/factory-reset - Reset to factory defaults
    async_server_->on("/api/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!config_manager_) {
            request->send(500, "application/json", "{\"error\":\"Config manager not available\"}");
            return;
        }
        
        config_manager_->factoryReset();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset complete\"}");
        stats_.http_requests++;
        
        // Optional: Reboot after a delay
        // delay(1000);
        // ESP.restart();
    });
    
    // AC-WEB-001: Serve static files from LittleFS (LAST - only after API handlers)
    // Handle root path explicitly
    async_server_->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // ESPAsyncWebServer sets Content-Type automatically when using String
        File file = LittleFS.open("/index.html", "r");
        if (file) {
            String content = file.readString();
            file.close();
            request->send(200, "text/html", content);
        } else {
            request->send(404, "text/plain", "index.html not found");
        }
    });
    
    // Handle other static files
    async_server_->onNotFound([](AsyncWebServerRequest* request) {
        String uri = request->url();
        
        // API endpoints should already be handled - send 404 for unknown API paths
        if (uri.startsWith("/api/")) {
            request->send(404, "application/json", "{\"error\":\"API endpoint not found\"}");
            return;
        }
        
        // Try to serve as static file from LittleFS
        if (LittleFS.exists(uri)) {
            // Determine content type based on extension
            String content_type = "text/plain";
            if (uri.endsWith(".html")) content_type = "text/html";
            else if (uri.endsWith(".css")) content_type = "text/css";
            else if (uri.endsWith(".js")) content_type = "application/javascript";
            else if (uri.endsWith(".json")) content_type = "application/json";
            else if (uri.endsWith(".png")) content_type = "image/png";
            else if (uri.endsWith(".jpg") || uri.endsWith(".jpeg")) content_type = "image/jpeg";
            else if (uri.endsWith(".ico")) content_type = "image/x-icon";
            
            request->send(LittleFS, uri, content_type);
        } else {
            request->send(404, "text/plain", "File Not Found: " + uri);
        }
    });
#endif
}

// ============================================================================
// HTTP REST API
// ============================================================================

void WebServer::onGet(const std::string& path,
                      std::function<void(const HttpRequest&, HttpResponse&)> handler) {
    get_handlers_[path] = handler;
    
#ifndef NATIVE_BUILD
    if (async_server_) {
        async_server_->on(path.c_str(), HTTP_GET, [this, path, handler](AsyncWebServerRequest* request) {
            HttpRequest req;
            req.method = HttpMethod::GET;
            req.path = path;
            
            HttpResponse res;
            
            // Call user handler
            handler(req, res);
            
            // Send response
            request->send(res.status_code, res.content_type.c_str(), res.body.c_str());
            
            stats_.http_requests++;
        });
    }
#endif
}

void WebServer::onPost(const std::string& path,
                       std::function<void(const HttpRequest&, HttpResponse&)> handler) {
    post_handlers_[path] = handler;
    
#ifndef NATIVE_BUILD
    if (async_server_) {
        async_server_->on(path.c_str(), HTTP_POST, 
            [this, path, handler](AsyncWebServerRequest* request) {
                // Response will be sent in onBody callback
            },
            nullptr,  // onUpload
            [this, path, handler](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                // onBody callback
                HttpRequest req;
                req.method = HttpMethod::POST;
                req.path = path;
                req.body = std::string((char*)data, len);
                
                HttpResponse res;
                
                // Call user handler
                handler(req, res);
                
                // Send response
                request->send(res.status_code, res.content_type.c_str(), res.body.c_str());
                
                stats_.http_requests++;
            }
        );
    }
#endif
}

void WebServer::serveStatic(const std::string& path, const std::string& file_path) {
#ifdef NATIVE_BUILD
    (void)path;
    (void)file_path;
#else
    if (async_server_) {
        async_server_->serveStatic(path.c_str(), LittleFS, file_path.c_str());
    }
#endif
}

// ============================================================================
// WebSocket
// ============================================================================

void WebServer::onWebSocketConnect(std::function<void(uint32_t)> handler) {
    ws_connect_handler_ = handler;
}

void WebServer::onWebSocketDisconnect(std::function<void(uint32_t)> handler) {
    ws_disconnect_handler_ = handler;
}

void WebServer::onWebSocketMessage(std::function<void(const WebSocketMessage&)> handler) {
    ws_message_handler_ = handler;
}

void WebServer::broadcastWebSocket(const std::string& message) {
#ifdef NATIVE_BUILD
    (void)message;
    // Native build: just increment counter for testing
    stats_.websocket_broadcasts++;
#else
    if (async_websocket_) {
        async_websocket_->textAll(message.c_str());
        stats_.websocket_broadcasts++;
    }
#endif
}

void WebServer::sendWebSocket(uint32_t client_id, const std::string& message) {
#ifdef NATIVE_BUILD
    (void)client_id;
    (void)message;
#else
    if (async_websocket_) {
        async_websocket_->text(client_id, message.c_str());
    }
#endif
}

uint32_t WebServer::getWebSocketClientCount() const {
    return stats_.connected_clients;
}

void WebServer::broadcastBPM(float bpm, bool stable) {
    // Check rate limiting (AC-WEB-007: max 2Hz = 500ms interval)
    if (!canSendBPMUpdate()) {
        return;  // Rate limit exceeded, skip this update
    }
    
#ifdef NATIVE_BUILD
    (void)bpm;
    (void)stable;
    // Native build: simulate broadcast
    stats_.websocket_broadcasts++;
#else
    // Create JSON message
    DynamicJsonDocument doc(128);
    doc["bpm"] = bpm;
    doc["stable"] = stable;
    
    String message;
    serializeJson(doc, message);
    
    // Broadcast to all clients
    broadcastWebSocket(message.c_str());
#endif
    
    // Record timestamp for rate limiting
    recordBPMUpdate();
}

// ============================================================================
// Rate Limiting (AC-WEB-007)
// ============================================================================

bool WebServer::canSendBPMUpdate() {
    uint64_t now_us = micros();
    uint64_t elapsed_us = now_us - stats_.last_bpm_update_us;
    uint64_t rate_limit_us = config_.bpm_update_rate_limit_ms * 1000;
    
    return (elapsed_us >= rate_limit_us) || (stats_.last_bpm_update_us == 0);
}

void WebServer::recordBPMUpdate() {
    stats_.last_bpm_update_us = micros();
}

// ============================================================================
// Server Management
// ============================================================================

void WebServer::update() {
    // In ESPAsyncWebServer, all processing is async via callbacks
    // This method is for future extensions or periodic tasks
}

void WebServer::resetStats() {
    stats_.http_requests = 0;
    stats_.websocket_messages = 0;
    stats_.websocket_broadcasts = 0;
    stats_.last_bpm_update_us = 0;
    // Don't reset connected_clients as that's actual state
}

// ============================================================================
// Private Methods
// ============================================================================

uint64_t WebServer::micros() const {
#ifdef NATIVE_BUILD
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
#else
    return ::micros();
#endif
}

} // namespace clap_metronome
