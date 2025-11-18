/**
 * @file Laufschrift_DualPanel_64x16.cpp
 * @brief Große Laufschrift über zwei 8x32 Panels (gesamt 32x16).
 *
 * Virtuelles Bild:
 *   - LEDText rendert auf 64x8 (Fonthöhe 8)
 *   - Wir skalieren vertikal auf 64x16 (2x)
 *   - Danach verschieben wir den kompletten Text um 1 Zeile nach unten,
 *     sodass Zeile 0 schwarz bleibt.
 *
 * Physische Panels:
 *   - Oberes Panel:  32x8, VERTICAL_ZIGZAG_MATRIX, an GPIO 25
 *   - Unteres Panel: 32x8, VERTICAL_ZIGZAG_MATRIX, an GPIO 26, kopfüber montiert
 *
 * Mapping:
 *   - Oben:  bekommt die UNTERE Hälfte (y=8..15) des 64x16-Bildes
 *   - Unten: bekommt die OBERE Hälfte (y=0..7) des 64x16-Bildes
 *   - Beide Panels werden horizontal gespiegelt (y-Achse)
 *   - Unteres Panel zusätzlich um 180° gedreht
 */

#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>

// --- Hardware-Konfiguration --------------------------------------------------
#define pinTop         25
#define pinBottom      26

#define panelWidth     32
#define panelHeight     8

#define ledsPerPanel   (panelWidth * panelHeight)

#define brightness     25
#define colorOrder     GRB
#define chipset        WS2812

// --- Virtuelles Canvas -------------------------------------------------------
#define canvasWidth8   64
#define canvasHeight8   8

#define canvasWidth16  64
#define canvasHeight16 16

CRGB canvas8Leds[canvasWidth8 * canvasHeight8];
CRGB canvas16Leds[canvasWidth16 * canvasHeight16];

cLEDMatrix<canvasWidth8,
           canvasHeight8,
           HORIZONTAL_MATRIX> canvas8;

cLEDMatrix<canvasWidth16,
           canvasHeight16,
           HORIZONTAL_MATRIX> canvas16;

// --- Physische Panels --------------------------------------------------------
CRGB ledsTop[ledsPerPanel];
CRGB ledsBottom[ledsPerPanel];

cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelTop;

cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelBottom;

// --- Laufschrift & Timing ----------------------------------------------------
cLEDText scrollingText;

static const char* laufText =
  "   HTL Anichstrasse  |  ESP32 Laufschrift  |  Q-LABS   ";

static uint32_t lastFrameMs = 0;
static const uint16_t frameIntervalMs = 5;

// --- Prototypen --------------------------------------------------------------
static void initAnzeige();
static void updateAnzeige();
static void startSequenz();
static void debugAusgabe();

static void scaleVertTo16();
static void verschiebeCanvas16EineZeileNachUnten();
static void blitPanelsFromCanvas16();

static void mirrorPanelHorizontal(
  cLEDMatrix<panelWidth, panelHeight, VERTICAL_ZIGZAG_MATRIX> &panel);

static void rotatePanel180(
  cLEDMatrix<panelWidth, panelHeight, VERTICAL_ZIGZAG_MATRIX> &panel);

// --- Setup -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(30);

  initAnzeige();
  startSequenz();

  Serial.println(F("Laufschrift 64x16 über zwei Panels gestartet."));
}

// --- Loop --------------------------------------------------------------------
void loop() {
  updateAnzeige();
  debugAusgabe();
}

// --- Implementierung ---------------------------------------------------------
static void initAnzeige() {
  FastLED.addLeds<chipset, pinTop,    colorOrder>(ledsTop,    ledsPerPanel);
  FastLED.addLeds<chipset, pinBottom, colorOrder>(ledsBottom, ledsPerPanel);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  panelTop.SetLEDArray(ledsTop);
  panelBottom.SetLEDArray(ledsBottom);

  canvas8.SetLEDArray(canvas8Leds);
  canvas16.SetLEDArray(canvas16Leds);

  scrollingText.SetFont(MatriseFontData);
  scrollingText.Init(&canvas8,
                     canvas8.Width(),
                     canvas8.Height(),
                     0, 0);
  scrollingText.SetScrollDirection(SCROLL_LEFT);
  scrollingText.SetFrameRate(30);
  scrollingText.SetTextColrOptions(COLR_RGB | COLR_SINGLE,
                                   255, 0, 255);

  const size_t len = strlen(laufText);
  scrollingText.SetText((unsigned char*)laufText, len);
}

static void updateAnzeige() {
  const uint32_t now = millis();
  if (now - lastFrameMs < frameIntervalMs) return;
  lastFrameMs = now;

  // 1) Text auf 64x8 rendern
  if (scrollingText.UpdateText() == -1) {
    const size_t len = strlen(laufText);
    scrollingText.SetText((unsigned char*)laufText, len);
  }

  // 2) Auf 64x16 skalieren (jede Zeile verdoppeln)
  scaleVertTo16();

  // 3) Gesamtes Bild um 1 Zeile nach unten verschieben,
  //    damit Zeile 0 oben schwarz bleibt
  verschiebeCanvas16EineZeileNachUnten();

  // 4) 64x16 → zwei 32x8 Panels mappen
  blitPanelsFromCanvas16();

  FastLED.show();
}

/**
 * @brief Verdoppelt jede Zeile von canvas8 (64x8) nach canvas16 (64x16).
 */
