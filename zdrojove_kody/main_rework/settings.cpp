#include "settings.h"
#include <LittleFS.h>

static const char *PATH = "/settings.json";
static const char *TMP_PATH = "/settings.tmp";

static bool fsOk = false;
static bool dirty = false;
static uint32_t lastChange = 0;

void settingsBegin() {
  fsOk = LittleFS.begin();
  if (!fsOk) {
    Serial.println("Settings: LittleFS mount failed, formatting");
    fsOk = LittleFS.format() && LittleFS.begin();
  }
  if (!fsOk) Serial.println("Settings: no filesystem (check Flash Size / FS partition), not saving");
}

LightState settingsLoad() {
  LightState s = {true, 255, 140, 60, 60, 0, 50};  // warm white, 60 %
  if (!fsOk || !LittleFS.exists(PATH)) return s;

  File f = LittleFS.open(PATH, "r");
  if (!f) return s;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("Settings: corrupt file (%s), using defaults\n", err.c_str());
    return s;
  }
  lightStateFromJson(doc.as<JsonVariantConst>(), s);
  Serial.println("Settings: loaded");
  return s;
}

void settingsMarkDirty() {
  dirty = true;
  lastChange = millis();
}

static void save() {
  File f = LittleFS.open(TMP_PATH, "w");
  if (!f) {
    Serial.println("Settings: cannot open file for writing");
    return;
  }
  JsonDocument doc;
  lightStateToJson(lightsGet(), doc.to<JsonObject>());
  serializeJson(doc, f);
  f.close();
  // write to a temp file first so a power cut mid-write can't corrupt the settings
  if (LittleFS.rename(TMP_PATH, PATH))
    Serial.println("Settings: saved");
  else
    Serial.println("Settings: rename failed");
}

void settingsTick() {
  if (dirty && fsOk && millis() - lastChange >= SAVE_DEBOUNCE_MS) {
    dirty = false;
    save();
  }
}
