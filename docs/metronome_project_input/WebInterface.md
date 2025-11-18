adding a **Web Interface** changes the design priorities a bit and enables *very practical* FOH workflows:

* You don’t need to access the box physically during a show
* You can monitor BPM & lock-status from the FOH mixer position
* You can remotely adjust sensitivity if the band suddenly gets louder
* You can switch between input modes (kick mic vs ambient mic)
* You can enable/disable tap-tempo output live

---

# 🌐 Web Interface — Best-Practice FOH Feature Set

### **Live Monitoring**

* Display **Current BPM** (big + high contrast)
* Tempo stability indicator (green = locked / yellow = tracking / red = lost)
* “Confidence” % for beat detection
* Visual beat LED (just a simple pulse square on screen)

### **Controls**

* Sensitivity slider (input threshold)
* Band selection (Kick / Full spectrum)
* BPM smoothing aggressiveness
* Min/Max BPM range (safe bounds)
* Output mode:

  * MIDI Clock
  * MIDI Tap Tempo
  * Relay tap output
  * ArtNet/DMX pulse

### **Advanced FOH Tools**

* **Tap override** button (manual control if drummer drifts)
* “Hold BPM” mode (freeze detection temporarily)
* “Rapid Lock” mode (aggressive tempo update)
* Reset lock / auto-relock button

### **System Page**

* Wi-Fi config
* Mic calibration helper (peak/avg bars)
* Firmware version + update (OTA update possible)

---

# 📱 UI Layout Example (Mobile-Friendly)

```
┌─────────────────────────┐
│ BPM: 124.7  [LOCKED]    │
│ ● beat pulse indicator   │
├─ Sensitivity [==-----]   │
├─ Input Mode: [Kick Only] │
├─ Output: [MIDI TAP]      │
├─ CONFIDENCE: 92%         │
│ [Tap override] [Freeze]  │
└─────────────────────────┘
```

Works on a phone or tablet — **optimized for dark FOH conditions**.

---

# ⚙️ Technical Architecture

## Web UI Technology

✔ ESP8266 Async Web Server
✔ WebSockets for **real-time beat pulse** & BPM updates
✔ Responsive layout (Bootstrap or simple CSS Grid)

## Protocols Supported

* HTTP for control
* WebSocket for streaming detection updates

## Timing Protection

Critical loop never blocked by:

* HTTP requests
* Wi-Fi retries
* Browser slowdowns

→ Beat detection remains **priority**.

---

# 🧩 Example Data Flow

```
ADC → Onset Detection → BPM Tracker
                      ↓
               Real-time state → WebSocket → Browser UI
                      ↓
               Beat output → MIDI/Relay/DMX
```

ESP8266 still stays focused on audio timing.

---

# 🔒 FOH Safety Features to Include

| Feature                            | Why                                  |
| ---------------------------------- | ------------------------------------ |
| Local pushbutton “Mute Tap Output” | FOH emergency fallback               |
| Graceful fallback BPM              | If beat lost, keep last stable tempo |
| Wi-Fi access point mode            | Standalone use backstage             |
| Optional HTTPS disabled            | ESP8266 too weak for SSL             |

---

