#pragma once

#include "MenuItem.h"
#include "DisplayManager.h"
#include <WiFi.h>
#include <time.h>
#include <LEDText.h>
#include <FontMatrise.h>

class ClockApp : public BaseApp {
private:
  DisplayManager* display;
  const char* wifiSsid;
  const char* wifiPassword;
  const long gmtOffsetSec = 3600;
  const int daylightOffsetSec = 0;
  const char* ntpServer1 = "pool.ntp.org";
  const char* ntpServer2 = "time.nist.gov";
  cLEDText scrollingText;
  CRGB canvas8Leds[64 * 8];
  cLEDMatrix<64, 8, HORIZONTAL_MATRIX>* canvas8;
  unsigned long lastUpdate;
  unsigned long lastTimeUpdate;
  int lastMinute;
  char timeBuffer[64];
  bool wifiConnected;
  bool ntpSynced;

  void connectToWifi();
  void initTimeNtp();
  void updateTimeText();

public:
  ClockApp(const char* ssid, const char* password);
  ~ClockApp();
  void init() override;
  void update() override;
  void render() override;
  void cleanup() override;
  const char* getName() const override { return "Clock"; }
};
