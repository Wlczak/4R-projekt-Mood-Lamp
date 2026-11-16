#include "web.h"
#include "config.h"
#include "effects.h"
#include "lights.h"
#include "settings.h"
#include "wifi_mgr.h"
#include "web_ui.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

static ESP8266WebServer server(80);

static void sendJson(int code, JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

static void sendError(int code, const char *msg) {
  JsonDocument doc;
  doc["error"] = msg;
  sendJson(code, doc);
}

static void sendState() {
  JsonDocument doc;
  lightStateToJson(lightsGet(), doc.to<JsonObject>());
  JsonObject w = doc["wifi"].to<JsonObject>();
  bool up = WiFi.status() == WL_CONNECTED;
  w["state"] = wifiStateName();
  w["ssid"] = STA_SSID;
  w["host"] = HOSTNAME ".local";
  if (up) {
    w["ip"] = WiFi.localIP().toString();
    w["rssi"] = WiFi.RSSI();
  }
  w["ap"] = wifiApActive();
  if (wifiApActive()) {
    w["apSsid"] = wifiApSsid();
    w["apIp"] = WiFi.softAPIP().toString();
  }
  sendJson(200, doc);
}

static void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", INDEX_HTML);
}

static void handleGetState() {
  sendState();
}

static void handlePostState() {
  JsonDocument in;
  if (deserializeJson(in, server.arg("plain")) || !in.is<JsonObject>()) {
    sendError(400, "invalid JSON");
    return;
  }
  LightState s = lightsGet();
  if (!lightStateFromJson(in.as<JsonVariantConst>(), s)) {
    sendError(400, "invalid value");
    return;
  }
  lightsSet(s);
  settingsMarkDirty();
  sendState();
}

static void handleEffects() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = i;
    o["name"] = EFFECTS[i].name;
    o["usesColor"] = EFFECTS[i].usesColor;
  }
  sendJson(200, doc);
}

// While the hotspot is up, send phones' captive-portal checks (and anything else
// addressed to a foreign host) to the lamp's page.
static void handleNotFound() {
  if (wifiApActive()) {
    String host = server.hostHeader();
    String apIp = WiFi.softAPIP().toString();
    bool ours = host == apIp || host.startsWith(HOSTNAME) ||
                (WiFi.status() == WL_CONNECTED && host == WiFi.localIP().toString());
    if (!ours) {
      server.sendHeader("Location", String("http://") + apIp + "/", true);
      server.send(302, "text/plain", "");
      return;
    }
  }
  server.send(404, "text/plain", "Not found");
}

void webBegin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleGetState);
  server.on("/api/state", HTTP_POST, handlePostState);
  server.on("/api/effects", HTTP_GET, handleEffects);
  server.onNotFound(handleNotFound);
  server.begin();
}

void webTick() {
  server.handleClient();
}
