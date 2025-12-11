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

void handleRGB(const char *topic, const char *msg) {
  Serial.print("Received: ");
  Serial.print(topic);
  Serial.print("RGB: ");
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

void rgb(const char *topic, const char *msg) {
  std::string hexString = msg;
  if (hexString.length() != 6) {
    return;
  }

  // Convert each pair of characters (hex) to integers
  int newRed = std::stoi(hexString.substr(0, 2), nullptr, 16);
  int newGreen = std::stoi(hexString.substr(2, 2), nullptr, 16);
  int newBlue = std::stoi(hexString.substr(4, 2), nullptr, 16);

  int time = 100;

  float stepRed = static_cast<float>(newRed - red) / time;
  float stepGreen = static_cast<float>(newGreen - green) / time;
  float stepBlue = static_cast<float>(newBlue - blue) / time;

  int startingRed = red;
  int startingGreen = green;
  int startingBlue = blue;

  Serial.print("stepRed:");
  Serial.println(stepRed);

  for (int j = 0; j < time; j++) {
    int setRed = static_cast<int>(std::floor(stepRed * j + red));
    int setGreen = static_cast<int>(std::floor(stepGreen * j + green));
    int setBlue = static_cast<int>(std::floor(stepBlue * j + blue));

    for (int i = 0; i < PIXELS; i++) {
      neofruit.setPixelColor(i, neofruit.Color(setRed, setGreen, setBlue));
      // Serial.print("time: ");
      // Serial.println();
    }
    Serial.print("red:");
    Serial.println(static_cast<int>(std::floor(stepRed * j + red)));
    neofruit.show();
    delay(10);
  }
  red = newRed;
  green = newGreen;
  blue = newBlue;
}

void brightness(const char *topic, const char *msg) {
  Serial.println(std::stoi(msg));

  int time = 100;

  int newBrightness = std::stoi(msg);

  float stepBrightness = static_cast<float>(newBrightness - last_brightness) / time;

  int startingBrightness = last_brightness;

  for (int j = 0; j < time; j++) {
    int setBrightness = static_cast<int>(std::floor(stepBrightness * j + last_brightness));

    for (int i = 0; i < PIXELS; i++) {
      neofruit.setBrightness(setBrightness);
      neofruit.setPixelColor(i, neofruit.Color(red, green, blue));
    }
    neofruit.show();
    delay(10);
    Serial.print("brightness:");
    Serial.println(setBrightness);
  }
  last_brightness = newBrightness;

  neofruit.setBrightness(std::stoi(msg));
  neofruit.show();
}

void loop() {
  wico.runMQTT();
}