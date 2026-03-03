/**
 * @file Uhr_Laufschrift_64x16.cpp
 * @brief NTP-Uhrzeit als Laufschrift über zwei 8x32 Panels (gesamt 64x16).
 *
 * Pipeline:
 *   - Virtuelles Canvas (64x8) mit LEDText (Laufschrift)
 *   - Vertikal auf 64x16 skaliert
 *   - Eine Zeile nach unten verschoben (Top-Rand frei)
 *   - Auf zwei 8x32 Panels gemappt (Pin 25 / 26) mit Spiegel- und
 *     Rotationskorrektur wie im funktionierenden Code.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>

// ---------------------------------------------------------------------------
// WLAN + NTP
// ---------------------------------------------------------------------------
const char* wifiSsid     = "GabPhone";
const char* wifiPassword = "Covquf012wud";

const long gmtOffsetSec      = 3600;   // UTC+1
const int  daylightOffsetSec = 0;      // Winterzeit
const char* ntpServer1       = "pool.ntp.org";
const char* ntpServer2       = "time.nist.gov";

// ---------------------------------------------------------------------------
// LED / Panels
// ---------------------------------------------------------------------------
#define pinTop         25   // physisch unten
#define pinBottom      26   // physisch oben

#define panelWidth     32
#define panelHeight     8
#define ledsPerPanel   (panelWidth * panelHeight)

#define brightness     25
#define colorOrder     GRB
#define chipset        WS2812

// virtuelles Canvas
#define canvasWidth8   64
#define canvasHeight8   8
#define canvasWidth16  64
#define canvasHeight16 16

// virtuelle Buffers
CRGB canvas8Leds[canvasWidth8 * canvasHeight8];
CRGB canvas16Leds[canvasWidth16 * canvasHeight16];

cLEDMatrix<canvasWidth8,
           canvasHeight8,
           HORIZONTAL_MATRIX> canvas8;

cLEDMatrix<canvasWidth16,
           canvasHeight16,
           HORIZONTAL_MATRIX> canvas16;

// physische Panele
CRGB ledsTop[ledsPerPanel];      // Pin 25, physisch unten
CRGB ledsBottom[ledsPerPanel];   // Pin 26, physisch oben

cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelTop;     // logisch oben
cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelBottom;  // logisch unten

// Laufschrift
cLEDText scrollingText;

// Textpuffer
static char laufTextBuffer[64];
static size_t laufTextLen = 0;

// Timing
static uint32_t lastFrameMs        = 0;
static const uint16_t frameIntervalMs = 10;   // Scroll-Geschwindigkeit

static int lastMinute = -1;

// ---------------------------------------------------------------------------
// Prototypen
// ---------------------------------------------------------------------------
static void connectToWifi();
static void initTimeNtp();
static void initAnzeige();

static void updateAnzeige();
static void scaleVertTo16();
static void shiftCanvas16Down();
static void blitPanelsFromCanvas16();

static void mirrorPanelHorizontal(
  cLEDMatrix<panelWidth, panelHeight, VERTICAL_ZIGZAG_MATRIX> &panel);
static void rotatePanel180(
  cLEDMatrix<panelWidth, panelHeight, VERTICAL_ZIGZAG_MATRIX> &panel);

static void setLaufschriftText(const String& text);
static void updateTimeTextIfNeeded();

static void debugAusgabe();

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(50);

  connectToWifi();
  initTimeNtp();
  initAnzeige();

  // Starttext, bis gültige Zeit da ist
  setLaufschriftText("   Starte Uhr...   ");

  Serial.println(F("NTP-Zeit-Laufschrift 64x16 gestartet."));
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
  updateTimeTextIfNeeded();
  updateAnzeige();
  debugAusgabe();
}

// ---------------------------------------------------------------------------
// WLAN + NTP
// ---------------------------------------------------------------------------
static void connectToWifi() {
  Serial.print(F("Verbinde mit WLAN: "));
  Serial.println(wifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WLAN verbunden, IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WLAN-Verbindung fehlgeschlagen (Uhr läuft lokal)."));
  }
}

static void initTimeNtp() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Kein WLAN, NTP übersprungen."));
    return;
  }

  Serial.println(F("Initialisiere NTP..."));
  configTime(gmtOffsetSec,
             daylightOffsetSec,
             ntpServer1,
             ntpServer2);

  // kurzer Check, ob Zeit gesetzt wurde
  time_t now = time(nullptr);
  uint32_t startMs = millis();
  while (now < 100000 &&
         (millis() - startMs) < 10000) {
    delay(500);
    now = time(nullptr);
  }

  if (now < 100000) {
    Serial.println(F("NTP-Sync fehlgeschlagen, nutze Epoch-Zeit."));
  } else {
    struct tm timeInfo;
    localtime_r(&now, &timeInfo);
    Serial.print(F("Zeit synchronisiert: "));
    Serial.print(timeInfo.tm_hour);
    Serial.print(F(":"));
    Serial.println(timeInfo.tm_min);
  }
}

// ---------------------------------------------------------------------------
// Anzeige-Initialisierung
// ---------------------------------------------------------------------------
static void initAnzeige() {
  // FastLED Controller
  FastLED.addLeds<chipset, pinTop,    colorOrder>(ledsTop,
                                                  ledsPerPanel);
  FastLED.addLeds<chipset, pinBottom, colorOrder>(ledsBottom,
                                                  ledsPerPanel);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  // logische/physische Zuordnung (bewährt aus vorherigem Code):
  // - panelTop = physisch oben → ledsBottom (Pin 26)
  // - panelBottom = physisch unten → ledsTop (Pin 25)
  panelTop.SetLEDArray(ledsBottom);   // Pin 26
  panelBottom.SetLEDArray(ledsTop);   // Pin 25

  // virtuelle Canvas
  canvas8.SetLEDArray(canvas8Leds);
  canvas16.SetLEDArray(canvas16Leds);

  // LEDText Vorbereitung
  scrollingText.SetFont(MatriseFontData);
  scrollingText.Init(&canvas8,
                     canvas8.Width(),
                     canvas8.Height(),
                     0, 0);
  scrollingText.SetScrollDirection(SCROLL_LEFT);
  scrollingText.SetFrameRate(30);
  scrollingText.SetTextColrOptions(COLR_RGB | COLR_SINGLE,
                                   0, 255, 255); // Cyan
}

// ---------------------------------------------------------------------------
// Zeit → Text (HH:MM) und Laufschrift setzen
// ---------------------------------------------------------------------------
static void updateTimeTextIfNeeded() {
  time_t now = time(nullptr);
  struct tm timeInfo;
  localtime_r(&now, &timeInfo);

  int currentMinute = timeInfo.tm_min;

  // Nur updaten, wenn sich die Minute geändert hat
  if (currentMinute == lastMinute) return;
  lastMinute = currentMinute;

  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr),
           "%02d:%02d",
           timeInfo.tm_hour,
           timeInfo.tm_min);

  String text = "   ";
  text += timeStr;
  text += "   ";

  Serial.print(F("Neue Uhrzeit-Laufschrift: "));
  Serial.println(text);

  setLaufschriftText(text);
}

static void setLaufschriftText(const String& text) {
  laufTextLen = text.length();
  if (laufTextLen >= sizeof(laufTextBuffer)) {
    laufTextLen = sizeof(laufTextBuffer) - 1;
  }

  text.substring(0, laufTextLen)
      .toCharArray(laufTextBuffer, sizeof(laufTextBuffer));

  laufTextBuffer[laufTextLen] = '\0';

  scrollingText.SetText((unsigned char*)laufTextBuffer,
                        laufTextLen);
}

// ---------------------------------------------------------------------------
// Anzeige-Update
// ---------------------------------------------------------------------------
static void updateAnzeige() {
  const uint32_t now = millis();
  if (now - lastFrameMs < frameIntervalMs) return;
  lastFrameMs = now;

  // 1) Laufschrift auf 64x8-Canvas rendern
  if (scrollingText.UpdateText() == -1) {
    // Wenn der Durchlauf fertig ist, wieder von vorne starten
    scrollingText.SetText((unsigned char*)laufTextBuffer,
                          laufTextLen);
  }

  // 2) Vertikal auf 64x16 skalieren
  scaleVertTo16();

  // 3) Eine Zeile nach unten schieben (Top-Rand)
  shiftCanvas16Down();

  // 4) Canvas16 → zwei Panels (mit Korrekturen)
  blitPanelsFromCanvas16();

  FastLED.show();
}

// Skaliert 64x8 → 64x16 (jede Zeile doppelt)
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

// verschiebt alles im 64x16-Canvas um 1 Pixel nach unten
static void shiftCanvas16Down() {
  for (int y = canvasHeight16 - 1; y > 0; y--) {
    for (uint8_t x = 0; x < canvasWidth16; x++) {
      canvas16(x, y) = canvas16(x, y - 1);
    }
  }

  for (uint8_t x = 0; x < canvasWidth16; x++) {
    canvas16(x, 0) = CRGB::Black;
  }
}

// 64x16-Canvas → zwei 8x32-Panels mappen + Spiegel/Rotation
static void blitPanelsFromCanvas16() {
  // logisches oben (y=8..15) → physisch oben (panelTop)
  for (uint8_t y = 0; y < panelHeight; y++) {
    const uint8_t ySrc = y + panelHeight;  // 8..15
    for (uint8_t x = 0; x < panelWidth; x++) {
      panelTop(x, y) = canvas16(x, ySrc);
    }
  }

  // logisches unten (y=0..7) → physisch unten (panelBottom)
  for (uint8_t y = 0; y < panelHeight; y++) {
    const uint8_t ySrc = y;                // 0..7
    for (uint8_t x = 0; x < panelWidth; x++) {
      panelBottom(x, y) = canvas16(x, ySrc);
    }
  }

  // Spiegelung über y-Achse (wie im funktionierenden Code)
  mirrorPanelHorizontal(panelTop);
  mirrorPanelHorizontal(panelBottom);

  // PanelTop ist kopfüber montiert → 180° drehen
  rotatePanel180(panelTop);
}

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

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------
static void debugAusgabe() {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < 1000) return;
  lastMs = now;

  Serial.print(now);            Serial.print(F(";"));
  Serial.print(brightness);     Serial.print(F(";"));
  Serial.print(1000 / frameIntervalMs);
  Serial.print(F(";"));
  Serial.print(canvasWidth16);  Serial.print(F(";"));
  Serial.println(canvasHeight16);
}