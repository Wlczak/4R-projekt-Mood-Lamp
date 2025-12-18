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

int curR = 0;
int curG = 0;
int curB = 0;
int curBrightness = 0;

int targetR = 0;
int targetG = 0;
int targetB = 0;
int targetBrightness = 0;

unsigned long lastFade = 0;
const unsigned long fadeInterval = 20;
const int fadeStep = 1;

void applyPixels() {
  for (int i = 0; i < PIXELS; i++) {
    neofruit.setPixelColor(i, neofruit.Color(curR, curG, curB));
  }
  neofruit.setBrightness(curBrightness);
  neofruit.show();
}

int stepTowards(int current, int target) {
  if (current < target) return min(current + fadeStep, target);
  if (current > target) return max(current - fadeStep, target);
  return current;
}

void rgb(const char *topic, const char *msg) {
  if (strlen(msg) != 6) return;

  targetR = strtol(String(msg).substring(0, 2).c_str(), nullptr, 16);
  targetG = strtol(String(msg).substring(2, 4).c_str(), nullptr, 16);
  targetB = strtol(String(msg).substring(4, 6).c_str(), nullptr, 16);

  Serial.print("New RGB target: ");
  Serial.println(msg);
}

void brightness(const char *topic, const char *msg) {
  int b = atoi(msg);
  b = constrain(b, 0, 100);
  targetBrightness = map(b, 0, 100, 0, 255);

  Serial.print("New brightness target: ");
  Serial.println(targetBrightness);
}

void updateFade() {
  if (millis() - lastFade < fadeInterval) return;
  lastFade = millis();

  curR = stepTowards(curR, targetR);
  curG = stepTowards(curG, targetG);
  curB = stepTowards(curB, targetB);
  curBrightness = stepTowards(curBrightness, targetBrightness);

  applyPixels();
}


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
  wico.subscribeMQTT("mood_lamp/rgb", rgb);
  wico.subscribeMQTT("mood_lamp/brightness", brightness);
  neofruit.setBrightness(0);
}

void loop() {
  wico.runMQTT();
  updateFade();
}
