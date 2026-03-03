#include "ClockApp.h"

ClockApp::ClockApp(const char* ssid, const char* password)
  : wifiSsid(ssid), wifiPassword(password),
    display(DisplayManager::getInstance()),
    lastUpdate(0), lastTimeUpdate(0), lastMinute(-1),
    wifiConnected(false), ntpSynced(false) {
  canvas8 = new cLEDMatrix<64, 8, HORIZONTAL_MATRIX>();
  canvas8->SetLEDArray(canvas8Leds);
  strcpy(timeBuffer, "   --:--   ");
}

ClockApp::~ClockApp() {
  delete canvas8;
}

void ClockApp::init() {
  Serial.println(F("ClockApp init"));
  display->clear();
  scrollingText.SetFont(MatriseFontData);
  scrollingText.Init(canvas8, 64, 8, 0, 0);
  scrollingText.SetScrollDirection(SCROLL_LEFT);
  scrollingText.SetFrameRate(1); // Sehr niedrige Framerate = quasi statisch
  scrollingText.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 255, 0, 0); // ROT für die Zeit
  strcpy(timeBuffer, " --:-- ");
  scrollingText.SetText((unsigned char*)timeBuffer, strlen(timeBuffer));

  Serial.println(F("Verbinde mit WLAN..."));
  connectToWifi();
  if (wifiConnected) {
    Serial.println(F("Synchronisiere Zeit..."));
    initTimeNtp();
  }

  lastUpdate = millis();
  lastTimeUpdate = millis();

  Serial.println(F("ClockApp bereit"));
}

void ClockApp::connectToWifi() {
  Serial.print(F("Verbinde mit WLAN: "));
  Serial.println(wifiSsid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(250);
    Serial.print(".");
    tries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.print(F("WLAN verbunden, IP: "));
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println(F("WLAN-Verbindung fehlgeschlagen"));
  }
}

void ClockApp::initTimeNtp() {
  if (!wifiConnected) {
    Serial.println(F("Kein WLAN, NTP übersprungen"));
    return;
  }
  Serial.println(F("Initialisiere NTP..."));
  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer1, ntpServer2);
  time_t now = time(nullptr);
  uint32_t startMs = millis();
  while (now < 100000 && (millis() - startMs) < 5000) {
    delay(100);
    now = time(nullptr);
  }
  if (now < 100000) {
    ntpSynced = false;
    Serial.println(F("NTP-Sync fehlgeschlagen"));
  } else {
    ntpSynced = true;
    struct tm timeInfo;
    localtime_r(&now, &timeInfo);
    Serial.print(F("Zeit synchronisiert: "));
    Serial.print(timeInfo.tm_hour);
    Serial.print(F(":"));
    Serial.println(timeInfo.tm_min);
  }
}

void ClockApp::update() {
  unsigned long now = millis();
  if (now - lastTimeUpdate >= 1000) {
    lastTimeUpdate = now;
    updateTimeText();
  }
  if (now - lastUpdate >= 30) {
    lastUpdate = now;
    if (scrollingText.UpdateText() == -1) {
      scrollingText.SetText((unsigned char*)timeBuffer, strlen(timeBuffer));
    }
  }
}

void ClockApp::updateTimeText() {
  time_t now = time(nullptr);
  struct tm timeInfo;
  localtime_r(&now, &timeInfo);
  int currentMinute = timeInfo.tm_min;
  if (currentMinute != lastMinute) {
    lastMinute = currentMinute;
    if (ntpSynced && now > 100000) {
      // Statische Zeit-Anzeige: " HH:MM " (mit Leerzeichen für Zentrierung)
      snprintf(timeBuffer, sizeof(timeBuffer),
               " %02d:%02d ",
               timeInfo.tm_hour, timeInfo.tm_min);
    } else {
      strcpy(timeBuffer, " --:-- ");
    }
    scrollingText.SetText((unsigned char*)timeBuffer, strlen(timeBuffer));
    Serial.print(F("Uhrzeit aktualisiert: "));
    Serial.println(timeBuffer);
  }
}

void ClockApp::render() {
  // Zeit in der Mitte rendern (Zeilen 4-11, also 8 Zeilen)
  const int TIME_Y_OFFSET = 4; // Start bei Zeile 4

  for (uint8_t y8 = 0; y8 < 8; y8++) {
    for (uint8_t x = 0; x < 64 && x < display->getWidth(); x++) {
      CRGB color = (*canvas8)(x, y8);
      uint8_t displayY = TIME_Y_OFFSET + y8;
      if (displayY < display->getHeight()) {
        display->setPixel(x, displayY, color);
      }
    }
  }
}

void ClockApp::cleanup() {
  Serial.println(F("ClockApp cleanup"));
  display->clear();
}
