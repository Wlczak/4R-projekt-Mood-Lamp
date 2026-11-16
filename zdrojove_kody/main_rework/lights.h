#pragma once
#include "config.h"
#include <ArduinoJson.h>

struct LightState {
  bool on;
  uint8_t r, g, b;
  uint8_t brightness;  // 0-100 %
  uint8_t effect;      // index into EFFECTS
  uint8_t speed;       // 1-100, 50 = normal
};

void lightsBegin(const LightState &initial);
void lightsTick();
const LightState &lightsGet();
void lightsSet(const LightState &s);

// Partial update from JSON. Missing fields are left as they are.
// Returns false if a present field is invalid (bad color / unknown effect).
bool lightStateFromJson(JsonVariantConst v, LightState &s);
void lightStateToJson(const LightState &s, JsonObject o);
