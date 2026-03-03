#include "Button/Button.h"

Button::Button(int pin, unsigned long debounceIntervalMillis, bool positiveLogic)
  : pin(pin),
    debounceInterval(debounceIntervalMillis),
    positiveLogic(positiveLogic),
    currentState(false),
    lastState(false),
    lastDebouncedState(false),
    lastDebounceTime(0),
    pressStartTime(0),
    longPressTriggered(false) {
  pinMode(pin, positiveLogic ? INPUT : INPUT_PULLUP);
}

void Button::update() {
  bool rawState = readRawState();
  unsigned long now = millis();

  if (rawState != lastState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > debounceInterval) {
    if (rawState != currentState) {
      currentState = rawState;

      if (currentState) {
        pressStartTime = now;
        longPressTriggered = false;
      }
    }
  }

  lastState = rawState;
}

bool Button::readRawState() {
  int reading = digitalRead(pin);
  return positiveLogic ? (reading == HIGH) : (reading == LOW);
}

bool Button::isPressed() const {
  return currentState;
}

bool Button::wasPressed() {
  if (currentState && !lastDebouncedState && !longPressTriggered) {
    lastDebouncedState = currentState;
    return false; // Wait for release
  }

  if (!currentState && lastDebouncedState && !longPressTriggered) {
    lastDebouncedState = currentState;
    return true; // Short press completed
  }

  lastDebouncedState = currentState;
  return false;
}

bool Button::wasReleased() {
  bool released = !currentState && lastDebouncedState;
  lastDebouncedState = currentState;
  return released;
}

bool Button::wasLongPressed(unsigned long longPressDuration) {
  if (currentState && !longPressTriggered) {
    if ((millis() - pressStartTime) >= longPressDuration) {
      longPressTriggered = true;
      return true;
    }
  }
  return false;
}
