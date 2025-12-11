#include <WiCo.h>

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <DHT11.h>

int pixels = 72;

WiCo wico;
Adafruit_NeoPixel neofruit(pixels, D8, NEO_GRB + NEO_KHZ800);

bool hadConnectionInteruption = false;

void setColor(int r, int g, int b) {
  delay(600);
  for (int i = 0; i < pixels; i++) {
    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
  }
  neofruit.show();
}

void handleRGB(const char *topic, const char *msg) {
  Serial.print("Received: ");
  Serial.print(topic);
  Serial.print("RGB: ");
  Serial.println(msg);
}

void setupMQTT() {
  setColor(255, 255, 0);
  wico.setMQTTId("mood_lamp");
  wico.setMQTTAuth("wemos", "wemosR1D2");
  if (!wico.connectMQTT("192.168.0.106", 1883)) {
    setColor(255, 120, 0);
    delay(5000);
    return;
  }
}

void setup() {
  Serial.begin(115200);
  neofruit.begin();
  neofruit.setBrightness(20);
  Serial.println("");
  Serial.println("Starting...");
  setColor(0, 250, 0);

  setColor(0, 0, 250);
  wico.addSTA("wemos", "wemosD1R2");
  wico.addSTA("3301-IoT", "mikrobus");
  setColor(255, 0, 250);
  if (!wico.connectSTA()) {
    setColor(255, 0, 0);
    delay(5000);
  }
  Serial.println("Sta connected");
  setColor(0, 0, 250);
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  setColor(0, 0, 0);
  setupMQTT();
  wico.subscribeMQTT("mood_lamp/rgb", handleRGB);
}

void loop() {
  if (!wico.isConnected()) {
    hadConnectionInteruption = true;
    setColor(255, 0, 0);
  } else {
    setColor(0, 255, 0);
  }
  if (wico.isConnectedMQTT()) {
    Serial.println("MQTT connected");
  }
  if (hadConnectionInteruption) {
    setupMQTT();
    hadConnectionInteruption = false;
  }
  if (!wico.isConnectedMQTT()) {
    setupMQTT();
  }
  wico.runMQTT();
}
