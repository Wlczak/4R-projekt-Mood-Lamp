#include <WiCo.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <DHT11.h>

#define PIXELS 72

WiCo wico;
Adafruit_NeoPixel neofruit(PIXELS, D8, NEO_GRB + NEO_KHZ800);

int red = 0;
int green = 0;
int blue = 0;
int last_brightness = 0;

int temperature = 0;
int humidity = 0;
long dht_millis = 0;

void setColor(int r, int g, int b) {
  delay(600);
  for (int i = 0; i < PIXELS; i++) {
    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
  }
  neofruit.show();
}

void mqttPrint(const char *topic, const char *msg) {
  Serial.print("MQTT incoming [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);
}

void setupMQTT() {
  setColor(255, 255, 0);
  if (!wico.connectMQTT("192.168.0.106", 1883)) {
    setColor(255, 120, 0);
    delay(5000);
    return;
  }
}

void setup() {
  wico.setMQTTId("mood_lamp");
  wico.setMQTTAuth("wemos", "wemosR1D2");
  Serial.begin(115200);
  neofruit.begin();
  neofruit.setBrightness(20);
  Serial.println("");
  Serial.println("Starting...");
  setColor(0, 250, 0);

  setColor(0, 0, 250);
  wico.addSTA("wemos", "wemosD1R2");
  setColor(255, 0, 250);
  if (!wico.connectSTA()) {
    setColor(255, 0, 0);
    delay(5000);
  }
  Serial.println("Sta connected");
  setColor(0, 0, 250);
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  setupMQTT();
  setColor(0, 0, 0);
  wico.subscribeMQTT("mood_lamp/rgb", mqttPrint);
  wico.subscribeMQTT("mood_lamp/brightness", mqttPrint);
  neofruit.setBrightness(0);
}

void loop() {
  wico.runMQTT();
}
