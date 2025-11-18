// MatrixPanel.cpp
#include "MatrixPanel.h"

MatrixPanel::MatrixPanel(uint8_t pinPanel0, uint8_t pinPanel1)
  : pin0(pinPanel0), pin1(pinPanel1) {}

void MatrixPanel::begin() {
  delay(50);

  FastLED.addLeds<WS2812, 25, GRB>(leds, ledsPerPanel);
  FastLED.addLeds<WS2812, 26, GRB>(leds + ledsPerPanel,
                                              ledsPerPanel);

  FastLED.setBrightness(120);

  clearAll();
  show();
}

uint16_t MatrixPanel::XY(uint16_t x, uint16_t y) {
  if (x >= totalWidth || y >= totalHeight) return UINT16_MAX;

  uint8_t panel = (y < panelHeight) ? 0 : 1;
  uint16_t panelOffset = panel * ledsPerPanel;

  uint16_t localY = y % panelHeight;
  uint16_t localX = x;

  if (panel == 1) {
    localX = (panelWidth - 1) - localX;
    localY = (panelHeight - 1) - localY;
  }

  uint16_t indexInPanel;

  if ((localX & 1) == 0) {
    indexInPanel = localX * panelHeight + localY;
  } else {
    indexInPanel = localX * panelHeight + (panelHeight - 1 - localY);
  }

  return panelOffset + indexInPanel;
}

void MatrixPanel::setPixelXY(uint16_t x, uint16_t y,
                             const CRGB &color) {
  uint16_t idx = XY(x, y);
  if (idx < ledsTotal) {
    leds[idx] = color;
  }
}

void MatrixPanel::show() {
  FastLED.show();
}

void MatrixPanel::clearAll() {
  FastLED.clear();
}
