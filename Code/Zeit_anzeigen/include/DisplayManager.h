#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <LEDMatrix.h>

// Display-Konfiguration
#define PIN_TOP         25
#define PIN_BOTTOM      26
#define PANEL_WIDTH     32
#define PANEL_HEIGHT    8
#define LEDS_PER_PANEL  (PANEL_WIDTH * PANEL_HEIGHT)
#define CANVAS_WIDTH    64
#define CANVAS_HEIGHT   16
#define BRIGHTNESS      80  // Erhöht für bessere Sichtbarkeit (war 25)
#define COLOR_ORDER     GRB
#define CHIPSET         WS2812

class DisplayManager {
private:
  static DisplayManager* instance;
  CRGB ledsTop[LEDS_PER_PANEL];
  CRGB ledsBottom[LEDS_PER_PANEL];
  CRGB canvasLeds[CANVAS_WIDTH * CANVAS_HEIGHT];
  cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>* panelTop;
  cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>* panelBottom;
  cLEDMatrix<CANVAS_WIDTH, CANVAS_HEIGHT, HORIZONTAL_MATRIX>* canvas;
  bool initialized;
  DisplayManager();
  void mirrorPanelHorizontal(cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>* panel);
  void rotatePanel180(cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>* panel);

public:
  static DisplayManager* getInstance();
  void init();
  void clear();
  void show();
  void setBrightness(uint8_t brightness);
  CRGB& pixel(uint8_t x, uint8_t y);
  void setPixel(uint8_t x, uint8_t y, CRGB color);
  void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, CRGB color);
  void updatePanels();
  uint8_t getWidth() const { return CANVAS_WIDTH; }
  uint8_t getHeight() const { return CANVAS_HEIGHT; }
  cLEDMatrix<CANVAS_WIDTH, CANVAS_HEIGHT, HORIZONTAL_MATRIX>* getCanvas() { return canvas; }
  ~DisplayManager();
};
