// Non-blocking ports of the effects from effect_test.ino.
// Every effect only fills the frame buffer for time ctx.t - no delay(), no show().
#include "effects.h"
#include <math.h>

static const float TWO_PI_F = 6.2831853f;

static inline float phase(uint32_t t, uint32_t period) {
  return (t % period) / (float)period;
}

static inline void setPx(Pixel &p, uint8_t r, uint8_t g, uint8_t b) {
  p[0] = r;
  p[1] = g;
  p[2] = b;
}

static inline void putScaled(Pixel &p, const RGB &c, float k) {
  if (k <= 0) {
    setPx(p, 0, 0, 0);
    return;
  }
  if (k > 1) k = 1;
  setPx(p, c.r * k, c.g * k, c.b * k);
}

static RGB wheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) return {(uint8_t)(255 - pos * 3), 0, (uint8_t)(pos * 3)};
  if (pos < 170) {
    pos -= 85;
    return {0, (uint8_t)(pos * 3), (uint8_t)(255 - pos * 3)};
  }
  pos -= 170;
  return {(uint8_t)(pos * 3), (uint8_t)(255 - pos * 3), 0};
}

static void fadeBuf(Pixel *b, uint8_t amount) {
  for (int i = 0; i < PIXELS; i++)
    for (int c = 0; c < 3; c++)
      b[i][c] = b[i][c] > amount ? b[i][c] - amount : 0;
}

static void copyBuf(Pixel *dst, Pixel *src) {
  memcpy(dst, src, sizeof(Pixel) * PIXELS);
}

// Runs fixed-size simulation steps for the stateful effects.
static uint16_t takeSteps(uint32_t &acc, uint32_t dt, uint32_t stepMs) {
  acc += dt;
  if (acc > stepMs * 10) acc = stepMs * 10;  // don't try to catch up after a long stall
  uint16_t n = acc / stepMs;
  acc -= n * stepMs;
  return n;
}

// ---------------------------------------------------------------------------

static void fxSolid(const FxCtx &c, Pixel *f) {
  for (int i = 0; i < PIXELS; i++) setPx(f[i], c.base.r, c.base.g, c.base.b);
}

static void fxBreathe(const FxCtx &c, Pixel *f) {
  float k = 0.12f + 0.88f * (0.5f - 0.5f * cosf(TWO_PI_F * phase(c.t, 4000)));
  for (int i = 0; i < PIXELS; i++) putScaled(f[i], c.base, k);
}

static void fxColorWave(const FxCtx &c, Pixel *f) {
  float p = phase(c.t, 3000);
  for (int i = 0; i < PIXELS; i++) {
    float k = 0.2f + 0.8f * (0.5f + 0.5f * sinf(TWO_PI_F * (i / (PIXELS / 2.0f) - p)));
    putScaled(f[i], c.base, k);
  }
}

static void fxRainbow(const FxCtx &c, Pixel *f) {
  uint8_t off = (c.t / 20) & 255;
  for (int i = 0; i < PIXELS; i++) {
    RGB w = wheel((i * 256 / PIXELS + off) & 255);
    setPx(f[i], w.r, w.g, w.b);
  }
}

static void fxRainbowFire(const FxCtx &c, Pixel *f) {
  uint32_t tp = (uint32_t)((uint64_t)c.t * 18 / 100);
  float br = (sinf(TWO_PI_F * phase(c.t, 1745)) + 1.0f) * 0.4f + 0.2f;
  for (int i = 0; i < PIXELS; i++) {
    int pos = (i * 10 + tp) & 255;
    float r = (sinf(pos * 0.024f) * 127 + 128) * br;
    float g = (sinf((pos + 85) * 0.024f) * 127 + 128) * br;
    float b = (sinf((pos + 170) * 0.024f) * 127 + 128) * br;
    setPx(f[i], r, g, b);
  }
}

static void fxHyperspace(const FxCtx &c, Pixel *f) {
  float p = phase(c.t, 1309);
  for (int i = 0; i < PIXELS; i++) {
    float d = fabsf(PIXELS / 2.0f - i);
    float z = sinf(TWO_PI_F * (p - d * 8 / 157.08f)) * 0.5f + 0.5f;
    float z2 = z * z;
    setPx(f[i], z2 * z2 * 255, z2 * 200, z * 255);
  }
}

