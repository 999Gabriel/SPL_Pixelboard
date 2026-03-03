#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "SharedLEDConfig.h"

// -----------------------------------------------------------------------------
// WLAN-Konfiguration
// -----------------------------------------------------------------------------
const char* wifiSsid     = "DEIN_HOTSPOT_NAME";
const char* wifiPassword = "DEIN_HOTSPOT_PASSWORT";

// Open-Meteo API für Innsbruck (ca. 47.27 N, 11.40 E)
const char* weatherApiUrl =
  "http://api.open-meteo.com/v1/forecast"
  "?latitude=47.27&longitude=11.40"
  "&current_weather=true"
  "&timezone=auto";

// Wetter-Update-Intervall (z. B. alle 10 Minuten) - definiert in Laufschrift_2_panels.cpp
extern const uint32_t weatherUpdateIntervalMs;
extern uint32_t lastWeatherUpdateMs;

// Puffer für aktuellen Lauftext
static char laufTextBuffer[160];
static size_t laufTextLen = 0;


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

// WLAN / Wetter - NOT static (linked from Laufschrift_2_panels.cpp)
void connectToWifi();
void updateWeatherIfNeeded();
bool fetchWeatherAndBuildText();
void setLaufschriftText(const String& text);
const char* mapWeatherCodeToText(int code);


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
void connectToWifi() {
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
    Serial.println(F("WLAN-Verbindung fehlgeschlagen (weiter im Offline-Modus)."));
  }
}

void updateWeatherIfNeeded() {
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
 * @brief Holt Wetterdaten von Open-Meteo und baut den Lauftext.
 *
 * Beispiel-JSON (gekürzt):
 * {
 *   "latitude": ...,
 *   "longitude": ...,
 *   "current_weather": {
 *     "temperature": 13.4,
 *     "windspeed": 7.2,
 *     "weathercode": 3,
 *     "time": "2025-03-01T12:00"
 *   }
 * }
 */
bool fetchWeatherAndBuildText() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Kein WLAN, überspringe Wetter-Update."));
    return false;
  }

  HTTPClient http;
  Serial.print(F("GET "));
  Serial.println(weatherApiUrl);

  http.begin(weatherApiUrl);
  http.setTimeout(8000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("HTTP Fehler: "));
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print(F("JSON Fehler: "));
    Serial.println(err.c_str());
    return false;
  }

  JsonObject current = doc["current_weather"];
  float temp = current["temperature"] | 0.0f;
  int code   = current["weathercode"] | -1;

  const char* descr = mapWeatherCodeToText(code);

  // Stadtname statisch, weil Open-Meteo nur Koordinaten kennt
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
void setLaufschriftText(const String& text) {
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

/**
 * @brief Einfaches Mapping der Open-Meteo-Wettercodes auf kurze deutsche Texte.
 *        (nicht vollständig, aber für Demo ausreichend)
 *
 * Codes siehe: https://open-meteo.com/en/docs
 */
const char* mapWeatherCodeToText(int code) {
  switch (code) {
    case 0:  return "klar";
    case 1:
    case 2:  return "leicht_bewoelkt";
    case 3:  return "bedeckt";
    case 45:
    case 48: return "Nebel";
    case 51:
    case 53:
    case 55: return "Nieselregen";
    case 61:
    case 63:
    case 65: return "Regen";
    case 71:
    case 73:
    case 75: return "Schnee";
    case 80:
    case 81:
    case 82: return "Regenschauer";
    case 95: return "Gewitter";
    case 96:
    case 99: return "Gewitter_stark";
    default: return "Wettercode";
  }
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
