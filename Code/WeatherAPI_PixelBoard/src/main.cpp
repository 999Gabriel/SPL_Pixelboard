/**
 * @file Laufschrift_DualPanel_64x16_Wttr.cpp
 * @brief Große Laufschrift über zwei 8x32 Panels (gesamt 32x16),
 *        Text kommt dynamisch von der kostenlosen wttr.in-API.
 *
 * Display-Setup:
 *   - 2x Panels 8x32 (VERTICAL_ZIGZAG_MATRIX)
 *   - Pin 26: physisch oben, kopfüber montiert
 *   - Pin 25: physisch unten
 *   - Virtuelles Canvas: 64x8 → vertikal skaliert auf 64x16
 *   - Mapping & Spiegelung so wie dein funktionierender Code.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>

// -----------------------------------------------------------------------------
// WLAN-Konfiguration
// -----------------------------------------------------------------------------
const char* wifiSsid     = "HTL-IoT";
const char* wifiPassword = "Internet0fThings!";

// -----------------------------------------------------------------------------
// Wetter-API: wttr.in (gratis, kein Key nötig)
// -----------------------------------------------------------------------------
// ACHTUNG: jetzt HTTPS benutzen, damit Cloudflare / Server mitspielen.
const char* weatherApiUrl =
  "https://wttr.in/Innsbruck?format=j1";

// Wetter-Update-Intervall (z. B. alle 10 Minuten)
const uint32_t weatherUpdateIntervalMs = 10UL * 60UL * 1000UL;
uint32_t lastWeatherUpdateMs = 0;

// -----------------------------------------------------------------------------
// LED / Panel-Konfiguration
// -----------------------------------------------------------------------------
#define pinTop         25   // physisch unten
#define pinBottom      26   // physisch oben

#define panelWidth     32
#define panelHeight     8

#define ledsPerPanel   (panelWidth * panelHeight)

#define brightness     25
#define colorOrder     GRB
#define chipset        WS2812

// Virtuelle Canvas-Größen
#define canvasWidth8   64
#define canvasHeight8   8

#define canvasWidth16  64
#define canvasHeight16 16

// --- Virtuelle Buffers -------------------------------------------------------
CRGB canvas8Leds[canvasWidth8 * canvasHeight8];
CRGB canvas16Leds[canvasWidth16 * canvasHeight16];

cLEDMatrix<canvasWidth8,
           canvasHeight8,
           HORIZONTAL_MATRIX> canvas8;

cLEDMatrix<canvasWidth16,
           canvasHeight16,
           HORIZONTAL_MATRIX> canvas16;

// --- Physische Panels --------------------------------------------------------
CRGB ledsTop[ledsPerPanel];      // Pin 25, physisch unten
CRGB ledsBottom[ledsPerPanel];   // Pin 26, physisch oben

cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelTop;     // logisch oben
cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelBottom;  // logisch unten

// --- Laufschrift & Timing ----------------------------------------------------
cLEDText scrollingText;

// Puffer für aktuellen Lauftext
static char laufTextBuffer[160];
static size_t laufTextLen = 0;

uint32_t lastFrameMs = 0;
const uint16_t frameIntervalMs = 5;

// -----------------------------------------------------------------------------
// Funktions-Prototypen
// -----------------------------------------------------------------------------
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

// WLAN / Wetter
static void connectToWifi();
static void updateWeatherIfNeeded();
static bool fetchWeatherAndBuildText();
static void setLaufschriftText(const String& text);

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(50);

  connectToWifi();
  initAnzeige();
  startSequenz();

  // Erstes Wetter-Update direkt beim Start
  if (!fetchWeatherAndBuildText()) {
    setLaufschriftText("   Wetter API Fehler, nutze Fallback-Text   ");
  }
  lastWeatherUpdateMs = millis();

  Serial.println(F("Laufschrift 64x16 mit wttr.in-Wetterdaten gestartet."));
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop() {
  updateAnzeige();
  updateWeatherIfNeeded();
  debugAusgabe();
}

// -----------------------------------------------------------------------------
// Anzeige-Logik
// -----------------------------------------------------------------------------
static void initAnzeige() {
  // Zwei FastLED-Controller:
  // ledsTop    -> pinTop    (25)  -> physisch unten
  // ledsBottom -> pinBottom (26)  -> physisch oben
  FastLED.addLeds<chipset, pinTop,    colorOrder>(ledsTop,    ledsPerPanel);
  FastLED.addLeds<chipset, pinBottom, colorOrder>(ledsBottom, ledsPerPanel);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  // Wichtig: logisches panelTop zeigt auf physisch OBEN
  panelTop.SetLEDArray(ledsBottom);   // Pin 26
  panelBottom.SetLEDArray(ledsTop);   // Pin 25

  // Canvas binden
  canvas8.SetLEDArray(canvas8Leds);
  canvas16.SetLEDArray(canvas16Leds);

  // LEDText vorbereiten, Starttext
  scrollingText.SetFont(MatriseFontData);
  scrollingText.Init(&canvas8,
                     canvas8.Width(),
                     canvas8.Height(),
                     0, 0);
  scrollingText.SetScrollDirection(SCROLL_LEFT);
  scrollingText.SetFrameRate(30);
  scrollingText.SetTextColrOptions(COLR_RGB | COLR_SINGLE,
                                   255, 0, 255);

  setLaufschriftText("   Verbinde WLAN & lade Wetterdaten...   ");
}

static void updateAnzeige() {
  const uint32_t now = millis();
  if (now - lastFrameMs < frameIntervalMs) return;
  lastFrameMs = now;

  // 1) Text auf 64x8 rendern
  if (scrollingText.UpdateText() == -1) {
    scrollingText.SetText((unsigned char*)laufTextBuffer,
                          laufTextLen);
  }

  // 2) Auf 64x16 skalieren
  scaleVertTo16();

  // 3) Eine Zeile nach unten schieben (oben schwarz)
  verschiebeCanvas16EineZeileNachUnten();

  // 4) 64x16 → zwei Panels
  blitPanelsFromCanvas16();

  FastLED.show();
}

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

static void verschiebeCanvas16EineZeileNachUnten() {
  for (int y = canvasHeight16 - 1; y > 0; y--) {
    for (uint8_t x = 0; x < canvasWidth16; x++) {
      canvas16(x, y) = canvas16(x, y - 1);
    }
  }

  for (uint8_t x = 0; x < canvasWidth16; x++) {
    canvas16(x, 0) = CRGB::Black;
  }
}

static void blitPanelsFromCanvas16() {
  // Oberes physisches Panel (Pin 26) bekommt logisches UNTEN (y=8..15)
  for (uint8_t y = 0; y < panelHeight; y++) {
    const uint8_t ySrc = y + panelHeight; // 8..15
    for (uint8_t x = 0; x < panelWidth; x++) {
      panelTop(x, y) = canvas16(x, ySrc);
    }
  }

  // Unteres physisches Panel (Pin 25) bekommt logisches OBEN (y=0..7)
  for (uint8_t y = 0; y < panelHeight; y++) {
    const uint8_t ySrc = y; // 0..7
    for (uint8_t x = 0; x < panelWidth; x++) {
      panelBottom(x, y) = canvas16(x, ySrc);
    }
  }

  // y-Achse-Spiegelung auf beiden Panels
  mirrorPanelHorizontal(panelTop);
  mirrorPanelHorizontal(panelBottom);

  // Kopfüber montiertes Panel (früher unten, jetzt oben) drehen
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

static void startSequenz() {
  // oben = ledsBottom, unten = ledsTop
  fill_solid(ledsBottom, ledsPerPanel, CRGB::Red);
  fill_solid(ledsTop,    ledsPerPanel, CRGB::Blue);
  FastLED.show();
  delay(600);

  fill_solid(ledsBottom, ledsPerPanel, CRGB::White);
  fill_solid(ledsTop,    ledsPerPanel, CRGB::White);
  FastLED.show();
  delay(400);

  fill_solid(ledsBottom, ledsPerPanel, CRGB::Black);
  fill_solid(ledsTop,    ledsPerPanel, CRGB::Black);
  FastLED.show();
}

// -----------------------------------------------------------------------------
// WLAN & Wetter
// -----------------------------------------------------------------------------
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
    Serial.println(F("WLAN-Verbindung fehlgeschlagen (Offline-Modus)."));
  }
}

static void updateWeatherIfNeeded() {
  const uint32_t now = millis();

  if (now - lastWeatherUpdateMs < weatherUpdateIntervalMs) {
    return;
  }

  lastWeatherUpdateMs = now;

  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }

  if (!fetchWeatherAndBuildText()) {
    Serial.println(F("Wetter-Update fehlgeschlagen, behalte bisherigen Text."));
  }
}

/**
 * @brief Holt Wetterdaten von wttr.in (HTTPS + TLS) und baut den Lauftext.
 *
 * JSON-Struktur (vereinfacht):
 * {
 *   "current_condition": [
 *     {
 *       "temp_C": "5",
 *       "weatherDesc": [ { "value": "Partly cloudy" } ]
 *     }
 *   ],
 *   ...
 * }
 */
