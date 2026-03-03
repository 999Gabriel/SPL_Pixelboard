#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// WLAN-Zugangsdaten (iPhone Hotspot)
// WICHTIG: iPhone Hotspot muss auf 2.4 GHz stehen!
// Einstellungen > Persoenlicher Hotspot > "Kompatibilitaet maximieren" AKTIVIEREN
const char* ssid = "GabPhone";
const char* password = "Covquf012wud";

// OpenWeatherMap API
const char* apiKey = "711b0df3e461b3c35e8ca67b28920759";
const char* city = "Innsbruck,AT";
String apiUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&units=metric&appid=" + String(apiKey);

// Matrix-Konfiguration
#define DATA_PIN_UPPER 25
#define DATA_PIN_LOWER 26
#define NUM_LEDS_PER_STRIP 256
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define MATRIX_TYPE VERTICAL_ZIGZAG_MATRIX

CRGB leds_upper[NUM_LEDS_PER_STRIP];
CRGB leds_lower[NUM_LEDS_PER_STRIP];
CRGB leds_array[NUM_LEDS_PER_STRIP * 2];
cLEDMatrix<-MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

// LED Mapping-Struktur
struct LedAddress {
  CRGB* array;
  int index;
};

// Mapping (Original aus funktionierendem Snake-Code)
LedAddress mapXY(int x, int y) {
  LedAddress result;
  int led;

  if (y < 8) {
    led = x * 8 + ((x % 2 == 0) ? y : 7 - y);
    result.array = leds_lower;
    result.index = led;
  } else {
    int flippedX = MATRIX_WIDTH - 1 - x;
    int flippedY = 15 - y;
    led = flippedX * 8 + ((flippedX % 2 == 0) ? flippedY : 7 - flippedY);
    result.array = leds_upper;
    result.index = led;
  }
  return result;
}

// Font 3x5 (optimiert für Punkt & Einheiten)
const byte font3x5[][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
  {0b010, 0b110, 0b010, 0b010, 0b111},  // 1
  {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
  {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
  {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
  {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
  {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
  {0b111, 0b001, 0b010, 0b100, 0b100},  // 7
  {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
  {0b111, 0b101, 0b111, 0b001, 0b111},  // 9
  {0b000, 0b000, 0b000, 0b010, 0b000},  // . (10)
  {0b011, 0b100, 0b100, 0b100, 0b011},  // C (11)
  {0b101, 0b001, 0b010, 0b100, 0b101},  // % (12)
  {0b000, 0b110, 0b101, 0b101, 0b101},  // m (13)
  {0b001, 0b010, 0b010, 0b100, 0b000},  // / (14)
  {0b111, 0b100, 0b111, 0b001, 0b111},  // s (15)
  {0b000, 0b010, 0b000, 0b000, 0b000},  // ° (16) - Gradzeichen (kleiner Punkt oben)
  {0b000, 0b000, 0b111, 0b000, 0b000},  // - (17) - Minuszeichen
};

// Werte
float temperature = 0.0;
int humidity = 0;
float windSpeed = 0.0;
bool dataReceived = false;

// Alle LEDs sicher loeschen
void clearAll() {
  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    leds_upper[i] = CRGB::Black;
    leds_lower[i] = CRGB::Black;
  }
}

// Einzelnen Pixel setzen via mapXY (Display um 180° gedreht)
void setPixel(int x, int y, CRGB color) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
  LedAddress addr = mapXY(MATRIX_WIDTH - 1 - x, MATRIX_HEIGHT - 1 - y);
  addr.array[addr.index] = color;
}

// Zeichen anzeigen
// Font row 0 = oberste Zeile des Zeichens, y = Display-Y der obersten Zeile
void drawChar(int index, int x, int y, CRGB color) {
  if (index < 0 || index >= 18) return;
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      if ((font3x5[index][row] >> (2 - col)) & 0x01) {
        setPixel(x + col, y + row, color);
      }
    }
  }
}

// Textbreite berechnen (fuer Zentrierung)
int getTextWidth(String text) {
  int width = 0;
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);
    if (c >= '0' && c <= '9') width += 4;
    else if (c == '.') width += 4;
    else if (c == 'C') width += 4;
    else if (c == '%') width += 4;
    else if (c == 'm') width += 4;
    else if (c == '/') width += 4;
    else if (c == 's') width += 4;
    else if (c == '*') width += 4;  // ° Gradzeichen
    else if (c == '-') width += 4;
    else if (c == ' ') width += 2;  // Leerzeichen schmaler
  }
  // Letztes Zeichen hat keinen Trailing-Space, also -1
  if (width > 0) width -= 1;
  return width;
}

