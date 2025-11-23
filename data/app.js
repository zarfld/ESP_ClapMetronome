// ESP Clap Metronome - Web UI JavaScript
// Handles REST API communication, WebSocket telemetry, and UI updates

class MetronomeUI {
    constructor() {
        this.ws = null;
        this.wsReconnectTimer = null;
        this.statusUpdateTimer = null;
        this.config = {};
        
        // Initialize on page load
        this.init();
    }

    init() {
        console.log('Initializing Metronome UI...');
        
        // Setup event listeners
        this.setupEventListeners();
        
        // Load configuration
        this.loadConfig();
        
        // Connect WebSocket
        this.connectWebSocket();
        
        // Start status polling (fallback if WebSocket fails)
        this.startStatusPolling();
    }

    // ========================================================================
    // WebSocket Communication
    // ========================================================================

    connectWebSocket() {
        const wsUrl = `ws://${window.location.hostname}/ws`;
        console.log(`Connecting to WebSocket: ${wsUrl}`);
        
        try {
            this.ws = new WebSocket(wsUrl);
            
            this.ws.onopen = () => {
                console.log('WebSocket connected');
                document.getElementById('ws-status').textContent = 'Connected';
                document.getElementById('ws-status').classList.remove('disconnected');
            };
            
            this.ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    this.handleTelemetryUpdate(data);
                } catch (e) {
                    console.error('Failed to parse WebSocket message:', e);
                }
            };
            
            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
            };
            
            this.ws.onclose = () => {
                console.log('WebSocket disconnected');
                document.getElementById('ws-status').textContent = 'Disconnected';
                document.getElementById('ws-status').classList.add('disconnected');
                
                // Attempt reconnection after 5 seconds
                this.wsReconnectTimer = setTimeout(() => {
                    this.connectWebSocket();
                }, 5000);
            };
        } catch (e) {
            console.error('Failed to create WebSocket:', e);
        }
    }

    handleTelemetryUpdate(data) {
        // Update BPM display
        if (data.bpm !== undefined) {
            document.getElementById('bpm-value').textContent = data.bpm.toFixed(1);
        }
        
        // Update confidence
        if (data.confidence !== undefined) {
            document.getElementById('confidence').textContent = `${data.confidence}%`;
        }
        
        // Update lock status
        if (data.lock_status) {
            const lockElement = document.getElementById('lock-status');
            lockElement.querySelector('.lock-text').textContent = data.lock_status;
            lockElement.className = `lock-status ${data.lock_status.toLowerCase()}`;
        }
        
        // Update beat count
        if (data.beat_count !== undefined) {
            document.getElementById('beat-count').textContent = data.beat_count;
        }
        
        // Update false positives
        if (data.false_positives !== undefined) {
            document.getElementById('false-positives').textContent = data.false_positives;
        }
        
        // Update audio levels
        if (data.audio) {
            const audioPercent = (data.audio.current / 4095) * 100;
            document.getElementById('audio-level').style.width = `${audioPercent}%`;
            document.getElementById('audio-value').textContent = data.audio.current;
            document.getElementById('audio-min').textContent = data.audio.min || 0;
            document.getElementById('audio-max').textContent = data.audio.max || 0;
            
            if (data.audio.threshold) {
                const thresholdPercent = (data.audio.threshold / 4095) * 100;
                document.getElementById('threshold-marker').style.left = `${thresholdPercent}%`;
                document.getElementById('threshold-value').textContent = data.audio.threshold;
            }
        }
        
        // Update gain level
        if (data.gain_level !== undefined) {
            document.getElementById('gain-level').textContent = `${data.gain_level}dB`;
        }
        
        // Update uptime
        if (data.uptime_sec !== undefined) {
            document.getElementById('uptime').textContent = this.formatUptime(data.uptime_sec);
        }
    }

    // ========================================================================
    // REST API Communication
    // ========================================================================

    async loadConfig() {
        try {
            const response = await fetch('/api/config');
            if (!response.ok) throw new Error('Failed to load config');
            
            this.config = await response.json();
            this.updateConfigForm();
            console.log('Configuration loaded:', this.config);
        } catch (e) {
            console.error('Failed to load configuration:', e);
            this.showNotification('Failed to load configuration', 'error');
        }
    }

    async saveConfig() {
        try {
            this.collectConfigFromForm();
            
            const response = await fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(this.config)
            });
            
            if (!response.ok) throw new Error('Failed to save config');
            
            const result = await response.json();
            console.log('Configuration saved:', result);
            this.showNotification('Configuration saved successfully', 'success');
            
            // Reload to confirm
            setTimeout(() => this.loadConfig(), 1000);
        } catch (e) {
            console.error('Failed to save configuration:', e);
            this.showNotification('Failed to save configuration', 'error');
        }
    }

    async loadStatus() {
        try {
            const response = await fetch('/api/status');
            if (!response.ok) throw new Error('Failed to load status');
            
            const status = await response.json();
            this.updateStatusDisplay(status);
        } catch (e) {
            console.error('Failed to load status:', e);
        }
    }

    async factoryReset() {
        if (!confirm('Are you sure you want to reset to factory defaults? This will restart the device.')) {
            return;
        }
        
        try {
            const response = await fetch('/api/reset', { method: 'POST' });
            if (!response.ok) throw new Error('Factory reset failed');
            
            this.showNotification('Factory reset complete. Device restarting...', 'success');
            
            // Reload page after 5 seconds
            setTimeout(() => window.location.reload(), 5000);
        } catch (e) {
            console.error('Factory reset failed:', e);
            this.showNotification('Factory reset failed', 'error');
        }
    }

    // ========================================================================
    // UI Update Methods
    // ========================================================================

    updateConfigForm() {
        // Audio config
        if (this.config.audio) {
            document.getElementById('sample-rate').value = this.config.audio.sample_rate || 8000;
            document.getElementById('threshold-margin').value = this.config.audio.threshold_margin || 80;
            document.getElementById('debounce-ms').value = this.config.audio.debounce_ms || 50;
            document.getElementById('gain-level').value = this.config.audio.gain_level || 50;
            document.getElementById('kick-only-mode').checked = this.config.audio.kick_only_mode || false;
        }
        
        // BPM config
        if (this.config.bpm) {
            document.getElementById('min-bpm').value = this.config.bpm.min_bpm || 40;
            document.getElementById('max-bpm').value = this.config.bpm.max_bpm || 300;
            document.getElementById('stability-threshold').value = this.config.bpm.stability_threshold || 5;
            document.getElementById('tempo-correction').checked = this.config.bpm.tempo_correction_enabled !== false;
        }
        
        // Output config
        if (this.config.output) {
            document.getElementById('midi-enabled').checked = this.config.output.midi_enabled !== false;
            document.getElementById('midi-channel').value = this.config.output.midi_channel || 10;
            document.getElementById('relay-enabled').checked = this.config.output.relay_enabled || false;
            document.getElementById('relay-pulse-ms').value = this.config.output.relay_pulse_ms || 50;
        }
        
        // Network config
        if (this.config.network) {
            document.getElementById('wifi-ssid').value = this.config.network.ssid || '';
            document.getElementById('mqtt-enabled').checked = this.config.network.mqtt_enabled || false;
            document.getElementById('mqtt-broker').value = this.config.network.mqtt_broker || '';
            document.getElementById('mqtt-port').value = this.config.network.mqtt_port || 1883;
            document.getElementById('mqtt-user').value = this.config.network.mqtt_user || '';
            document.getElementById('device-id').value = this.config.network.device_id || 'esp32-metronome';
        }
    }

    collectConfigFromForm() {
        // Audio config
        this.config.audio = {
            sample_rate: parseInt(document.getElementById('sample-rate').value),
            threshold_margin: parseInt(document.getElementById('threshold-margin').value),
            debounce_ms: parseInt(document.getElementById('debounce-ms').value),
            gain_level: parseInt(document.getElementById('gain-level').value),
            kick_only_mode: document.getElementById('kick-only-mode').checked
        };
        
        // BPM config
        this.config.bpm = {
            min_bpm: parseInt(document.getElementById('min-bpm').value),
            max_bpm: parseInt(document.getElementById('max-bpm').value),
            stability_threshold: parseInt(document.getElementById('stability-threshold').value),
            tempo_correction_enabled: document.getElementById('tempo-correction').checked
        };
        
        // Output config
        this.config.output = {
            midi_enabled: document.getElementById('midi-enabled').checked,
            midi_channel: parseInt(document.getElementById('midi-channel').value),
            relay_enabled: document.getElementById('relay-enabled').checked,
            relay_pulse_ms: parseInt(document.getElementById('relay-pulse-ms').value)
        };
        
        // Network config
        this.config.network = {
            ssid: document.getElementById('wifi-ssid').value,
            mqtt_enabled: document.getElementById('mqtt-enabled').checked,
            mqtt_broker: document.getElementById('mqtt-broker').value,
            mqtt_port: parseInt(document.getElementById('mqtt-port').value),
            mqtt_user: document.getElementById('mqtt-user').value,
            device_id: document.getElementById('device-id').value
        };
        
        // Include passwords only if they're not empty
        const wifiPassword = document.getElementById('wifi-password').value;
        if (wifiPassword) {
            this.config.network.password = wifiPassword;
        }
        
        const mqttPassword = document.getElementById('mqtt-password').value;
        if (mqttPassword) {
            this.config.network.mqtt_password = mqttPassword;
        }
    }

    updateStatusDisplay(status) {
        // Update output states
        if (status.midi_state) {
            const midiState = document.getElementById('midi-state');
            midiState.textContent = status.midi_state;
            midiState.className = status.midi_state === 'RUNNING' ? 'output-state active' : 'output-state';
        }
        
        if (status.relay_state) {
            const relayState = document.getElementById('relay-state');
            relayState.textContent = status.relay_state;
            relayState.className = status.relay_state === 'ON' ? 'output-state active' : 'output-state';
        }
        
        if (status.mqtt_connected !== undefined) {
            const mqttState = document.getElementById('mqtt-state');
            mqttState.textContent = status.mqtt_connected ? 'CONNECTED' : 'DISCONNECTED';
            mqttState.className = status.mqtt_connected ? 'output-state active' : 'output-state';
        }
    }

    // ========================================================================
    // Event Listeners
    // ========================================================================

    setupEventListeners() {
        // Tab switching
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const tabName = e.target.dataset.tab;
                this.switchTab(tabName);
            });
        });
        
        // Config buttons
        document.getElementById('refresh-btn').addEventListener('click', () => {
            this.loadConfig();
        });
        
        document.getElementById('save-btn').addEventListener('click', () => {
            this.saveConfig();
        });
        
        document.getElementById('reset-btn').addEventListener('click', () => {
            this.factoryReset();
        });
    }

    switchTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
        
        // Update tab content
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.remove('active');
        });
        document.getElementById(`${tabName}-tab`).classList.add('active');
    }

    // ========================================================================
    // Utility Methods
    // ========================================================================

    startStatusPolling() {
        // Poll status every 5 seconds (backup if WebSocket fails)
        this.statusUpdateTimer = setInterval(() => {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
                this.loadStatus();
            }
        }, 5000);
    }

    formatUptime(seconds) {
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;
        return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }

    showNotification(message, type = 'info') {
        // Simple notification system (can be enhanced with a proper toast library)
        const notification = document.createElement('div');
        notification.textContent = message;
        notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            background: ${type === 'error' ? '#f44336' : type === 'success' ? '#4CAF50' : '#2196F3'};
            color: white;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
            z-index: 10000;
            animation: slideIn 0.3s ease;
        `;
        
        document.body.appendChild(notification);
        
        setTimeout(() => {
            notification.style.animation = 'slideOut 0.3s ease';
            setTimeout(() => notification.remove(), 300);
        }, 3000);
    }
}

// Initialize UI when page loads
let metronomeUI;
document.addEventListener('DOMContentLoaded', () => {
    metronomeUI = new MetronomeUI();
});

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (metronomeUI && metronomeUI.ws) {
        metronomeUI.ws.close();
    }
    if (metronomeUI && metronomeUI.statusUpdateTimer) {
        clearInterval(metronomeUI.statusUpdateTimer);
    }
    if (metronomeUI && metronomeUI.wsReconnectTimer) {
        clearTimeout(metronomeUI.wsReconnectTimer);
    }
});
