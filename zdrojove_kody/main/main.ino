#include <WiCo.h>
WiCo wico;
int frames = 0;
long milli = millis();
bool countFrames = false;
void setup() {
  Serial.begin(115200);
  wico.addSTA("wemos", "wemosD1R2");
  wico.connectSTA();
  wico.setOTAsettings("mood_lamp", "8266", 8266);
  wico.startOTA();
  wico.setMQTTAuth("wemos", "wemosR1D2");
  wico.setMQTTId("mood_lamp");
  wico.connectMQTT("192.168.0.106", 1883);
  wico.publishMQTT("test", "hello there :)");
  wico.subscribeMQTT("test", echo);
}
void calcFramerate() {
  frames++;
  if (millis() - milli > 1000) {
    Serial.println(frames);
    milli = millis();
    frames = 0;
  }
}

void echo(const char* topic, const char* msg) {
  Serial.println("This is the echo function.");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Msg: ");
  Serial.println(msg);
}

void loop() {
  wico.runOTA();
  wico.runMQTT();
  if (countFrames) {
    calcFramerate();
  }
}