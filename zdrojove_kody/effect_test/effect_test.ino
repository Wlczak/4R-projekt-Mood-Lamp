#include <Adafruit_NeoPixel.h>

#define PIXELS 72

Adafruit_NeoPixel neofruit(PIXELS, D8, NEO_GRB + NEO_KHZ800);

uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return neofruit.Color(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return neofruit.Color(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    return neofruit.Color(pos * 3, 255 - pos * 3, 0);
  }
}

void fadePixel(uint16_t i, uint8_t amount) {
  uint32_t col = neofruit.getPixelColor(i);
  uint8_t r = (col >> 16) & 0xFF;
  uint8_t g = (col >> 8) & 0xFF;
  uint8_t b = col & 0xFF;
  if (r <= amount) r = 0; else r -= amount;
  if (g <= amount) g = 0; else g -= amount;
  if (b <= amount) b = 0; else b -= amount;
  neofruit.setPixelColor(i, neofruit.Color(r, g, b));
}

void colorWipe(uint8_t r, uint8_t g, uint8_t b, int waitMs) {
  for (int i = 0; i < PIXELS; i++) {
    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
    neofruit.show();
    delay(waitMs);
    yield();
  }
}

void theaterChaseRainbow(int wait) {
  for (int q = 0; q < 3; q++) {
    for (int j = 0; j < 256; j++) {
      for (int i = 0; i < PIXELS; i += 3) {
        int idx = (i + j) & 255;
        neofruit.setPixelColor(i+q, wheel(idx));
      }
      neofruit.show();
      delay(wait);
      for (int i = 0; i < PIXELS; i += 3) {
        neofruit.setPixelColor(i+q, 0);
      }
      yield();
    }
  }
}

void twinkleRandom(int density, int speed) {
  static uint8_t state[PIXELS];
  for (int i = 0; i < PIXELS; i++) {
    if (state[i] > 0) {
      if (state[i] > 10) state[i] -= 10; else state[i] = 0;
      uint8_t v = state[i];
      uint32_t c = wheel((i * 5 + v) & 255);
      uint8_t r = (c >> 16) & 0xFF;
      uint8_t g = (c >> 8) & 0xFF;
      uint8_t b = c & 0xFF;
      r = (r * v) >> 8;
      g = (g * v) >> 8;
      b = (b * v) >> 8;
      neofruit.setPixelColor(i, neofruit.Color(r, g, b));
    } else {
      if (random(0, 100) < density) {
        state[i] = random(160, 255);
      } else {
        neofruit.setPixelColor(i, 0);
      }
    }
  }
  neofruit.show();
  delay(speed);
  yield();
}

void gradientWave(int speed) {
  static uint16_t t = 0;
  for (int i = 0; i < PIXELS; i++) {
    uint8_t pos = (i * 256 / PIXELS + (t >> 1)) & 255;
    neofruit.setPixelColor(i, wheel(pos));
  }
  neofruit.show();
  t += speed;
  yield();
}

void sparkle(int speed, int density) {
  for (int i = 0; i < PIXELS; i++) {
    fadePixel(i, 12);
  }
  if (random(0, 100) < density) {
    int p = random(0, PIXELS);
    neofruit.setPixelColor(p, neofruit.Color(255, 255, 255));
  }
  neofruit.show();
  delay(speed);
  yield();
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
    yield();
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
  yield();
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
  yield();
}

void electricArcs(int speed, int density) {
  static uint16_t t = 0;
  for (int i = 0; i < PIXELS; i++) {
    int brightness = 0;
    if ((random(0, 100) < density)) {
      brightness = random(150, 255);
    }
    int pulse = (sin((i * 4 + t) * 0.1) + 1) * 127;
    brightness = max(brightness, pulse);
    int r = brightness / 4;
    int g = brightness / 2;
    int b = brightness;
    neofruit.setPixelColor(i, neofruit.Color(r, g, b));
  }
  neofruit.show();
  t += speed;
  yield();
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());
  neofruit.begin();
  neofruit.setBrightness(100);
  for (int i = 0; i < PIXELS; i++) {
    neofruit.setPixelColor(i, neofruit.Color(0, 0, 0));
    neofruit.show();
  }
}

void loop() {
  comet(255, 50, 0, 8, 15);
  comet(0, 80, 255, 10, 10);
  long time = millis();
  while (millis() - time < 10000) {
    rainbowFire(3);
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    hyperspace(2);
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    electricArcs(3, 20);
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    colorWipe(255, 0, 0, 8);
    colorWipe(0, 255, 0, 8);
    colorWipe(0, 0, 255, 8);
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    theaterChaseRainbow(50);
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    for (int i = 0; i < 50; i++) {
      twinkleRandom(12, 30);
    }
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    for (int i = 0; i < 200; i++) {
      gradientWave(2);
    }
    yield();
  }
  time = millis();
  while (millis() - time < 10000) {
    for (int i = 0; i < 150; i++) {
      sparkle(20, 30);
    }
    yield();
  }
}
