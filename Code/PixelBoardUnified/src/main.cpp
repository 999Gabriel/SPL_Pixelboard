#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <time.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef WEATHER_API_KEY
#define WEATHER_API_KEY ""
#endif

#ifndef WEATHER_CITY
#define WEATHER_CITY "Innsbruck,AT"
#endif

#ifndef GOOGLE_SCRIPT_URL
#define GOOGLE_SCRIPT_URL ""
#endif

static constexpr uint8_t DATA_PIN_UPPER = 25;
static constexpr uint8_t DATA_PIN_LOWER = 26;
static constexpr uint8_t JOYSTICK_BUTTON_PIN = 32;
static constexpr uint8_t JOYSTICK_X_PIN = 34;
static constexpr uint8_t JOYSTICK_Y_PIN = 35;

static constexpr uint8_t MATRIX_WIDTH = 32;
static constexpr uint8_t MATRIX_HEIGHT = 16;
static constexpr uint16_t NUM_LEDS_PER_PANEL = 256;
static constexpr uint8_t BRIGHTNESS = 50;

static constexpr uint32_t RENDER_INTERVAL_MS = 33;
static constexpr uint32_t INPUT_INTERVAL_MS = 20;
static constexpr uint32_t UI_INTERVAL_MS = 25;
static constexpr uint32_t WEATHER_INTERVAL_MS = 60000;
static constexpr uint32_t TIME_SYNC_INTERVAL_MS = 1800000;
static constexpr uint32_t SCROLL_STEP_INTERVAL_MS = 70;
static constexpr uint32_t LONG_PRESS_MS = 900;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;

static constexpr int JOYSTICK_CENTER = 2048;
static constexpr int JOYSTICK_DEADZONE = 700;
static constexpr int JOYSTICK_NAV_HIGH = JOYSTICK_CENTER + 1000;
static constexpr int JOYSTICK_NAV_LOW = JOYSTICK_CENTER - 1000;

enum class AppMode : uint8_t {
  Menu = 0,
  Clock,
  Weather,
  Snake,
  Scroll,
  Input
};

enum class Direction : uint8_t {
  Up = 0,
  Right,
  Down,
  Left
};

struct LedAddress {
  CRGB* array;
  int index;
};

struct WeatherData {
  float temperature;
  int humidity;
  float windSpeed;
  bool valid;
  bool configured;
  bool wifiConnected;
  bool syncValid;
  uint32_t lastFetchMs;
  char status[24];
};

struct SnakeState {
  int x[128];
  int y[128];
  int length;
  Direction direction;
  int foodX;
  int foodY;
  uint32_t lastMoveMs;
  uint16_t speedMs;
  bool gameOver;
  int score;
};

struct Glyph {
  char value;
  uint8_t rows[7];
};