static bool fetchWeatherAndBuildText() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Kein WLAN, überspringe Wetter-Update."));
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Zertifikat nicht prüfen (einfach, aber unsicher)

  HTTPClient http;
  Serial.print(F("GET "));
  Serial.println(weatherApiUrl);

  http.setTimeout(8000);
  http.setUserAgent("ESP32-WeatherPanel/1.0");

  if (!http.begin(client, weatherApiUrl)) {
    Serial.println(F("http.begin() fehlgeschlagen."));
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("HTTP Fehler: "));
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print(F("JSON Fehler: "));
    Serial.println(err.c_str());
    return false;
  }

  JsonObject current = doc["current_condition"][0];

  const char* tempCStr = current["temp_C"] | "0";
  const char* descStr  = current["weatherDesc"][0]["value"] | "No data";

  float temp = atof(tempCStr);

  String descr(descStr);
  descr.toLowerCase();
  descr.trim();           // trailing spaces bei "Partly Cloudy "
  descr.replace(" ", "_");

  String text = "   INNSBRUCK ";
  text += String(temp, 1);
  text += "C ";
  text += descr;
  text += "   ";

  Serial.print(F("Neuer Wetter-Text: "));
  Serial.println(text);

  setLaufschriftText(text);
  return true;
}

/**
 * @brief Schreibt einen String in den globalen Lauftext-Puffer
 *        und setzt ihn im LEDText-Objekt.
 */
static void setLaufschriftText(const String& text) {
  laufTextLen = text.length();
  if (laufTextLen >= sizeof(laufTextBuffer)) {
    laufTextLen = sizeof(laufTextBuffer) - 1;
  }

  text.substring(0, laufTextLen).toCharArray(laufTextBuffer,
                                             sizeof(laufTextBuffer));
  laufTextBuffer[laufTextLen] = '\0';

  scrollingText.SetText((unsigned char*)laufTextBuffer,
                        laufTextLen);
}

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------
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
