// Mood Lamp - lights + WiFi + built-in web app.
//
// Everything runs from loop() without blocking: WiFi state machine (with hotspot
// fallback), web server, LED renderer (~60 fps) and debounced settings saving.
//
// Board: LOLIN(WEMOS) D1 R2 & mini. Flash Size must include an FS partition
// (e.g. "4MB (FS:2MB OTA:~1019KB)") so settings can be stored in LittleFS.
//
// Web app: http://moodlamp.local (or the IP printed on Serial).
// No WiFi for 30 s -> hotspot "MoodLamp-XXXX", password "moodlamp", http://192.168.4.1

#include "config.h"
#include "lights.h"
#include "settings.h"
#include "status_anim.h"
#include "web.h"
#include "wifi_mgr.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Mood Lamp booting");
  randomSeed(RANDOM_REG32);

  settingsBegin();
  statusBegin();
  lightsBegin(settingsLoad());
  wifiBegin();
  webBegin();
}

void loop() {
  NetState s;
  if (wifiTick(s)) statusOnNet(s);
  webTick();
  lightsTick();
  settingsTick();
}
