#include <WiCo.h>

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <DHT11.h>


WiCo wico;
DHT11 dht11(D6);
int frames = 0;
long milli = millis();
bool countFrames = false;

int pixels = 72;

int red = 0;
int green = 0;
int blue = 0;
int last_brightness = 0;

int temperature = 0;
int humidity = 0;
long dht_millis = 0;

Adafruit_NeoPixel neofruit(pixels, D8, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  wico.addSTA("wemos", "wemosD1R2");
  wico.addSTA("3301-IoT", "mikrobus");
  wico.connectSTA();
  wico.setOTAsettings("mood_lamp", "8266", 8266);
  wico.startOTA();
  
  wico.setMQTTId("mood_lamp");
  // home MQTT
  wico.setMQTTAuth("wemos", "wemosR1D2");
  wico.connectMQTT("192.168.0.106", 1883);

  // school MQTT
  //wico.connectMQTT("10.202.31.167", 1883);
  wico.publishMQTT("mood_lamp", "hello there :)");
  wico.subscribeMQTT("mood_lamp/rgb", rgb);
  wico.subscribeMQTT("mood_lamp/brightness", brightness);

  neofruit.begin();
  neofruit.setBrightness(0);
  for (int i = 0; i < pixels; i++) {
    neofruit.setPixelColor(i, neofruit.Color(0, 0, 0));
    neofruit.show();
  }
}
void calcFramerate() {
  frames++;
  if (millis() - milli > 1000) {
    Serial.println(frames);
    milli = millis();
    frames = 0;
  }
}

void rgb(const char* topic, const char* msg) {
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

    for (int i = 0; i < pixels; i++) {
      neofruit.setPixelColor(i, neofruit.Color(setRed, setGreen, setBlue));
      //Serial.print("time: ");
      //Serial.println();
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

void brightness(const char* topic, const char* msg) {
  Serial.println(std::stoi(msg));

  int time = 100;

  int newBrightness = std::stoi(msg);

  float stepBrightness = static_cast<float>(newBrightness - last_brightness) / time;

  int startingBrightness = last_brightness;

  for (int j = 0; j < time; j++) {
    int setBrightness = static_cast<int>(std::floor(stepBrightness * j + last_brightness));

    for (int i = 0; i < pixels; i++) {
      neofruit.setBrightness(setBrightness);
      neofruit.setPixelColor(i, neofruit.Color(red, green, blue));
    }
    neofruit.show();
    delay(10);
    Serial.print("brightness:");
    Serial.println(setBrightness);
  }
  last_brightness = newBrightness;

  //neofruit.setBrightness(std::stoi(msg));
  //neofruit.show();
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

  if (millis() - dht_millis > 1000) {
    dht_millis = millis();
    int new_temperature;
    int new_humidity;
    int result = dht11.readTemperatureHumidity(new_temperature, new_humidity);
    if (result == 0) {
      if (new_temperature != temperature) {
        wico.publishMQTT("mood_lamp/temperature", std::to_string(new_temperature).c_str(), true);
        temperature = new_temperature;
      }

      if (new_humidity != humidity) {
        wico.publishMQTT("mood_lamp/humidity", std::to_string(new_humidity).c_str(), true);
        humidity = new_humidity;
      }
    } else {
      // Print error message based on the error code.
      Serial.println(DHT11::getErrorString(result));
    }
  }
}