static const Glyph FONT_5X7[] = {
  {'0', {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}},
  {'1', {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
  {'2', {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}},
  {'3', {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}},
  {'4', {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},
  {'5', {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110}},
  {'6', {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}},
  {'7', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}},
  {'8', {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}},
  {'9', {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b11100}},
  {'A', {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
  {'B', {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}},
  {'C', {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111}},
  {'D', {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110}},
  {'E', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}},
  {'F', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}},
  {'G', {0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111}},
  {'H', {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
  {'I', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111}},
  {'J', {0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100}},
  {'K', {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}},
  {'L', {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}},
  {'M', {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}},
  {'N', {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001}},
  {'O', {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
  {'P', {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}},
  {'Q', {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}},
  {'R', {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}},
  {'S', {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}},
  {'T', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
  {'U', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
  {'V', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}},
  {'W', {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010}},
  {'X', {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}},
  {'Y', {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}},
  {'Z', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}},
  {':', {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000}},
  {'-', {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000}},
  {'.', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100}},
  {'/', {0b00001, 0b00010, 0b00100, 0b00100, 0b01000, 0b10000, 0b00000}},
  {'%', {0b11001, 0b11010, 0b00100, 0b01000, 0b10110, 0b00110, 0b00000}},
  {'*', {0b00100, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00100}},
  {'|', {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
  {' ', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}}
};

static CRGB ledsUpper[NUM_LEDS_PER_PANEL];
static CRGB ledsLower[NUM_LEDS_PER_PANEL];
static portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE snakeMux = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t wifiMutex = nullptr;

static volatile AppMode currentMode = AppMode::Menu;
static volatile int selectedMenuIndex = 0;
static volatile int joystickX = JOYSTICK_CENTER;
static volatile int joystickY = JOYSTICK_CENTER;
static volatile bool buttonDown = false;
static volatile int navEvent = 0;
static volatile bool shortPressEvent = false;
static volatile bool longPressEvent = false;
static volatile int scrollOffset = MATRIX_WIDTH;
static volatile bool timeSynced = false;
static volatile bool wifiConnected = false;

static WeatherData weatherData = {0.0f, 0, 0.0f, false, false, false, false, 0, "BOOT"};
static SnakeState snakeState = {};

static const char* MENU_ITEMS[] = {"TIME", "WTHR", "SNAKE", "TEXT", "INPUT"};
static constexpr int MENU_ITEM_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
static const char* SCROLL_MESSAGE = " PIXELBOARD | TIME | WEATHER | SNAKE | INPUT ";

static const char* appModeToString(AppMode mode) {
  switch (mode) {
    case AppMode::Menu:
      return "Menu";
    case AppMode::Clock:
      return "Clock";
    case AppMode::Weather:
      return "Weather";
    case AppMode::Snake:
      return "Snake";
    case AppMode::Scroll:
      return "Scroll";
    case AppMode::Input:
      return "Input";
  }

  return "Unknown";
}

static const char* wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_DONE";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

static inline bool hasWiFiCredentials() {
  return strlen(WIFI_SSID) > 0 && strlen(WIFI_PASSWORD) > 0;
}

static inline bool hasWeatherConfig() {
  return hasWiFiCredentials() && strlen(WEATHER_API_KEY) > 0;
}

static inline bool hasGoogleSheetsConfig() {
  return strlen(GOOGLE_SCRIPT_URL) > 10 &&
         strcmp(GOOGLE_SCRIPT_URL, "YOUR_GOOGLE_SCRIPT_URL_HERE") != 0;
}

static void logDataToGoogleSheets(float temp, int hum, float wind) {
  if (!hasGoogleSheetsConfig()) {
    Serial.println(F("[gsheets] Skipping log: URL not configured"));
    return;
  }

  Serial.println(F("[gsheets] Logging weather row"));
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  const String url =
    String(GOOGLE_SCRIPT_URL) +
    "?temp=" + String(temp, 1) +
    "&hum=" + String(hum) +
    "&wind=" + String(wind, 1);

  if (!http.begin(client, url)) {
    Serial.println(F("[gsheets] Error: begin failed"));
    return;
  }

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  const int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[gsheets] Success, HTTP %d\n", httpCode);
  } else {
    Serial.printf("[gsheets] Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

static LedAddress mapXY(int x, int y) {
  LedAddress result;
  int led;

  if (y < 8) {
    led = x * 8 + ((x % 2 == 0) ? y : 7 - y);
    result.array = ledsLower;
    result.index = led;
  } else {
    const int flippedX = MATRIX_WIDTH - 1 - x;
    const int flippedY = MATRIX_HEIGHT - 1 - y;
    led = flippedX * 8 + ((flippedX % 2 == 0) ? flippedY : 7 - flippedY);
    result.array = ledsUpper;
    result.index = led;
  }

  return result;
}

static void clearAll() {
  for (int i = 0; i < NUM_LEDS_PER_PANEL; ++i) {
    ledsUpper[i] = CRGB::Black;
    ledsLower[i] = CRGB::Black;
  }
}

static void setPixel(int x, int y, const CRGB& color) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) {
    return;
  }

  const LedAddress addr = mapXY(MATRIX_WIDTH - 1 - x, MATRIX_HEIGHT - 1 - y);
  addr.array[addr.index] = color;
}

static void fillRect(int x, int y, int w, int h, const CRGB& color) {
  for (int yy = y; yy < y + h; ++yy) {
    for (int xx = x; xx < x + w; ++xx) {
      setPixel(xx, yy, color);
    }
  }
}

static const Glyph* findGlyph(char c) {
  if (c >= 'a' && c <= 'z') {
    c = static_cast<char>(c - 32);
  }

  for (size_t i = 0; i < sizeof(FONT_5X7) / sizeof(FONT_5X7[0]); ++i) {
    if (FONT_5X7[i].value == c) {
      return &FONT_5X7[i];
    }
  }

  return &FONT_5X7[sizeof(FONT_5X7) / sizeof(FONT_5X7[0]) - 1];
}

static int textWidth(const char* text) {
  int width = 0;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    width += (text[i] == ' ') ? 3 : 6;
  }
  if (width > 0) {
    width -= 1;
  }
  return width;
}

static void drawChar(char c, int x, int y, const CRGB& color) {
  const Glyph* glyph = findGlyph(c);
  for (int row = 0; row < 7; ++row) {
    for (int col = 0; col < 5; ++col) {
      if ((glyph->rows[row] >> (4 - col)) & 0x01) {
        setPixel(x + col, y + row, color);
      }
    }
  }
}

static void drawText(const char* text, int x, int y, const CRGB& color) {
  int cursor = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    if (text[i] == ' ') {
      cursor += 3;
      continue;
    }
    drawChar(text[i], cursor, y, color);
    cursor += 6;
  }
}

static void drawTextCentered(const char* text, int y, const CRGB& color) {
  const int startX = (MATRIX_WIDTH - textWidth(text)) / 2;
  drawText(text, startX, y, color);
}

static void drawScrollingText(const char* text, int x, int y, const CRGB& color) {
  drawText(text, x, y, color);
}

static void drawAccentBar(uint8_t hueBase) {
  const uint8_t pulse = beatsin8(18, 60, 160);
  for (int x = 0; x < MATRIX_WIDTH; ++x) {
    setPixel(x, 0, CHSV(hueBase + x * 3, 220, pulse));
  }
}

static void drawChevron(int x, int y, bool left, const CRGB& color) {
  if (left) {
    setPixel(x + 2, y + 0, color);
    setPixel(x + 1, y + 1, color);
    setPixel(x + 0, y + 2, color);
    setPixel(x + 1, y + 3, color);
    setPixel(x + 2, y + 4, color);
  } else {
    setPixel(x + 0, y + 0, color);
    setPixel(x + 1, y + 1, color);
    setPixel(x + 2, y + 2, color);
    setPixel(x + 1, y + 3, color);
    setPixel(x + 0, y + 4, color);
  }
}

static void drawClockIcon(int x, int y, const CRGB& color) {
  for (int dx = 1; dx < 5; ++dx) {
    setPixel(x + dx, y + 0, color);
    setPixel(x + dx, y + 5, color);
  }
  for (int dy = 1; dy < 5; ++dy) {
    setPixel(x + 0, y + dy, color);
    setPixel(x + 5, y + dy, color);
  }
  setPixel(x + 3, y + 2, color);
  setPixel(x + 3, y + 3, color);
  setPixel(x + 4, y + 3, color);
}

static void drawWeatherIcon(int x, int y, const CRGB& color) {
  fillRect(x + 0, y + 3, 6, 2, color);
  fillRect(x + 1, y + 2, 4, 3, color);
  setPixel(x + 1, y + 1, color);
  setPixel(x + 4, y + 1, color);
  setPixel(x + 2, y + 0, color);
  setPixel(x + 3, y + 0, color);
}

static void drawSnakeIcon(int x, int y, const CRGB& color) {
  fillRect(x + 0, y + 3, 2, 2, color);
  fillRect(x + 2, y + 3, 2, 2, color);
  fillRect(x + 4, y + 2, 2, 2, color);
  setPixel(x + 5, y + 1, CRGB::Red);
}

static void drawScrollIcon(int x, int y, const CRGB& color) {
  drawChevron(x + 0, y + 1, true, color);
  drawChevron(x + 5, y + 1, false, color);
  setPixel(x + 3, y + 3, color);
  setPixel(x + 4, y + 3, color);
}

static void drawInputIcon(int x, int y, const CRGB& color) {
  fillRect(x + 2, y + 0, 2, 6, color);
  fillRect(x + 0, y + 2, 6, 2, color);
  setPixel(x + 6, y + 2, color);
  setPixel(x + 6, y + 3, color);
  setPixel(x + 7, y + 2, color);
  setPixel(x + 7, y + 3, color);
}

static void drawMenuPreview(int index) {
  const uint8_t hue = static_cast<uint8_t>(index * 34);
  const CRGB accent = CHSV(hue, 220, 255);

  switch (index) {
    case 0:
      drawClockIcon(3, 9, accent);
      break;
    case 1:
      drawWeatherIcon(3, 9, accent);
      break;
    case 2:
      drawSnakeIcon(3, 9, accent);
      break;
    case 3:
      drawScrollIcon(2, 9, accent);
      break;
    default:
      drawInputIcon(2, 9, accent);
      break;
  }
}

static void drawMenu() {
  const int index = selectedMenuIndex;
  const uint8_t hue = static_cast<uint8_t>(index * 34);

  drawAccentBar(hue);
  drawTextCentered(MENU_ITEMS[index], 2, CHSV(hue, 220, 255));
  drawMenuPreview(index);

  drawChevron(24, 9, true, CRGB(80, 80, 80));
  drawChevron(28, 9, false, CRGB(80, 80, 80));

  for (int i = 0; i < MENU_ITEM_COUNT; ++i) {
    const int px = 6 + i * 5;
    const CRGB dotColor = (i == index) ? CHSV(hue, 220, 255) : CRGB(30, 30, 40);
    fillRect(px, 15, 3, 1, dotColor);
  }
}

static void drawClockApp() {
  drawAccentBar(0);

  if (!timeSynced) {
    drawTextCentered("WAIT", 4, CRGB(120, 120, 120));
    return;
  }

  time_t now = time(nullptr);
  struct tm timeInfo;
  localtime_r(&now, &timeInfo);

  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
  drawTextCentered(buffer, 4, CRGB::Red);
}

static void drawWeatherApp() {
  WeatherData localWeather;
  portENTER_CRITICAL(&stateMux);
  localWeather = weatherData;
  portEXIT_CRITICAL(&stateMux);

  drawAccentBar(96);

  if (!localWeather.configured) {
    drawTextCentered("SETUP", 4, CRGB(150, 80, 0));
    return;
  }

  if (!localWeather.valid) {
    drawTextCentered(localWeather.status, 4, CRGB(120, 120, 120));
    return;
  }

  char label[8];
  char value[8];
  const uint8_t page = (millis() / 3000UL) % 3;

  if (page == 0) {
    strcpy(label, "TEMP");
    snprintf(value, sizeof(value), "%d*", static_cast<int>(roundf(localWeather.temperature)));
  } else if (page == 1) {
    strcpy(label, "HUM");
    snprintf(value, sizeof(value), "%d%%", localWeather.humidity);
  } else {
    strcpy(label, "WIND");
    snprintf(value, sizeof(value), "%.1f", localWeather.windSpeed);
  }

  drawTextCentered(label, 1, CRGB::Yellow);
  drawTextCentered(value, 9, CRGB::White);
}

static void drawScrollApp() {
  drawAccentBar(180);
  const int localOffset = scrollOffset;
  drawScrollingText(SCROLL_MESSAGE, localOffset, 4, CRGB::Magenta);
}

static void drawInputApp() {
  int localX;
  int localY;
  bool localButtonDown;

  portENTER_CRITICAL(&stateMux);
  localX = joystickX;
  localY = joystickY;
  localButtonDown = buttonDown;
  portEXIT_CRITICAL(&stateMux);

  drawAccentBar(120);
  fillRect(7, 6, 18, 8, CRGB(8, 10, 18));

  for (int x = 7; x < 25; ++x) {
    setPixel(x, 6, CRGB(25, 35, 60));
    setPixel(x, 13, CRGB(25, 35, 60));
  }
  for (int y = 6; y < 14; ++y) {
    setPixel(7, y, CRGB(25, 35, 60));
    setPixel(24, y, CRGB(25, 35, 60));
  }

  setPixel(16, 9, CRGB(60, 60, 80));
  setPixel(15, 9, CRGB(60, 60, 80));
  setPixel(17, 9, CRGB(60, 60, 80));
  setPixel(16, 8, CRGB(60, 60, 80));
  setPixel(16, 10, CRGB(60, 60, 80));

  const int mappedX = map(localX, 0, 4095, 8, 23);
  const int mappedY = map(localY, 0, 4095, 12, 7);
  fillRect(mappedX - 1, mappedY - 1, 3, 3, CRGB::Green);
  fillRect(27, 8, 3, 3, localButtonDown ? CRGB::Red : CRGB(20, 20, 20));
}

static void drawSnakeGameOver() {
  fillRect(5, 5, 22, 7, CRGB(0, 0, 0));
  drawTextCentered("LOSE", 4, CRGB::Red);
}

static void drawSnakeApp() {
  portENTER_CRITICAL(&snakeMux);
  for (int i = 0; i < snakeState.length; ++i) {
    if (snakeState.x[i] >= 0 && snakeState.x[i] < MATRIX_WIDTH &&
        snakeState.y[i] >= 0 && snakeState.y[i] < MATRIX_HEIGHT) {
      const CRGB segmentColor = (i == 0) ? CRGB::White : CRGB(0, 160, 0);
      setPixel(snakeState.x[i], snakeState.y[i], segmentColor);
    }
  }

  const uint8_t foodPulse = beatsin8(24, 100, 255);
  setPixel(snakeState.foodX, snakeState.foodY, CRGB(foodPulse, 0, 0));
  const bool gameOver = snakeState.gameOver;
  portEXIT_CRITICAL(&snakeMux);

  if (gameOver) {
    drawSnakeGameOver();
  }
}

static void renderFrame() {
  clearAll();

  switch (currentMode) {
    case AppMode::Menu:
      drawMenu();
      break;
    case AppMode::Clock:
      drawClockApp();
      break;
    case AppMode::Weather:
      drawWeatherApp();
      break;
    case AppMode::Snake:
      drawSnakeApp();
      break;
    case AppMode::Scroll:
      drawScrollApp();
      break;
    case AppMode::Input:
      drawInputApp();
      break;
  }

  FastLED.show();
}

static void snakeGenerateFoodLocked() {
  bool valid = false;
  while (!valid) {
    valid = true;
    snakeState.foodX = random(MATRIX_WIDTH);
    snakeState.foodY = random(MATRIX_HEIGHT);

    for (int i = 0; i < snakeState.length; ++i) {
      if (snakeState.x[i] == snakeState.foodX && snakeState.y[i] == snakeState.foodY) {
        valid = false;
        break;
      }
    }
  }
}

static void resetSnakeGame() {
  portENTER_CRITICAL(&snakeMux);
  snakeState.length = 3;
  snakeState.direction = Direction::Right;
  snakeState.speedMs = 300;
  snakeState.gameOver = false;
  snakeState.score = 0;
  snakeState.lastMoveMs = millis();

  for (int i = 0; i < snakeState.length; ++i) {
    snakeState.x[i] = (MATRIX_WIDTH / 2) - i;
    snakeState.y[i] = MATRIX_HEIGHT / 2;
  }

  snakeGenerateFoodLocked();
  portEXIT_CRITICAL(&snakeMux);
}

static bool ensureWiFiConnected() {
  if (!hasWiFiCredentials()) {
    Serial.println(F("[wifi] Missing credentials, cannot connect"));
    portENTER_CRITICAL(&stateMux);
    wifiConnected = false;
    weatherData.wifiConnected = false;
    weatherData.syncValid = false;
    snprintf(weatherData.status, sizeof(weatherData.status), "WIFI");
    portEXIT_CRITICAL(&stateMux);
    return false;
  }

  xSemaphoreTake(wifiMutex, portMAX_DELAY);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] Already connected: %s\n", WiFi.localIP().toString().c_str());
    xSemaphoreGive(wifiMutex);
    portENTER_CRITICAL(&stateMux);
    wifiConnected = true;
    weatherData.wifiConnected = true;
    portEXIT_CRITICAL(&stateMux);
    return true;
  }

  Serial.printf("[wifi] Connecting to SSID '%s'\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(150);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    if (tries == 10 || tries == 20 || tries == 30) {
      Serial.printf("[wifi] Retry %u, status=%s\n", tries, wifiStatusToString(WiFi.status()));
    }
    if (tries == 20) {
      Serial.println(F("[wifi] Restarting association"));
      WiFi.disconnect(true);
      delay(200);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    delay(250);
    ++tries;
  }

  const bool connected = WiFi.status() == WL_CONNECTED;
  xSemaphoreGive(wifiMutex);

  if (connected) {
    Serial.printf("[wifi] Connected, IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
  } else {
    Serial.printf("[wifi] Failed, final status=%s\n", wifiStatusToString(WiFi.status()));
  }

  portENTER_CRITICAL(&stateMux);
  wifiConnected = connected;
  weatherData.wifiConnected = connected;
  if (!connected) {
    snprintf(weatherData.status, sizeof(weatherData.status), "LINK");
  }
  portEXIT_CRITICAL(&stateMux);

  return connected;
}

static void syncTimeIfPossible() {
  if (!ensureWiFiConnected()) {
    Serial.println(F("[time] Skipping NTP sync because WiFi is unavailable"));
    portENTER_CRITICAL(&stateMux);
    timeSynced = false;
    weatherData.syncValid = false;
    portEXIT_CRITICAL(&stateMux);
    return;
  }

  Serial.println(F("[time] Starting NTP sync"));
  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  const uint32_t startMs = millis();
  while (now < 100000 && (millis() - startMs) < 10000) {
    delay(200);
    now = time(nullptr);
  }

  const bool synced = now > 100000;
  if (synced) {
    struct tm timeInfo;
    localtime_r(&now, &timeInfo);
    Serial.printf("[time] Synced: %02d:%02d\n", timeInfo.tm_hour, timeInfo.tm_min);
  } else {
    Serial.println(F("[time] NTP sync failed"));
  }
  portENTER_CRITICAL(&stateMux);
  timeSynced = synced;
  weatherData.syncValid = synced;
  if (synced && !weatherData.valid) {
    snprintf(weatherData.status, sizeof(weatherData.status), "SYNCED");
  }
  portEXIT_CRITICAL(&stateMux);
}

static void fetchWeather() {
  WeatherData next = weatherData;
  next.configured = hasWeatherConfig();
  next.wifiConnected = wifiConnected;
  next.syncValid = timeSynced;

  if (!hasWeatherConfig()) {
    Serial.println(F("[weather] Missing API configuration"));
    next.valid = false;
    snprintf(next.status, sizeof(next.status), "SETUP");
    portENTER_CRITICAL(&stateMux);
    weatherData = next;
    portEXIT_CRITICAL(&stateMux);
    return;
  }

  if (!ensureWiFiConnected()) {
    Serial.println(F("[weather] Skipping fetch because WiFi is unavailable"));
    next.valid = false;
    snprintf(next.status, sizeof(next.status), "WIFI");
    portENTER_CRITICAL(&stateMux);
    weatherData = next;
    portEXIT_CRITICAL(&stateMux);
    return;
  }

  const String url =
    String("http://api.openweathermap.org/data/2.5/weather?q=") +
    WEATHER_CITY +
    "&units=metric&appid=" +
    WEATHER_API_KEY;

  Serial.printf("[weather] GET %s\n", url.c_str());

  HTTPClient http;
  http.setTimeout(8000);

  if (!http.begin(url)) {
    Serial.println(F("[weather] HTTP begin failed"));
    next.valid = false;
    snprintf(next.status, sizeof(next.status), "HTTP");
    portENTER_CRITICAL(&stateMux);
    weatherData = next;
    portEXIT_CRITICAL(&stateMux);
    return;
  }

  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[weather] HTTP error %d\n", httpCode);
    next.valid = false;
    snprintf(next.status, sizeof(next.status), "HTTP");
    http.end();
    portENTER_CRITICAL(&stateMux);
    weatherData = next;
    portEXIT_CRITICAL(&stateMux);
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();

  if (err) {
    Serial.printf("[weather] JSON error: %s\n", err.c_str());
    next.valid = false;
    snprintf(next.status, sizeof(next.status), "JSON");
    portENTER_CRITICAL(&stateMux);
    weatherData = next;
    portEXIT_CRITICAL(&stateMux);
    return;
  }

  next.temperature = doc["main"]["temp"] | 0.0f;
  next.humidity = doc["main"]["humidity"] | 0;
  next.windSpeed = doc["wind"]["speed"] | 0.0f;
  next.valid = true;
  next.lastFetchMs = millis();
  snprintf(next.status, sizeof(next.status), "OK");
  Serial.printf("[weather] Updated: %.1f C, %d %%, %.1f m/s\n",
                next.temperature,
                next.humidity,
                next.windSpeed);
  logDataToGoogleSheets(next.temperature, next.humidity, next.windSpeed);

  portENTER_CRITICAL(&stateMux);
  weatherData = next;
  portEXIT_CRITICAL(&stateMux);
}

static void consumeUiEvents(int& nav, bool& shortPress, bool& longPress) {
  portENTER_CRITICAL(&stateMux);
  nav = navEvent;
  shortPress = shortPressEvent;
  longPress = longPressEvent;
  navEvent = 0;
  shortPressEvent = false;
  longPressEvent = false;
  portEXIT_CRITICAL(&stateMux);
}

static void inputTask(void*) {
  bool rawButtonLast = true;
  bool stableButton = true;
  uint32_t lastDebounceMs = 0;
  uint32_t pressStartMs = 0;
  bool longPressTriggered = false;
  int lastNavDirection = 0;
  uint32_t lastNavMs = 0;

  while (true) {
    const int x = analogRead(JOYSTICK_X_PIN);
    const int y = analogRead(JOYSTICK_Y_PIN);
    const bool rawButton = digitalRead(JOYSTICK_BUTTON_PIN);
    const uint32_t now = millis();

    if (rawButton != rawButtonLast) {
      rawButtonLast = rawButton;
      lastDebounceMs = now;
    }

    if ((now - lastDebounceMs) >= BUTTON_DEBOUNCE_MS && rawButton != stableButton) {
      stableButton = rawButton;
      if (!stableButton) {
        pressStartMs = now;
        longPressTriggered = false;
      } else {
        if (!longPressTriggered) {
          portENTER_CRITICAL(&stateMux);
          shortPressEvent = true;
          portEXIT_CRITICAL(&stateMux);
        }
      }
    }

    if (!stableButton && !longPressTriggered && (now - pressStartMs) >= LONG_PRESS_MS) {
      longPressTriggered = true;
      portENTER_CRITICAL(&stateMux);
      longPressEvent = true;
      portEXIT_CRITICAL(&stateMux);
    }

    int navDirection = 0;
    if (y < JOYSTICK_NAV_LOW) {
      navDirection = -1;
    } else if (y > JOYSTICK_NAV_HIGH) {
      navDirection = 1;
    }

    if (navDirection != 0 && (navDirection != lastNavDirection || (now - lastNavMs) >= 220)) {
      portENTER_CRITICAL(&stateMux);
      navEvent = navDirection;
      portEXIT_CRITICAL(&stateMux);
      lastNavMs = now;
    }
    lastNavDirection = navDirection;

    portENTER_CRITICAL(&stateMux);
    joystickX = x;
    joystickY = y;
    buttonDown = !stableButton;
    portEXIT_CRITICAL(&stateMux);

    vTaskDelay(pdMS_TO_TICKS(INPUT_INTERVAL_MS));
  }
}

static void uiTask(void*) {
  while (true) {
    int nav = 0;
    bool shortPress = false;
    bool longPress = false;
    consumeUiEvents(nav, shortPress, longPress);

    if (longPress && currentMode != AppMode::Menu) {
      Serial.printf("[ui] Return to menu from %s\n", appModeToString(currentMode));
      currentMode = AppMode::Menu;
    }

    if (currentMode == AppMode::Menu) {
      if (nav != 0) {
        int next = selectedMenuIndex + nav;
        if (next < 0) {
          next = MENU_ITEM_COUNT - 1;
        } else if (next >= MENU_ITEM_COUNT) {
          next = 0;
        }
        selectedMenuIndex = next;
      }

      if (shortPress) {
        currentMode = static_cast<AppMode>(selectedMenuIndex + 1);
        Serial.printf("[ui] Enter app: %s\n", appModeToString(currentMode));
        if (currentMode == AppMode::Snake) {
          resetSnakeGame();
        }
      }
    } else if (currentMode == AppMode::Snake && shortPress) {
      portENTER_CRITICAL(&snakeMux);
      const bool needsReset = snakeState.gameOver;
      portEXIT_CRITICAL(&snakeMux);
      if (needsReset) {
        resetSnakeGame();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(UI_INTERVAL_MS));
  }
}

static void weatherTask(void*) {
  syncTimeIfPossible();
  fetchWeather();

  uint32_t lastSyncMs = millis();
  while (true) {
    const uint32_t now = millis();
    if (now - lastSyncMs >= TIME_SYNC_INTERVAL_MS) {
      syncTimeIfPossible();
      lastSyncMs = now;
    }

    fetchWeather();
    vTaskDelay(pdMS_TO_TICKS(WEATHER_INTERVAL_MS));
  }
}

static void scrollTask(void*) {
  const int resetWidth = textWidth(SCROLL_MESSAGE);
  while (true) {
    int nextOffset = scrollOffset - 1;
    if (nextOffset < -resetWidth) {
      nextOffset = MATRIX_WIDTH;
    }

    portENTER_CRITICAL(&stateMux);
    scrollOffset = nextOffset;
    portEXIT_CRITICAL(&stateMux);

    vTaskDelay(pdMS_TO_TICKS(SCROLL_STEP_INTERVAL_MS));
  }
}

static void snakeTask(void*) {
  while (true) {
    if (currentMode != AppMode::Snake) {
      vTaskDelay(pdMS_TO_TICKS(60));
      continue;
    }

    int localX;
    int localY;
    portENTER_CRITICAL(&stateMux);
    localX = joystickX;
    localY = joystickY;
    portEXIT_CRITICAL(&stateMux);

    portENTER_CRITICAL(&snakeMux);
    if (localY < JOYSTICK_CENTER - JOYSTICK_DEADZONE && snakeState.direction != Direction::Down) {
      snakeState.direction = Direction::Up;
    } else if (localY > JOYSTICK_CENTER + JOYSTICK_DEADZONE && snakeState.direction != Direction::Up) {
      snakeState.direction = Direction::Down;
    } else if (localX < JOYSTICK_CENTER - JOYSTICK_DEADZONE && snakeState.direction != Direction::Right) {
      snakeState.direction = Direction::Left;
    } else if (localX > JOYSTICK_CENTER + JOYSTICK_DEADZONE && snakeState.direction != Direction::Left) {
      snakeState.direction = Direction::Right;
    }

    const uint32_t now = millis();
    if (!snakeState.gameOver && (now - snakeState.lastMoveMs) >= snakeState.speedMs) {
      snakeState.lastMoveMs = now;

      for (int i = snakeState.length - 1; i > 0; --i) {
        snakeState.x[i] = snakeState.x[i - 1];
        snakeState.y[i] = snakeState.y[i - 1];
      }

      switch (snakeState.direction) {
        case Direction::Up:
          --snakeState.y[0];
          break;
        case Direction::Right:
          ++snakeState.x[0];
          break;
        case Direction::Down:
          ++snakeState.y[0];
          break;
        case Direction::Left:
          --snakeState.x[0];
          break;
      }

      if (snakeState.x[0] < 0 || snakeState.x[0] >= MATRIX_WIDTH ||
          snakeState.y[0] < 0 || snakeState.y[0] >= MATRIX_HEIGHT) {
        snakeState.gameOver = true;
      }

      for (int i = 1; i < snakeState.length && !snakeState.gameOver; ++i) {
        if (snakeState.x[i] == snakeState.x[0] && snakeState.y[i] == snakeState.y[0]) {
          snakeState.gameOver = true;
        }
      }

      if (!snakeState.gameOver &&
          snakeState.x[0] == snakeState.foodX &&
          snakeState.y[0] == snakeState.foodY) {
        if (snakeState.length < 127) {
          snakeState.x[snakeState.length] = snakeState.x[snakeState.length - 1];
          snakeState.y[snakeState.length] = snakeState.y[snakeState.length - 1];
          ++snakeState.length;
        }
        ++snakeState.score;
        if (snakeState.speedMs > 110) {
          snakeState.speedMs -= 10;
        }
        snakeGenerateFoodLocked();
      }
    }
    portEXIT_CRITICAL(&snakeMux);

    vTaskDelay(pdMS_TO_TICKS(35));
  }
}

static void renderTask(void*) {
  while (true) {
    renderFrame();
    vTaskDelay(pdMS_TO_TICKS(RENDER_INTERVAL_MS));
  }
}

void setup() {
  Serial.begin(115200);
  delay(150);

  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);

  FastLED.addLeds<WS2812B, DATA_PIN_UPPER, GRB>(ledsUpper, NUM_LEDS_PER_PANEL);
  FastLED.addLeds<WS2812B, DATA_PIN_LOWER, GRB>(ledsLower, NUM_LEDS_PER_PANEL);
  FastLED.setBrightness(BRIGHTNESS);

  clearAll();
  FastLED.show();

  randomSeed(static_cast<uint32_t>(micros()));
  wifiMutex = xSemaphoreCreateMutex();
  resetSnakeGame();

  portENTER_CRITICAL(&stateMux);
  weatherData.configured = hasWeatherConfig();
  snprintf(weatherData.status, sizeof(weatherData.status), hasWeatherConfig() ? "BOOT" : "SETUP");
  portEXIT_CRITICAL(&stateMux);

  xTaskCreatePinnedToCore(inputTask, "InputTask", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(uiTask, "UiTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(weatherTask, "WeatherTask", 8192, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(scrollTask, "ScrollTask", 3072, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(snakeTask, "SnakeTask", 4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(renderTask, "RenderTask", 6144, nullptr, 2, nullptr, 1);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("   PixelBoard Unified FreeRTOS Build"));
  Serial.println(F("========================================"));
  Serial.println(F("Menu: joystick up/down + short press"));
  Serial.println(F("Apps: long press returns to menu"));
  Serial.printf("[boot] WiFi SSID: %s\n", hasWiFiCredentials() ? WIFI_SSID : "<missing>");
  Serial.printf("[boot] Weather city: %s\n", WEATHER_CITY);
  Serial.printf("[boot] Weather API configured: %s\n", hasWeatherConfig() ? "yes" : "no");
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
