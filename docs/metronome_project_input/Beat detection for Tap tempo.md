That’s a **very specific** and **very practical** FOH use-case:

> **Beat detection so the sound engineer can feed Tap-Tempo to delay effects**
> — without constantly tapping the pedal/button during the show.

This is **a simpler** and more robust scenario than full lighting grid sync:

* We don’t need to detect meter or downbeats
* We only need a **stable BPM**
* Phase is unimportant → the audience only hears **delay timing**, not beat alignment

So you can focus on **tempo only**, which is ideal for an ESP8266-class system.

---

# 🧠 FOH-Engineer Workflow & Requirements

### Typical FOH requirements:

| Requirement                   | Why                                 |
| ----------------------------- | ----------------------------------- |
| Accurate BPM (±1–2 BPM)       | Delays must “groove” with mix       |
| Smooth averaging              | Avoid delay “flams” from jitter     |
| Noise / bleed resilience      | Mic hears everything loud           |
| Quick lock (2–4 beats)        | They don’t want to wait             |
| Bypass / manual mode          | FOH must remain in control          |
| No hunting on fills           | Delay must be stable                |
| Ability to ignore tempo drops | Meshuggah breakdowns are allowed 😂 |

---

# 🎯 What beat detection removes for FOH

The FOH engineer does **NOT** want:

* Downbeat marking
* Subdivision pulses
* Visual drummer training
* Full measure detection

They only need:

> **Predictable, stable continuous BPM number**
> → outputting `Tap Tempo` messages to delay processors

---

# 🔌 Output Integration

3 common ways to send the tempo:

| Method                      | Use case                                                  |
| --------------------------- | --------------------------------------------------------- |
| **MIDI Clock**              | Digital mixers, outboard FX (PCM70, H3000, Strymon, etc.) |
| **MIDI Tap message**        | Simulating pushbutton tap inputs                          |
| **Footswitch relay output** | For analog delay pedals or tap-input jacks                |

Example:

```
Each detected beat → send a short relay closure on "tap" output
```

With smoothing so you simulate a **steady tap**.

---

# 🎛️ Stability Algorithm Optimized for FOH

```
Mic -> Transient detector -> Onset timestamps
                        ↓
         Median filter or IOI window filtering (6–12 last beats)
                        ↓
              BPM estimation (updated gradually)
                        ↓
           Predict beat times in near future → Tap
```

Recommendations:

| Component  | FOH-friendly config           |
| ---------- | ----------------------------- |
| Window     | Last 6–10 beats               |
| Estimator  | Weighted median of IOIs       |
| Correction | Max +/- 1 BPM per beat        |
| Lock range | 70–180 BPM (typical pop/rock) |

Fade-in mode:

* Detection locks after **3 solid beats**
* Output ignores tempo during breaks/fills
* Optional: **kick-only** detection preferred

---

# 🚧 Hard environments & mitigation

| Situation                         | Solution                                                      |
| --------------------------------- | ------------------------------------------------------------- |
| Room mic & LOUD PA                | **Low-frequency band only** (kick mic if possible)            |
| Fast fills                        | Confidence-based ignore                                       |
| Tempo jumps (punk / metal breaks) | Slow smoothing, but allow steep change with strong confidence |
| Subkick rumble                    | HPF at ~40 Hz                                                 |

This prioritizes **kick** signals → most reliable anchor.

---

# ✔ ESP8266 Implementation Feasible

You only need:
✔ Adaptive envelope threshold
✔ Onset timestamp list
✔ IOI smoothing
✔ Tap-output (relay or MIDI)

No FFT, no classification, no complex DSP.

💡 The DS3231 RTC you mentioned = excellent for stable timestamping.

---

# 📌 User Experience for FOH:

LED feedback:

* One LED = detected beat
* One LED = tempo lock status
* One LED = fallback/manual mode

Front panel:

* “LOCK” means BPM stable
* One knob:

  * Sensitivity threshold
  * Band filter mode (Kick / Full)

Optional:

* Small OLED display showing BPM

---

# ✨ TL;DR

For **tap-tempo delay sync**:

> **Detect kick beats only**
>
> * median-smoothed BPM
> * controlled tap output
>   → perfect for FOH usage

No meter, no downbeat, no subdivisions required.