static void scaleVertTo16() {
  for (uint8_t y8 = 0; y8 < canvasHeight8; y8++) {
    const uint8_t y16a = 2 * y8;
    const uint8_t y16b = y16a + 1;

    for (uint8_t x = 0; x < canvasWidth8; x++) {
      const CRGB c = canvas8(x, y8);
      canvas16(x, y16a) = c;
      canvas16(x, y16b) = c;
    }
  }
}

/**
 * @brief Verschiebt das 64x16-Bild um 1 Zeile nach unten:
 *        - Zeilen 15..1 bekommen den Inhalt von 14..0
 *        - Zeile 0 oben wird schwarz
 *
 * Ergebnis:
 *   - Ganz oben (y=0) bleibt eine schwarze Linie
 *   - Der unterste ursprüngliche Pixel geht verloren
 *     (optisch kaum sichtbar, aber du bekommst den Top-Rand).
 */
static void verschiebeCanvas16EineZeileNachUnten() {
  for (int y = canvasHeight16 - 1; y > 0; y--) {
    for (uint8_t x = 0; x < canvasWidth16; x++) {
      canvas16(x, y) = canvas16(x, y - 1);
    }
  }

  // Zeile 0 oben löschen
  for (uint8_t x = 0; x < canvasWidth16; x++) {
    canvas16(x, 0) = CRGB::Black;
  }
}

/**
 * @brief Mapped 64x16 → beide Panels + Orientierungsfix.
 *
 * Logik:
 *   - Oben (physisch TOP) bekommt logisches UNTEN (y=8..15)
 *   - Unten (physisch BOTTOM) bekommt logisches OBEN (y=0..7)
 *   - Beide Panels horizontal spiegeln
 *   - Unteres Panel zusätzlich um 180° drehen
 */
static void blitPanelsFromCanvas16() {
  // Top-Panel: logisches unten → physisch oben
  for (uint8_t y = 0; y < panelHeight; y++) {
    const uint8_t ySrc = y + panelHeight; // 8..15
    for (uint8_t x = 0; x < panelWidth; x++) {
      panelTop(x, y) = canvas16(x, ySrc);
    }
  }

  // Bottom-Panel: logisches oben → physisch unten
  for (uint8_t y = 0; y < panelHeight; y++) {
    const uint8_t ySrc = y; // 0..7
    for (uint8_t x = 0; x < panelWidth; x++) {
      panelBottom(x, y) = canvas16(x, ySrc);
    }
  }

  // y-Achse-Spiegelung auf beiden Panels
  mirrorPanelHorizontal(panelTop);
  mirrorPanelHorizontal(panelBottom);

  // Unteres Panel wegen kopfüber Montage drehen
  rotatePanel180(panelBottom);
}

/**
 * @brief Horizontale Spiegelung (links↔rechts) für ein Panel.
 */
static void mirrorPanelHorizontal(
  cLEDMatrix<panelWidth, panelHeight, VERTICAL_ZIGZAG_MATRIX> &panel
) {
  for (uint8_t y = 0; y < panelHeight; y++) {
    for (uint8_t x = 0; x < panelWidth / 2; x++) {
      const uint8_t xo = panelWidth - 1 - x;
      CRGB tmp      = panel(x, y);
      panel(x, y)   = panel(xo, y);
      panel(xo, y)  = tmp;
    }
  }
}

/**
 * @brief 180°-Drehung für ein Panel.
 */
static void rotatePanel180(
  cLEDMatrix<panelWidth, panelHeight, VERTICAL_ZIGZAG_MATRIX> &panel
) {
  for (uint8_t y = 0; y < panelHeight; y++) {
    for (uint8_t x = 0; x < panelWidth / 2; x++) {
      const uint8_t xo = panelWidth - 1 - x;
      const uint8_t yo = panelHeight - 1 - y;

      CRGB tmp       = panel(x, y);
      panel(x, y)    = panel(xo, yo);
      panel(xo, yo)  = tmp;
    }
  }
}

/**
 * @brief Starttest: oben rot, unten blau, dann beide weiß, dann clear.
 */
static void startSequenz() {
  fill_solid(ledsTop,    ledsPerPanel, CRGB::Black);
  fill_solid(ledsBottom, ledsPerPanel, CRGB::Black);
  FastLED.show();
  delay(200);

  fill_solid(ledsTop,    ledsPerPanel, CRGB::Red);
  fill_solid(ledsBottom, ledsPerPanel, CRGB::Blue);
  FastLED.show();
  delay(600);

  fill_solid(ledsTop,    ledsPerPanel, CRGB::White);
  fill_solid(ledsBottom, ledsPerPanel, CRGB::White);
  FastLED.show();
  delay(400);

  fill_solid(ledsTop,    ledsPerPanel, CRGB::Black);
  fill_solid(ledsBottom, ledsPerPanel, CRGB::Black);
  FastLED.show();
}

/**
 * @brief Debug-Ausgabe alle ~250 ms.
 */
static void debugAusgabe() {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < 250) return;
  lastMs = now;

  Serial.print(now);            Serial.print(F(";"));
  Serial.print(brightness);     Serial.print(F(";"));
  Serial.print(30);             Serial.print(F(";"));
  Serial.print(canvasWidth16);  Serial.print(F(";"));
  Serial.println(canvasHeight16);
}
