#pragma once
#include <Arduino.h>

// ---------- Hardware ----------
#define LED_PIN D8
#define PIXELS 72
// Output cap (0-255) applied at 100 % brightness. 72 px at full white draw ~4.3 A;
// 200 keeps that around 3.4 A. Raise only if the PSU can handle it.
#define MAX_BRIGHTNESS 200

// ---------- WiFi ----------
#define STA_SSID "wemos"
#define STA_PSK "wemosD1R2"
#define HOSTNAME "moodlamp"           // -> http://moodlamp.local
#define AP_SSID_PREFIX "MoodLamp"     // fallback hotspot: MoodLamp-XXXX
#define AP_PASS "moodlamp"            // min. 8 characters
#define AP_FALLBACK_MS 30000UL        // no WiFi for this long -> open hotspot
#define AP_IDLE_OFF_MS 60000UL        // back on WiFi + hotspot unused this long -> close hotspot
#define STA_KICK_MS 60000UL           // re-issue WiFi.begin() this often while disconnected

// ---------- Timing ----------
#define FRAME_MS 16                   // ~60 fps
#define TRANSITION_MS 600             // color / brightness / effect crossfade
#define REFRESH_MS 2000               // re-send an unchanged frame this often (glitch recovery)
#define SAVE_DEBOUNCE_MS 3000UL       // write settings after this much quiet time
#define STATUS_MIN_LEVEL 0.30f        // status animations are at least this bright (0-1)

struct RGB {
  uint8_t r, g, b;
};
typedef uint8_t Pixel[3];
