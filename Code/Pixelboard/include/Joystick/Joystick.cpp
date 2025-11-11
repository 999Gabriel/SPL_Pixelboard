#include "Joystick.h"
#include "Button.h"
#include "Arduino.h"

// Constructor with default logic
Joystick::Joystick(int buttonPin, int xPin, int yPin, unsigned long debounceIntervalMillis)
  : Button(buttonPin, debounceIntervalMillis), xPin(xPin), yPin(yPin), xValue(0), yValue(0) {
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
}

// Constructor with custom logic
Joystick::Joystick(int buttonPin, int xPin, int yPin, unsigned long debounceIntervalMillis, bool positiveLogic)
  : Button(buttonPin, debounceIntervalMillis, positiveLogic), xPin(xPin), yPin(yPin), xValue(0), yValue(0) {
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
}

// Update both button state and axis values
void Joystick::update() {
  Button::update();  // Call parent class update for button handling
  xValue = analogRead(xPin);
  yValue = analogRead(yPin);
}

// Get raw X axis value (0-4095)
int Joystick::getXValue() {
  return xValue;
}

// Get raw Y axis value (0-4095)
int Joystick::getYValue() {
  return yValue;
}

// Get normalized X axis value (0.0-1.0)
float Joystick::getXNormalized() {
  return (float)xValue / (RESOLUTION - 1);
}

// Get normalized Y axis value (0.0-1.0)
float Joystick::getYNormalized() {
  return (float)yValue / (RESOLUTION - 1);
}

