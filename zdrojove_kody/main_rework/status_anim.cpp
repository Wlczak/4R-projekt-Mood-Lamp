#include "status_anim.h"
#include <math.h>

// Persistent animations loop while a state lasts; one-shots play once on a state change.
enum Persist : uint8_t { P_NONE, P_COMETS, P_HEARTBEAT, P_AP_BREATHE };
enum Shot : uint8_t { S_NONE, S_RIPPLE, S_AMBER, S_SWEEP };

static const float TWO_PI_F = 6.2831853f;
static const float PI_F = 3.1415927f;
static const uint32_t FADE_MS = 300;

static const RGB CYAN = {0, 190, 255};
static const RGB GREEN = {0, 255, 70};
static const RGB MINT = {160, 255, 190};
static const RGB AMBER = {255, 100, 0};
static const RGB PURPLE = {150, 0, 255};

static Persist pCur = P_NONE, pPrev = P_NONE;
static uint32_t pCurFadeT0 = 0, pCurAnimT0 = 0;
static uint32_t pPrevFadeT0 = 0, pPrevAnimT0 = 0;

static Shot shot = S_NONE;
static uint32_t shotT0 = 0;

static bool booting = true;
static uint32_t bootEndAt = 0;

static inline float clamp01(float x) {
  return x < 0 ? 0 : (x > 1 ? 1 : x);
}

static inline float phase(uint32_t t, uint32_t period) {
  return (t % period) / (float)period;
}

static float easeInOut(float p) {
  return p < 0.5f ? 2 * p * p : 1 - (-2 * p + 2) * (-2 * p + 2) / 2;
}

// Blend color c (scaled by lvl) over pixel p with alpha a.
static void blend(Pixel &p, const RGB &c, float a, float lvl) {
  a = clamp01(a);
  if (a <= 0) return;
  p[0] = p[0] * (1 - a) + c.r * lvl * a;
  p[1] = p[1] * (1 - a) + c.g * lvl * a;
  p[2] = p[2] * (1 - a) + c.b * lvl * a;
}

static RGB mix(const RGB &x, const RGB &y, float k) {
  return {(uint8_t)(x.r + (y.r - x.r) * k), (uint8_t)(x.g + (y.g - x.g) * k), (uint8_t)(x.b + (y.b - x.b) * k)};
}

// Small glow on the 3 outermost pixels at both ends of the strip.
static void ends(Pixel *out, const RGB &c, float a, float lvl) {
  static const float fall[3] = {1.0f, 0.55f, 0.25f};
  for (int j = 0; j < 3; j++) {
    blend(out[j], c, a * fall[j], lvl);
    blend(out[PIXELS - 1 - j], c, a * fall[j], lvl);
  }
}

// ---------- persistent ----------

// Two cyan comets orbiting in opposite directions, gently breathing.
static void drawComets(uint32_t t, float a, Pixel *out, float lvl) {
  const float tail = 10.0f;
  float h1 = phase(t, 1200) * PIXELS;
  float h2 = PIXELS - h1;
  float breathe = 0.75f + 0.25f * sinf(TWO_PI_F * phase(t, 2000));
  for (int i = 0; i < PIXELS; i++) {
    float d1 = fmodf(h1 - i + PIXELS, PIXELS);
    float d2 = fmodf(i - h2 + PIXELS, PIXELS);
    float k1 = d1 < tail ? 1 - d1 / tail : 0;
    float k2 = d2 < tail ? 1 - d2 / tail : 0;
    float k = max(k1 * k1, k2 * k2);
    blend(out[i], CYAN, k * breathe * a, lvl);
  }
}

// Double "heartbeat" on the strip ends while reconnecting.
static void drawHeartbeat(uint32_t t, float a, Pixel *out, float lvl) {
  uint32_t x = t % 1600;
  float k = 0;
  if (x < 180) k = sinf(PI_F * x / 180.0f);
  else if (x >= 260 && x < 440) k = 0.7f * sinf(PI_F * (x - 260) / 180.0f);
  ends(out, AMBER, 0.7f * k * a, lvl);
}

static void drawApBreathe(uint32_t t, float a, Pixel *out, float lvl) {
  float k = 0.2f + 0.8f * (0.5f - 0.5f * cosf(TWO_PI_F * phase(t, 3000)));
  ends(out, PURPLE, k * a, lvl);
}

static void drawPersist(Persist m, uint32_t t, float a, Pixel *out, float lvl) {
  switch (m) {
    case P_COMETS: drawComets(t, a, out, lvl); break;
    case P_HEARTBEAT: drawHeartbeat(t, a, out, lvl); break;
    case P_AP_BREATHE: drawApBreathe(t, a, out, lvl); break;
    case P_NONE: break;
  }
}

