// MatrixPanel.h
#ifndef MATRIXPANEL_H
#define MATRIXPANEL_H

#include <FastLED.h>

class MatrixPanel {
public:
  MatrixPanel(uint8_t pinPanel0, uint8_t pinPanel1);

  void begin();
  void setPixelXY(uint16_t x, uint16_t y, const CRGB &color);
  uint16_t XY(uint16_t x, uint16_t y);
  void show();
  void clearAll();

private:
  static const uint8_t panelHeight = 8;
  static const uint8_t panelWidth  = 32;
  static const uint8_t panelCount  = 2;

  static const uint16_t totalWidth  = panelWidth;
  static const uint16_t totalHeight = panelHeight * panelCount;

  static const uint16_t ledsPerPanel = panelHeight * panelWidth;
  static const uint16_t ledsTotal    = ledsPerPanel * panelCount;

  CRGB leds[ledsTotal];

  uint8_t pin0;
  uint8_t pin1;
};

#endif
