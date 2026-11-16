#pragma once
#include "lights.h"

void settingsBegin();
LightState settingsLoad();
// Call after every change; the file is written once things have been quiet for SAVE_DEBOUNCE_MS.
void settingsMarkDirty();
void settingsTick();
