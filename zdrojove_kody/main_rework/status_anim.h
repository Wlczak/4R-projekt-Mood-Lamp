#pragma once
#include "config.h"
#include "wifi_mgr.h"

// LED status animations for the WiFi state, drawn as an overlay on top of the light.
void statusBegin();
void statusOnNet(NetState s);
// True while the user's light should stay dark (first connection attempt after boot).
bool statusSuppressLight(uint32_t now);
// level: current light level (0-1); status is drawn at least at STATUS_MIN_LEVEL.
void statusRender(uint32_t now, Pixel *out, float level);
