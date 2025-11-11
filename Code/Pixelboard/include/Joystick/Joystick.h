#pragma once

#include <Arduino.h>
#include "Button.h"

class Joystick : public Button {
private:
  const int xPin;
  const int yPin;
  int xValue;
  int yValue;
  static const int RESOLUTION = 4096; // 12-bit resolution (2^12)

public:
  Joystick(int buttonPin, int xPin, int yPin, unsigned long debounceIntervalMillis);
  Joystick(int buttonPin, int xPin, int yPin, unsigned long debounceIntervalMillis, bool positiveLogic);
  
  void update();
  
  int getXValue();
  int getYValue();
  float getXNormalized();
  float getYNormalized();
};
