#pragma once
#include <Arduino.h>

enum NetState : uint8_t {
  NET_CONNECTING,  // first connection attempt after boot
  NET_CONNECTED,   // joined the hardcoded WiFi
  NET_LOST,        // was connected, now reconnecting
  NET_AP,          // WiFi unreachable for AP_FALLBACK_MS, own hotspot is up
};

void wifiBegin();
// Call every loop. Returns true (and sets changedTo) when the state changed.
bool wifiTick(NetState &changedTo);
NetState wifiState();
const char *wifiStateName();
bool wifiApActive();
const char *wifiApSsid();
