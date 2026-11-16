#pragma once
#include "config.h"

// t:  effect time in ms (already scaled by the speed setting)
// dt: effect time elapsed since the previous frame
// base: the user's color (already mid-transition if a fade is running)
struct FxCtx {
  uint32_t t;
  uint32_t dt;
  RGB base;
};

typedef void (*EffectFn)(const FxCtx &ctx, Pixel *frame);

struct EffectInfo {
  const char *name;
  bool usesColor;
  EffectFn fn;
};

// Index in this table is the effect id used by the API and in saved settings.
// Only append new effects at the end so saved ids stay valid.
extern const EffectInfo EFFECTS[];
extern const uint8_t EFFECT_COUNT;
