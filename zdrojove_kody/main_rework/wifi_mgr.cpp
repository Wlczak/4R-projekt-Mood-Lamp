#include "wifi_mgr.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>

static NetState st = NET_CONNECTING;
static uint32_t stateT0 = 0;
static uint32_t lastKick = 0;

static bool apOn = false;
static uint32_t apIdleSince = 0;
static char apSsid[24];
static DNSServer dns;  // captive portal: every hostname resolves to the lamp while the hotspot is up

static bool mdnsOn = false;

static void beginSta() {
  WiFi.begin(STA_SSID, STA_PSK);
  lastKick = millis();
}

static void startMdns() {
  if (mdnsOn) return;
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsOn = true;
  }
}

static void startAp() {
  if (apOn) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSsid, AP_PASS);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
  apOn = true;
  apIdleSince = millis();
  Serial.printf("WiFi: hotspot '%s' (pass '%s') at http://%s\n", apSsid, AP_PASS,
                WiFi.softAPIP().toString().c_str());
  startMdns();
}

static void stopAp() {
  if (!apOn) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apOn = false;
  Serial.println("WiFi: hotspot closed");
}

static bool setState(NetState s) {
  if (s == st) return false;
  st = s;
  stateT0 = millis();
  return true;
}

void wifiBegin() {
  snprintf(apSsid, sizeof(apSsid), "%s-%04X", AP_SSID_PREFIX, (unsigned)(ESP.getChipId() & 0xFFFF));
  WiFi.persistent(false);  // don't rewrite credentials to flash on every boot
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);  // mains powered; keeps the web UI responsive
  beginSta();
  st = NET_CONNECTING;
  stateT0 = millis();
  Serial.printf("WiFi: connecting to '%s'\n", STA_SSID);
}

bool wifiTick(NetState &changedTo) {
  uint32_t now = millis();
  bool up = WiFi.status() == WL_CONNECTED;
  bool changed = false;

  switch (st) {
    case NET_CONNECTING:
    case NET_LOST:
      if (up) {
        changed = setState(NET_CONNECTED);
      } else if (now - stateT0 >= AP_FALLBACK_MS) {
        startAp();
        changed = setState(NET_AP);
      }
      break;
    case NET_AP:
      if (up) changed = setState(NET_CONNECTED);
      break;
    case NET_CONNECTED:
      if (!up) changed = setState(NET_LOST);
      break;
  }

  if (changed && st == NET_CONNECTED) {
    Serial.printf("WiFi: connected, http://%s  (http://%s.local)\n", WiFi.localIP().toString().c_str(),
                  HOSTNAME);
    startMdns();
    apIdleSince = now;
  } else if (changed && st == NET_LOST) {
    Serial.println("WiFi: connection lost, reconnecting");
  }

  // The SDK retries on its own; a periodic begin() is a safety net in case it gives up.
  if (!up && now - lastKick >= STA_KICK_MS) beginSta();

  if (apOn) {
    dns.processNextRequest();
    if (st == NET_CONNECTED) {
      if (WiFi.softAPgetStationNum() > 0)
        apIdleSince = now;
      else if (now - apIdleSince >= AP_IDLE_OFF_MS)
        stopAp();
    }
  }

  if (mdnsOn) MDNS.update();

  if (changed) changedTo = st;
  return changed;
}

NetState wifiState() {
  return st;
}

const char *wifiStateName() {
  switch (st) {
    case NET_CONNECTING: return "connecting";
    case NET_CONNECTED: return "connected";
    case NET_LOST: return "lost";
    case NET_AP: return "hotspot";
  }
  return "?";
}

bool wifiApActive() {
  return apOn;
}

const char *wifiApSsid() {
  return apSsid;
}
