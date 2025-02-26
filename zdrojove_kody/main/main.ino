#include <WiCo.h>
WiCo wico;
int frames = 0;
long milli = millis();
bool countFrames = true;
void setup() {
  Serial.begin(115200);
  wico.addSTA("wemos", "wemosD1R2");
  wico.connectSTA();
  wico.setOTAsettings("mood_lamp", "8266", 8266);
  wico.startOTA();
}
void calcFramerate() {
  frames++;
  if (millis() - milli > 1000) {
    Serial.println(frames);
    milli = millis();
    frames = 0;
  }
}
void loop() {
  wico.handleOTA();
  if (countFrames) {
    calcFramerate();
  }
}