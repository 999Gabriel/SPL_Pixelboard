#pragma once

#include <Arduino.h>
#include <vector>
#include "MenuItem.h"
#include "DisplayManager.h"
#include "Joystick/Joystick.h"

// Neue Display-Bereiche
#define BORDER_ROW 0           // Oberste Zeile für weißen Rand
#define TIME_START_ROW 4       // Zeit-Anzeige mittig (Zeilen 4-10)
#define MENU_START_ROW 12      // Menü unten (Zeilen 12-15)

class MenuManager {
private:
  std::vector<MenuItem> menuItems;
  int selectedIndex;
  AppState currentState;
  BaseApp* currentApp;
  DisplayManager* display;
  Joystick* joystick;
  unsigned long lastUpdate;
  unsigned long lastNavigationTime;
  bool joystickMoved;
  bool menuVisible;              // Menü sichtbar oder versteckt
  int menuScrollOffset;          // Für scrollendes PlayStation-Style Menü
  float menuAnimationOffset;     // Für sanfte Animation

  static const unsigned long MENU_UPDATE_INTERVAL = 50;
  static const unsigned long JOYSTICK_DEBOUNCE = 200;
  static const unsigned long LONG_PRESS_DURATION = 1000; // 1 Sekunde für langen Druck

  void drawBorderRow();
  void drawTime();
  void drawMenu();
  void drawMenuItem(int index, float xOffset, bool selected);
  void handleNavigation();
  void handleMenuToggle();
  void enterSelectedApp();
  void exitCurrentApp();

public:
  MenuManager(Joystick* joystick);
  void init();
  void addMenuItem(const char* name, BaseApp* app, bool enabled = true);
  void update();
  void render();
  AppState getState() const { return currentState; }
  int getSelectedIndex() const { return selectedIndex; }
  bool isMenuVisible() const { return menuVisible; }
  static void menuTask(void* parameter);
};
