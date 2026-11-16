#include "lights.h"
#include "effects.h"
#include "status_anim.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>

static Adafruit_NeoPixel strip(PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

static LightState state;

// color + level transition (all channels move together)
static RGB fromColor, toColor;
static float fromLevel = 0, toLevel = 0;
static uint32_t transT0 = 0;
static bool inTrans = false;

// effect crossfade
static uint8_t curEffect = 0, prevEffect = 0;
static uint32_t effectT0 = 0;
static bool effectFading = false;

static bool suppressed = true;
static uint32_t lastFrame = 0, lastShow = 0;
static uint32_t fxTime = 0;
static float fxFrac = 0;

static Pixel bufA[PIXELS], bufB[PIXELS], out[PIXELS], shown[PIXELS];

static float easeInOut(float p) {
  return p < 0.5f ? 2 * p * p : 1 - (-2 * p + 2) * (-2 * p + 2) / 2;
}

// 0-100 % -> output scale 0-1. Quadratic so the slider feels even; 1 % is still visible.
static float levelFor(uint8_t b) {
  if (b == 0) return 0;
  float x = (b - 1) / 99.0f;
  return (1.0f + (MAX_BRIGHTNESS - 1) * x * x) / 255.0f;
}

// 1-100 -> 0.25x .. 4x, 50 = 1x
static float speedMul(uint8_t s) {
  return powf(2.0f, ((int)s - 50) / 25.0f);
}

static void current(uint32_t now, RGB &c, float &lvl) {
  if (inTrans) {
    float p = (now - transT0) / (float)TRANSITION_MS;
    if (p < 1) {
      float e = easeInOut(p);
      c.r = fromColor.r + (toColor.r - fromColor.r) * e;
      c.g = fromColor.g + (toColor.g - fromColor.g) * e;
      c.b = fromColor.b + (toColor.b - fromColor.b) * e;
      lvl = fromLevel + (toLevel - fromLevel) * e;
      return;
    }
    inTrans = false;
  }
  c = toColor;
  lvl = toLevel;
}

// Start a transition from wherever we are now to the current state.
static void retarget(uint32_t now) {
  current(now, fromColor, fromLevel);
  toColor = {state.r, state.g, state.b};
  toLevel = (state.on && !suppressed) ? levelFor(state.brightness) : 0;
  transT0 = now;
  inTrans = true;
}

void lightsBegin(const LightState &initial) {
  strip.begin();
  strip.setBrightness(255);  // brightness is applied in software, never via setBrightness()
  strip.clear();
  strip.show();

  uint32_t now = millis();
  state = initial;
  if (state.effect >= EFFECT_COUNT) state.effect = 0;
  curEffect = prevEffect = state.effect;
  suppressed = statusSuppressLight(now);
  toColor = {state.r, state.g, state.b};
  toLevel = 0;
  inTrans = false;
  retarget(now);
  memset(shown, 0, sizeof(shown));
  lastFrame = lastShow = now;
}

const LightState &lightsGet() {
  return state;
}

void lightsSet(const LightState &s) {
  uint32_t now = millis();
  LightState n = s;
  if (n.effect >= EFFECT_COUNT) n.effect = state.effect;

  if (n.effect != curEffect) {
    prevEffect = curEffect;
    curEffect = n.effect;
    effectT0 = now;
    effectFading = true;
  }

  bool visual = n.on != state.on || n.r != state.r || n.g != state.g || n.b != state.b ||
                n.brightness != state.brightness;
  state = n;
  if (visual) retarget(now);
}

void lightsTick() {
  uint32_t now = millis();
  uint32_t dt = now - lastFrame;
  if (dt < FRAME_MS) return;
  lastFrame = now;
  if (dt > 100) dt = 100;

  bool sup = statusSuppressLight(now);
  if (sup != suppressed) {
    suppressed = sup;
    retarget(now);
  }

  RGB base;
  float level;
  current(now, base, level);

  // advance effect time by speed-scaled dt, keeping the fractional part
  float v = dt * speedMul(state.speed) + fxFrac;
  uint32_t vdt = (uint32_t)v;
  fxFrac = v - vdt;
  fxTime += vdt;

  if (level > 0) {
    FxCtx ctx = {fxTime, vdt, base};
    EFFECTS[curEffect].fn(ctx, bufA);

    if (effectFading) {
      float p = (now - effectT0) / (float)TRANSITION_MS;
      if (p >= 1) {
        effectFading = false;
      } else {
        EFFECTS[prevEffect].fn(ctx, bufB);
        uint16_t a = easeInOut(p) * 255;
        for (int i = 0; i < PIXELS; i++)
          for (int ch = 0; ch < 3; ch++)
            bufA[i][ch] = (bufB[i][ch] * (255 - a) + bufA[i][ch] * a) / 255;
      }
    }

    uint16_t lq = (uint16_t)(level * 255 + 0.5f);
    for (int i = 0; i < PIXELS; i++)
      for (int ch = 0; ch < 3; ch++)
        out[i][ch] = (bufA[i][ch] * lq + 127) / 255;
  } else {
    memset(out, 0, sizeof(out));
    effectFading = false;
  }

  statusRender(now, out, level);

  if (memcmp(out, shown, sizeof(out)) != 0 || now - lastShow >= REFRESH_MS) {
    for (int i = 0; i < PIXELS; i++) strip.setPixelColor(i, out[i][0], out[i][1], out[i][2]);
    strip.show();
    memcpy(shown, out, sizeof(out));
    lastShow = now;
  }
}

// ---------- JSON ----------

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool parseColor(const char *str, LightState &s) {
  if (!str) return false;
  if (*str == '#') str++;
  if (strlen(str) != 6) return false;
  uint8_t v[3];
  for (int i = 0; i < 3; i++) {
    int hi = hexVal(str[i * 2]), lo = hexVal(str[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    v[i] = hi * 16 + lo;
  }
  s.r = v[0];
  s.g = v[1];
  s.b = v[2];
  return true;
}

bool lightStateFromJson(JsonVariantConst v, LightState &s) {
  bool ok = true;
  if (v["on"].is<bool>()) s.on = v["on"].as<bool>();
  if (!v["color"].isNull() && !parseColor(v["color"].as<const char *>(), s)) ok = false;
  if (v["brightness"].is<int>()) s.brightness = constrain(v["brightness"].as<int>(), 0, 100);
  if (v["speed"].is<int>()) s.speed = constrain(v["speed"].as<int>(), 1, 100);
  if (v["effect"].is<int>()) {
    int e = v["effect"].as<int>();
    if (e >= 0 && e < EFFECT_COUNT)
      s.effect = e;
    else
      ok = false;
  }
  return ok;
}

void lightStateToJson(const LightState &s, JsonObject o) {
  char hex[8];
  snprintf(hex, sizeof(hex), "#%02x%02x%02x", s.r, s.g, s.b);
  o["on"] = s.on;
  o["color"] = hex;
  o["brightness"] = s.brightness;
  o["effect"] = s.effect;
  o["speed"] = s.speed;
}
