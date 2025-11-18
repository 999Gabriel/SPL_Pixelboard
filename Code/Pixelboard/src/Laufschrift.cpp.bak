#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>

#define ledPin         25
#define matrixWidth    32
#define matrixHeight   8
#define ledCount       (matrixWidth * matrixHeight)

#define brightness     25
#define colorOrder     GRB
#define chipset        WS2812

CRGB leds[ledCount];

cLEDMatrix<matrixWidth, matrixHeight, VERTICAL_ZIGZAG_MATRIX> ledMatrix;
cLEDText scrollingText;

static const char* laufText =
  "   HTL Anichstrasse ESP32 Laufschrift   ";

static uint32_t lastFrameMs = 0;
static const uint16_t frameIntervalMs = 5;

static void initAnzeige();
static void updateAnzeige();
static void debugAusgabe();
static void showStartupSequence();
static void fillMatrixWithColor(const CRGB& color);

/**
 * @brief Spiegelt den aktuellen Matrix-Frame horizontal (y-Achse).
 */
static void mirrorFrameHoriz() {
  for (uint8_t y = 0; y < matrixHeight; y++) {
    for (uint8_t x = 0; x < matrixWidth / 2; x++) {
      CRGB tmp = ledMatrix(x, y);
      ledMatrix(x, y) = ledMatrix(matrixWidth - 1 - x, y);
      ledMatrix(matrixWidth - 1 - x, y) = tmp;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(30);

  initAnzeige();
  showStartupSequence();
  Serial.println(F("Laufschrift gestartet (ohne serielle Eingabe)."));
}

void loop() {
  updateAnzeige();
  debugAusgabe();
}

static void initAnzeige() {
  FastLED.addLeds<chipset, ledPin, colorOrder>(leds, ledCount);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  ledMatrix.SetLEDArray(leds);

  scrollingText.SetFont(MatriseFontData);
  scrollingText.Init(&ledMatrix, matrixWidth, matrixHeight, 0, 0);
  scrollingText.SetScrollDirection(SCROLL_LEFT);
  scrollingText.SetFrameRate(30);
  scrollingText.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 255, 0, 255);

  const size_t len = strlen(laufText);
  scrollingText.SetText((unsigned char*)laufText, len);
}

static void updateAnzeige() {
  const uint32_t now = millis();
  if (now - lastFrameMs < frameIntervalMs) return;
  lastFrameMs = now;

  if (scrollingText.UpdateText() == -1) {
    const size_t len = strlen(laufText);
    scrollingText.SetText((unsigned char*)laufText, len);
  }

  // y-Achse Spiegelung (links↔rechts) erzwingen
  mirrorFrameHoriz();

  FastLED.show();
}

static void debugAusgabe() {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < 250) return;
  lastMs = now;

  Serial.print(now);        Serial.print(F(";"));
  Serial.print(brightness); Serial.print(F(";"));
  Serial.print(30);         Serial.print(F(";"));
  Serial.print(matrixWidth);Serial.print(F(";"));
  Serial.println(matrixHeight);
}

static void showStartupSequence() {
  fillMatrixWithColor(CRGB::Red);   delay(500);
  fillMatrixWithColor(CRGB::Green); delay(500);
  fillMatrixWithColor(CRGB::Blue);  delay(500);
  fillMatrixWithColor(CRGB::White); delay(500);
  FastLED.clear(true);
}

static void fillMatrixWithColor(const CRGB& color) {
  for (int i = 0; i < ledCount; i++) leds[i] = color;
  FastLED.show();
}
