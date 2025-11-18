Excellent — **RTP-MIDI only** is a perfectly modern and clean direction, especially for FOH and network-centric rigs. I’ve **revised and adapted the entire design** accordingly.
(Technical references used: RTP-MIDI is MIDI over RTP, running over UDP/IP networks and its specification defines handling of timestamping, packet loss, and session management in RFC-4696.)

Here is your updated spec 👇

---

# 🎛 Updated MIDI Integration Specification

*(RTP-MIDI Only — No DIN / No USB for now)*

## 1️⃣ Functional Behavior 🔧

### Primary vs Secondary Input

* **Default mode: AUTO**

  * If **RTP-MIDI session active + valid Note On messages**
    → MIDI **primary**
  * If **no MIDI messages** for timeout
    → fallback to **Mic** (if enabled)
* Web UI setting:

  * `Mode = MIDI | Mic | Auto`

**Switching rules remain the same** — but now based on **network MIDI session presence**.

---

### MIDI Interface Support

* **RTP-MIDI via UDP/IP Wi-Fi**

  * RTP session setup (control + data ports)
  * Automatic Bonjour/mDNS session announcements **optional** (later)
* Peer may be:

  * DAW (Logic, Ableton, Cubase)
  * iPad/iPhone (built-in CoreMIDI)
  * AVB controller or network mixers

No hardware MIDI connectors required 🥳

> Benefit: Full electrical isolation inherently via network transport.

---

### Note Map Configuration

* **GM defaults**, user-adaptable:

  * Kick → 36
  * Snare → 38, 40
  * Hats → 42, 44, 46 (optional)
* Web UI: learn-mode “Hit Kick now → assigned”

---

### MIDI Channel Handling

* **Default: Accept all channels**
  → FOH sends can be from varied sources
* Option: filter to specific channel (configurable)

---

### Velocity Handling

Same logic:

* LED brightness = velocity scale
* Ignore hits below `VEL_MIN` (ghost note filtering)
* Minor weighting in tempo estimation

---

## 2️⃣ Boundary & Performance Ranges 📏

### Incoming RTP-MIDI Message Rate

Requirement:

> MUST process **≥100 Note On/sec** without loss

Rationale:
Network bursts + rapid rolls still safe.

---

### Fallback Timeout (To Mic Mode)

* Configurable: **500–2000 ms**
* Default: **1000 ms**
* Timer resets on every new valid MIDI Note On

---

### Buffer Strategy

* **Ring buffer 32–64 events**
* Tempo estimation using last 8–16 Kick-role hits

---

## 3️⃣ Error Handling & Validation ⚠️

### RTP-MIDI Specifics

* Drop malformed or incomplete RTP packets
* Validate MIDI payload extracted from each packet
* Gracefully handle:

  * Out-of-order packets
  * Missing packets (compensation via timestamp fields)
    *Standard RTP-MIDI behavior*

### Tempo Stability

If irregular:

* Display “Waiting for stable tempo…”
* Continue serving last stable tempo for **2–4 bars**
* No visible tempo jumps

---

### Session Loss

If **no RTP-MIDI session** (socket timeout):
1️⃣ Keep internal tempo running
2️⃣ Indicate **MIDI LOST** visually
3️⃣ In AUTO: switch to Mic **after** fallback timeout

---

## 4️⃣ Performance & Timing ⚡

### Latency Target

> MIDI Note On → visual beat pulse < **2 ms** typical
> < **5 ms** upper bound guaranteed

Tempo smoothing continues in parallel → **musical stability**.

---

## 5️⃣ Security Considerations 🔒

* UDP packet size validation (protect parser)
* Rate-limit processing per loop to prevent floods
* Only accept RTP-MIDI on **specific UDP port** (configurable)
* Optionally: restrict trusted IP ranges (future)

---

## 6️⃣ Integration & Dependencies 🔗

Applies cleanly to:

| System                                 | Status            |
| -------------------------------------- | ----------------- |
| Relay Tap Output                       | ✔ fully supported |
| WebSocket BPM Telemetry                | ✔                 |
| MQTT Telemetry (future FOH dashboards) | ✔                 |
| DMX/ArtNet Beat Sync                   | ✔                 |

### MIDI Thru/Merge

Because transport is network-based:

* **RTP-MIDI forward/bridge** optional
* **MIDI Clock** generation stays **local timing master**

---

### DAW Sync

* DAW → hit messages for tempo follow
* No MTC/MIDI Clock input needed initially
* Possible future remote control:

  * Start/Stop
  * Cue-advance events

---

## 7️⃣ UX & Accessibility ♿

### Mode Indication

* LED:

  * Blue = MIDI
  * Green = Mic
  * Yellow = Auto
* Web UI:

  ```
  Input Source: RTP-MIDI (Stable)
  Last Note: 36 (Kick)
  Velocity: 102
  Peers: MacBook Pro (Logic Pro)
  ```

### Auto-Discovery (future)

* Bonjour/mDNS for session advertisements like macOS/iOS RTP-MIDI

---

## 8️⃣ Compliance & Protocol Scope 📋

* Implement **core** RTP-MIDI spec (MIDI 1.0 payload)
* **Ignore**:

  * SysEx
  * MIDI Machine Control
  * MIDI Time Code
* Ensure **robust session negotiation** and timeout handling

---

# 🧠 What This Unlocks

✔ Cleaner system (no optocouplers, no extra PCB cost)
✔ Zero ground-loop risk (huge for FOH!)
✔ Fully networked show integration
✔ Closer alignment with **AVB**, Dante, Milan workflows

Your device becomes a **network-native tempo authority** driven by the drummer 🎯

---