// ---------- one-shots ----------

static uint32_t shotDuration(Shot s) {
  switch (s) {
    case S_RIPPLE: return 1650;
    case S_AMBER: return 3000;
    case S_SWEEP: return 1100;
    case S_NONE: break;
  }
  return 0;
}

// Green ripple from the center outwards, a soft mint flash, then fade out.
static void drawRipple(uint32_t t, Pixel *out, float lvl) {
  const float center = (PIXELS - 1) / 2.0f;
  if (t < 500) {
    float r = (t / 500.0f) * (PIXELS / 2.0f + 4);
    for (int i = 0; i < PIXELS; i++) blend(out[i], GREEN, (r - fabsf(i - center)) / 4.0f, lvl);
  } else if (t < 650) {
    RGB c = mix(GREEN, MINT, (t - 500) / 150.0f);
    for (int i = 0; i < PIXELS; i++) blend(out[i], c, 1, lvl);
  } else {
    float a = 1 - easeInOut(clamp01((t - 650) / 1000.0f));
    for (int i = 0; i < PIXELS; i++) blend(out[i], MINT, a, lvl);
  }
}

// Three slow amber pulses over the whole strip (light stays visible underneath).
static void drawAmber(uint32_t t, Pixel *out, float lvl) {
  float s = sinf(PI_F * phase(t, 1000));
  for (int i = 0; i < PIXELS; i++) blend(out[i], AMBER, 0.45f * s * s, lvl);
}

// Purple band sweeping across the strip.
static void drawSweep(uint32_t t, Pixel *out, float lvl) {
  float c = -12 + (PIXELS + 24) * (t / 1100.0f);
  for (int i = 0; i < PIXELS; i++) blend(out[i], PURPLE, 1 - fabsf(i - c) / 6.0f, lvl);
}

static void drawShot(Shot s, uint32_t t, Pixel *out, float lvl) {
  switch (s) {
    case S_RIPPLE: drawRipple(t, out, lvl); break;
    case S_AMBER: drawAmber(t, out, lvl); break;
    case S_SWEEP: drawSweep(t, out, lvl); break;
    case S_NONE: break;
  }
}

// ---------- state ----------

static void setPersist(Persist m, uint32_t now) {
  if (m == pCur) return;
  pPrev = pCur;
  pPrevFadeT0 = now;
  pPrevAnimT0 = pCurAnimT0;
  pCur = m;
  pCurFadeT0 = now;
  pCurAnimT0 = now;
}

static void playShot(Shot s, uint32_t now) {
  shot = s;
  shotT0 = now;
}

void statusBegin() {
  setPersist(P_COMETS, millis());
}

void statusOnNet(NetState s) {
  uint32_t now = millis();
  switch (s) {
    case NET_CONNECTING:
      setPersist(P_COMETS, now);
      break;
    case NET_CONNECTED:
      setPersist(P_NONE, now);
      playShot(S_RIPPLE, now);
      if (booting) {
        booting = false;
        bootEndAt = now + 650;  // light fades in while the flash fades out
      }
      break;
    case NET_LOST:
      setPersist(P_HEARTBEAT, now);
      playShot(S_AMBER, now);
      break;
    case NET_AP:
      setPersist(P_AP_BREATHE, now);
      playShot(S_SWEEP, now);
      if (booting) {
        booting = false;
        bootEndAt = now + 900;
      }
      break;
  }
}

bool statusSuppressLight(uint32_t now) {
  return booting || (int32_t)(bootEndAt - now) > 0;
}

void statusRender(uint32_t now, Pixel *out, float level) {
  float lvl = max(level, STATUS_MIN_LEVEL);

  bool shotActive = shot != S_NONE && now - shotT0 < shotDuration(shot);
  if (shot != S_NONE && !shotActive) {
    shot = S_NONE;
    pCurFadeT0 = now;  // persistent animation fades in after the one-shot
  }

  if (pPrev != P_NONE) {
    uint32_t e = now - pPrevFadeT0;
    if (e >= FADE_MS)
      pPrev = P_NONE;
    else
      drawPersist(pPrev, now - pPrevAnimT0, 1 - e / (float)FADE_MS, out, lvl);
  }

  if (shotActive) {
    pCurFadeT0 = now;
    drawShot(shot, now - shotT0, out, lvl);
  } else if (pCur != P_NONE) {
    drawPersist(pCur, now - pCurAnimT0, clamp01((now - pCurFadeT0) / (float)FADE_MS), out, lvl);
  }
}