static void fxPlasma(const FxCtx &c, Pixel *f) {
  uint32_t off = (uint32_t)((uint64_t)c.t * 12 / 100);
  for (int i = 0; i < PIXELS; i++) {
    uint8_t v = (i * 7 + off) & 255;
    uint8_t r = (v < 128) ? v * 2 : 255;
    uint8_t g = (v < 128) ? 255 : (255 - (v - 128) * 2);
    uint8_t b = (v < 128) ? 255 - v * 2 : (v - 128) * 2;
    setPx(f[i], r, g, b);
  }
}

static void fxTheaterChase(const FxCtx &c, Pixel *f) {
  uint32_t s = c.t / 50;
  uint8_t q = s % 3;
  uint8_t j = (s / 3) & 255;
  for (int i = 0; i < PIXELS; i++) {
    if (i % 3 == q) {
      RGB w = wheel((i + j) & 255);
      setPx(f[i], w.r, w.g, w.b);
    } else {
      setPx(f[i], 0, 0, 0);
    }
  }
}

static void fxTwinkle(const FxCtx &c, Pixel *f) {
  static uint8_t lvl[PIXELS];
  static uint32_t acc = 0;
  for (uint16_t n = takeSteps(acc, c.dt, 30); n > 0; n--) {
    for (int i = 0; i < PIXELS; i++) {
      if (lvl[i] > 0)
        lvl[i] = lvl[i] > 10 ? lvl[i] - 10 : 0;
      else if (random(100) < 12)
        lvl[i] = random(160, 255);
    }
  }
  for (int i = 0; i < PIXELS; i++) {
    if (!lvl[i]) {
      setPx(f[i], 0, 0, 0);
      continue;
    }
    RGB w = wheel((i * 5 + lvl[i]) & 255);
    setPx(f[i], (w.r * lvl[i]) >> 8, (w.g * lvl[i]) >> 8, (w.b * lvl[i]) >> 8);
  }
}

static void fxSparkle(const FxCtx &c, Pixel *f) {
  static Pixel buf[PIXELS];
  static uint32_t acc = 0;
  for (uint16_t n = takeSteps(acc, c.dt, 20); n > 0; n--) {
    fadeBuf(buf, 12);
    if (random(100) < 30) setPx(buf[random(PIXELS)], 255, 255, 255);
  }
  copyBuf(f, buf);
}

static void fxMeteorRain(const FxCtx &c, Pixel *f) {
  static Pixel buf[PIXELS];
  static uint32_t acc = 0;
  for (uint16_t n = takeSteps(acc, c.dt, 20); n > 0; n--) {
    fadeBuf(buf, 24);
    int pos = random(PIXELS);
    for (int j = 0; j < 6; j++)
      if (pos + j < PIXELS) setPx(buf[pos + j], 255, 180, 100);
  }
  copyBuf(f, buf);
}

static void fxScanner(const FxCtx &c, Pixel *f) {
  const float eye = 3.0f;
  float p = phase(c.t, 2 * (PIXELS - 1) * 12);
  float pos = (p < 0.5f ? p : 1.0f - p) * 2 * (PIXELS - 1);
  for (int i = 0; i < PIXELS; i++) putScaled(f[i], c.base, 1.0f - fabsf(i - pos) / eye);
}

static void fxComet(const FxCtx &c, Pixel *f) {
  const int tail = 8;
  const int span = PIXELS + tail;
  float head = phase(c.t, span * 15) * span;
  for (int i = 0; i < PIXELS; i++) {
    float d = head - i;
    putScaled(f[i], c.base, (d >= 0 && d < tail) ? 1.0f - d / tail : 0);
  }
}

// ---------------------------------------------------------------------------

const EffectInfo EFFECTS[] = {
  {"Solid", true, fxSolid},
  {"Breathe", true, fxBreathe},
  {"Color wave", true, fxColorWave},
  {"Rainbow", false, fxRainbow},
  {"Rainbow fire", false, fxRainbowFire},
  {"Hyperspace", false, fxHyperspace},
  {"Plasma", false, fxPlasma},
  {"Theater chase", false, fxTheaterChase},
  {"Twinkle", false, fxTwinkle},
  {"Sparkle", false, fxSparkle},
  {"Meteor rain", false, fxMeteorRain},
  {"Scanner", true, fxScanner},
  {"Comet", true, fxComet},
};
const uint8_t EFFECT_COUNT = sizeof(EFFECTS) / sizeof(EFFECTS[0]);
