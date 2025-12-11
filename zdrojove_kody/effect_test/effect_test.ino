#include <Adafruit_NeoPixel.h>

#define PIXELS 72

Adafruit_NeoPixel neofruit(PIXELS, D8, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  neofruit.begin();
  neofruit.setBrightness(100);
  for (int i = 0; i < PIXELS; i++) {
    neofruit.setPixelColor(i, neofruit.Color(0, 0, 0));
    neofruit.show();
  }
}

void comet(int r, int g, int b, int tail, int delayMs) {
  for (int i = 0; i < PIXELS + tail; i++) {
    for (int j = 0; j < PIXELS; j++) {
      int d = i - j;
      if (d >= 0 && d < tail) {
        int fade = map(d, 0, tail - 1, 255, 0);
        neofruit.setPixelColor(j, neofruit.Color((r * fade) / 255, (g * fade) / 255, (b * fade) / 255));
      } else {
        neofruit.setPixelColor(j, 0);
      }
    }
    neofruit.show();
    delay(delayMs);
  }
}

void rainbowFire(int speed) {
  static uint16_t t = 0;

  for (int i = 0; i < PIXELS; i++) {
    int pos = (i * 10 + t) & 255;

    int r = (sin((pos + 0) * 0.024) * 127) + 128;
    int g = (sin((pos + 85) * 0.024) * 127) + 128;
    int b = (sin((pos + 170) * 0.024) * 127) + 128;

    float breathe = (sin(t * 0.02) + 1.0) * 0.4 + 0.2;

    r *= breathe;
    g *= breathe;
    b *= breathe;

    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
  }

  neofruit.show();
  t += speed;
}

void hyperspace(int speed) {
  static uint16_t t = 0;

  for (int i = 0; i < PIXELS; i++) {
    float d = abs((PIXELS / 2) - i);
    float z = sin((t - d * 8) * 0.04) * 0.5 + 0.5;

    int r = pow(z, 4) * 255;
    int g = pow(z, 2) * 200;
    int b = z * 255;

    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
  }

  neofruit.show();
  t += speed;
}

void electricArcs(int speed, int density) {
  static uint16_t t = 0;

  for (int i = 0; i < PIXELS; i++) {
    int brightness = 0;

    // Random arcs
    if ((random(0, 100) < density)) {
      brightness = random(150, 255);
    }

    // Pulse with time
    int pulse = (sin((i * 4 + t) * 0.1) + 1) * 127;
    brightness = max(brightness, pulse);

    // Color shifts: electric blue with white spikes
    int r = brightness / 4;
    int g = brightness / 2;
    int b = brightness;

    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
  }

  neofruit.show();
  t += speed;
}


void loop() {
  comet(255, 50, 0, 8, 15);   // orange comet with 8-pixel tail
  comet(0, 80, 255, 10, 10);  // blue comet with longer tail
  long time = millis();
  while (millis() - time < 10000) {
    rainbowFire(3);
    // Serial.println("fire");
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    hyperspace(2);
    yield();
  }

  time = millis();
  while (millis() - time < 10000) {
    electricArcs(3, 20);  // speed 3, 20% chance for arcs
    yield();
  }
}
