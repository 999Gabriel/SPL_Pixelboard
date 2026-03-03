#include <Arduino.h>
#include <FastLED.h>

// LED Strips - 2 rows only!
#define PIN_TOP    25    // Top row: 32 LEDs
#define PIN_BOTTOM 26    // Bottom row: 32 LEDs (rotated 180°)
#define NUM_LEDS_PER_ROW 32

CRGB ledsTop[NUM_LEDS_PER_ROW];     // Top row (0-31)
CRGB ledsBottom[NUM_LEDS_PER_ROW];  // Bottom row (0-31, but rotated 180°)

// Menu system
const int MENU_ITEMS = 2;
const char* menuNames[MENU_ITEMS] = {"Snake", "Exit"};
int selectedMenuItem = 0;

void setPixel(int x, int y, CRGB color) {
  // x: 0-31 (position in row)
  // y: 0 = top row, 1 = bottom row
  if (x < 0 || x >= NUM_LEDS_PER_ROW) return;

  if (y == 0) {
    // Top row (normal order)
    ledsTop[x] = color;
  } else if (y == 1) {
    // Bottom row (rotated 180° - reverse the index)
    // Since it's rotated, x=0 appears on the right, x=31 on the left
    // So we directly use the reversed index
    ledsBottom[NUM_LEDS_PER_ROW - 1 - x] = color;
  }
}

void drawMenu() {
  // Left side menu: 2 items stacked vertically

  // Menu item 1: "Snake" (top row - left column x=0-7)
  CRGB color1 = (selectedMenuItem == 0) ? CRGB::Green : CRGB::White;

  // Menu item 2: "Exit" (bottom row - left column x=0-7)
  CRGB color2 = (selectedMenuItem == 1) ? CRGB::Green : CRGB::White;

  // Top row: Snake on left (x=0-7)
  for (int x = 0; x < 8; x++) {
    setPixel(x, 0, color1);
  }

  // Bottom row: place the SELECTED (green) block so it appears on the visual left.
  // The bottom strip is inverted; to appear on the left, draw on the right side.
  int bottomStart = (selectedMenuItem == 1) ? 24 : 0;
  int bottomEnd = bottomStart + 8;
  for (int x = bottomStart; x < bottomEnd; x++) {
    setPixel(x, 1, color2);
  }

  // Right side (x=8-31): empty/black (reserved for content area)
  for (int x = 8; x < NUM_LEDS_PER_ROW; x++) {
    setPixel(x, 0, CRGB::Black);
  }
  // Bottom row: clear everything except the menu block
  for (int x = 0; x < NUM_LEDS_PER_ROW; x++) {
    if (x < bottomStart || x >= bottomEnd) {
      setPixel(x, 1, CRGB::Black);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Menu System - Snake/Exit"));

  // Initialize LEDs
  FastLED.addLeds<WS2812, PIN_TOP, GRB>(ledsTop, NUM_LEDS_PER_ROW);
  FastLED.addLeds<WS2812, PIN_BOTTOM, GRB>(ledsBottom, NUM_LEDS_PER_ROW);
  FastLED.setBrightness(80);
  FastLED.clear(true);

  Serial.println(F("Setup complete. Menu initialized."));
}

void loop() {
  // Render menu
  drawMenu();
  FastLED.show();

  delay(20);
}
