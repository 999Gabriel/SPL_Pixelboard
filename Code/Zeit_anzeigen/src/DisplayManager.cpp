#include "DisplayManager.h"

DisplayManager* DisplayManager::instance = nullptr;

DisplayManager::DisplayManager() : initialized(false) {
  panelTop = new cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>();
  panelBottom = new cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>();
  canvas = new cLEDMatrix<CANVAS_WIDTH, CANVAS_HEIGHT, HORIZONTAL_MATRIX>();
}

DisplayManager* DisplayManager::getInstance() {
  if (instance == nullptr) {
    instance = new DisplayManager();
  }
  return instance;
}

void DisplayManager::init() {
  if (initialized) return;

  FastLED.addLeds<CHIPSET, PIN_TOP, COLOR_ORDER>(ledsTop, LEDS_PER_PANEL);
  FastLED.addLeds<CHIPSET, PIN_BOTTOM, COLOR_ORDER>(ledsBottom, LEDS_PER_PANEL);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  // KORRIGIERT: Top Panel = ledsTop, Bottom Panel = ledsBottom
  panelTop->SetLEDArray(ledsTop);
  panelBottom->SetLEDArray(ledsBottom);
  canvas->SetLEDArray(canvasLeds);

  initialized = true;
  Serial.println(F("DisplayManager initialisiert"));
  Serial.print(F("  PIN_TOP (25): "));
  Serial.print(LEDS_PER_PANEL);
  Serial.println(F(" LEDs"));
  Serial.print(F("  PIN_BOTTOM (26): "));
  Serial.print(LEDS_PER_PANEL);
  Serial.println(F(" LEDs"));
}

void DisplayManager::clear() {
  memset(canvasLeds, 0, sizeof(canvasLeds));
}

void DisplayManager::show() {
  updatePanels();
  FastLED.show();
}

void DisplayManager::setBrightness(uint8_t brightness) {
  FastLED.setBrightness(brightness);
}

CRGB& DisplayManager::pixel(uint8_t x, uint8_t y) {
  if (x >= CANVAS_WIDTH) x = CANVAS_WIDTH - 1;
  if (y >= CANVAS_HEIGHT) y = CANVAS_HEIGHT - 1;
  return (*canvas)(x, y);
}

void DisplayManager::setPixel(uint8_t x, uint8_t y, CRGB color) {
  if (x < CANVAS_WIDTH && y < CANVAS_HEIGHT) {
    (*canvas)(x, y) = color;
  }
}

void DisplayManager::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, CRGB color) {
  for (uint8_t j = y; j < y + h && j < CANVAS_HEIGHT; j++) {
    for (uint8_t i = x; i < x + w && i < CANVAS_WIDTH; i++) {
      setPixel(i, j, color);
    }
  }
}

void DisplayManager::updatePanels() {
  // Canvas: 64x16 Pixel
  // Oberes Panel (PIN_TOP/25): Zeilen 0-7 des Canvas
  // Unteres Panel (PIN_BOTTOM/26): Zeilen 8-15 des Canvas

  // Oberes Panel: Canvas-Zeilen 0-7
  for (uint8_t y = 0; y < PANEL_HEIGHT; y++) {
    for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
      (*panelTop)(x, y) = (*canvas)(x, y);
    }
  }

  // Unteres Panel: Canvas-Zeilen 8-15
  for (uint8_t y = 0; y < PANEL_HEIGHT; y++) {
    uint8_t canvasY = y + PANEL_HEIGHT; // 8-15
    for (uint8_t x = 0; x < PANEL_WIDTH; x++) {
      (*panelBottom)(x, y) = (*canvas)(x, canvasY);
    }
  }

  // Falls nötig: Spiegelung und Rotation (abhängig von der physischen Verkabelung)
  // Kommentiere diese aus, wenn die Anzeige falsch herum ist
  // mirrorPanelHorizontal(panelTop);
  // mirrorPanelHorizontal(panelBottom);
  // rotatePanel180(panelTop);
}

void DisplayManager::mirrorPanelHorizontal(cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>* panel) {
  for (uint8_t y = 0; y < PANEL_HEIGHT; y++) {
    for (uint8_t x = 0; x < PANEL_WIDTH / 2; x++) {
      uint8_t xo = PANEL_WIDTH - 1 - x;
      CRGB tmp = (*panel)(x, y);
      (*panel)(x, y) = (*panel)(xo, y);
      (*panel)(xo, y) = tmp;
    }
  }
}

void DisplayManager::rotatePanel180(cLEDMatrix<PANEL_WIDTH, PANEL_HEIGHT, VERTICAL_ZIGZAG_MATRIX>* panel) {
  for (uint8_t y = 0; y < PANEL_HEIGHT; y++) {
    for (uint8_t x = 0; x < PANEL_WIDTH / 2; x++) {
      uint8_t xo = PANEL_WIDTH - 1 - x;
      uint8_t yo = PANEL_HEIGHT - 1 - y;
      CRGB tmp = (*panel)(x, y);
      (*panel)(x, y) = (*panel)(xo, yo);
      (*panel)(xo, yo) = tmp;
    }
  }
}

DisplayManager::~DisplayManager() {
  delete panelTop;
  delete panelBottom;
  delete canvas;
}