// Text zentriert anzeigen
void drawTextCentered(String text, int y, CRGB color) {
  int textW = getTextWidth(text);
  int x = (MATRIX_WIDTH - textW) / 2;  // Horizontal zentrieren

  int pos = 0;
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);
    int index = -1;

    if (c >= '0' && c <= '9') index = c - '0';
    else if (c == '.') index = 10;
    else if (c == 'C') index = 11;
    else if (c == '%') index = 12;
    else if (c == 'm') index = 13;
    else if (c == '/') index = 14;
    else if (c == 's') index = 15;
    else if (c == '*') index = 16;  // ° = '*' im String
    else if (c == '-') index = 17;
    else if (c == ' ') { pos += 2; continue; }  // Leerzeichen

    if (index != -1) {
      drawChar(index, x + pos, y, color);
      pos += 4;
    }
  }
}

// WLAN verbinden (mit Retries und Timeout)
void connectWiFi() {
  Serial.println();
  Serial.println("===========================================");
  Serial.println("ESP32 unterstuetzt NUR 2.4 GHz WLAN!");
  Serial.println("iPhone: Einstellungen > Persoenlicher Hotspot");
  Serial.println("  -> 'Kompatibilitaet maximieren' AKTIVIEREN");
  Serial.println("===========================================");
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  Serial.print("Verbinde mit: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int tries = 0;
  int maxTries = 40;  // 20 Sekunden Timeout

  while (WiFi.status() != WL_CONNECTED && tries < maxTries) {
    delay(500);
    Serial.print(".");
    tries++;

    // Nach 10 Versuchen nochmal neu versuchen
    if (tries == 20) {
      Serial.println();
      Serial.println("Neuversuch...");
      WiFi.disconnect(true);
      delay(500);
      WiFi.begin(ssid, password);
    }
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WLAN verbunden! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WLAN-Verbindung FEHLGESCHLAGEN!");
    Serial.println("Pruefe: Ist 'Kompatibilitaet maximieren' aktiv?");
    Serial.print("WiFi Status Code: ");
    Serial.println(WiFi.status());
  }
}

// WLAN-Verbindung pruefen und ggf. neu verbinden
void ensureWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN verloren, verbinde neu...");
    connectWiFi();
  }
}

// Wetterdaten abrufen
void fetchWeatherData(void *pvParameters) {
  // Erstes Update sofort nach Verbindung
  delay(2000);

  while (true) {
    ensureWiFi();

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(10000);
      http.begin(apiUrl);
      int httpCode = http.GET();

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          temperature = doc["main"]["temp"];
          humidity = doc["main"]["humidity"];
          windSpeed = doc["wind"]["speed"];
          dataReceived = true;
          Serial.printf("Temp: %.1f°C | Humidity: %d%% | Wind: %.1f m/s\n",
                        temperature, humidity, windSpeed);
        } else {
          Serial.print("JSON Fehler: ");
          Serial.println(error.c_str());
        }
      } else {
        Serial.print("HTTP Fehler: ");
        Serial.println(httpCode);
      }
      http.end();
    }
    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
}

// Anzeige - zentriert und kompakt
void updateDisplay(void *pvParameters) {
  // Warten bis erste Daten da sind
  while (!dataReceived) {
    clearAll();
    // "---" anzeigen waehrend geladen wird
    drawTextCentered("---", 6, CRGB(80, 80, 80));
    FastLED.show();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  while (true) {
    char buffer[16];

    // === Temperatur (zentriert, vertikal: (16-5)/2 = 5) ===
    clearAll();
    if (temperature < 0) {
      snprintf(buffer, sizeof(buffer), "-%.1f*C", -temperature);
    } else {
      snprintf(buffer, sizeof(buffer), "%.1f*C", temperature);
    }
    drawTextCentered(String(buffer), 6, CRGB::Green);
    FastLED.show();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // === Luftfeuchtigkeit (zentriert) ===
    clearAll();
    snprintf(buffer, sizeof(buffer), "%d%%", humidity);
    drawTextCentered(String(buffer), 6, CRGB::Yellow);
    FastLED.show();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // === Windgeschwindigkeit (zentriert) ===
    clearAll();
    snprintf(buffer, sizeof(buffer), "%.1fm/s", windSpeed);
    drawTextCentered(String(buffer), 6, CRGB::Blue);
    FastLED.show();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  FastLED.addLeds<WS2812B, DATA_PIN_UPPER, GRB>(leds_upper, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, DATA_PIN_LOWER, GRB>(leds_lower, NUM_LEDS_PER_STRIP);
  FastLED.setBrightness(50);

  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    leds_array[i] = leds_lower[i];
    leds_array[i + NUM_LEDS_PER_STRIP] = leds_upper[i];
  }
  leds.SetLEDArray(leds_array);

  clearAll();
  FastLED.show();

  // WLAN verbinden
  connectWiFi();

  // FreeRTOS Tasks
  xTaskCreatePinnedToCore(fetchWeatherData, "WeatherTask", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(updateDisplay, "DisplayTask", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}