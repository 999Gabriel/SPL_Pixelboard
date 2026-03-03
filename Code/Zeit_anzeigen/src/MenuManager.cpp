#include "MenuManager.h"
#include <LEDText.h>

MenuManager::MenuManager(Joystick* joystick)
  : selectedIndex(0), currentState(AppState::MENU), currentApp(nullptr),
    display(DisplayManager::getInstance()), joystick(joystick),
    lastUpdate(0), lastNavigationTime(0), joystickMoved(false),
    menuVisible(false), menuScrollOffset(0), menuAnimationOffset(0.0f) {}

void MenuManager::init() {
  display->init();
  Serial.println(F("MenuManager initialisiert"));
  Serial.print(F("Anzahl Menü-Items: "));
  Serial.println(menuItems.size());
  Serial.println(F("Langer Tasterdruck (1s) zum Öffnen/Schließen des Menüs"));
}

void MenuManager::addMenuItem(const char* name, BaseApp* app, bool enabled) {
  menuItems.push_back(MenuItem(name, app, enabled));
  Serial.print(F("Menü-Item hinzugefügt: "));
  Serial.println(name);
}

void MenuManager::update() {
  unsigned long now = millis();
  joystick->update();

  // Prüfe auf langen Tasterdruck für Menü-Toggle
  handleMenuToggle();

  if (now - lastUpdate >= MENU_UPDATE_INTERVAL) {
    lastUpdate = now;

    if (currentState == AppState::MENU && menuVisible) {
      // Im Menü: Navigation und Auswahl
      handleNavigation();
    } else if (currentState == AppState::RUNNING_APP) {
      // In App: Zurück zum Menü mit langem Druck (wird in handleMenuToggle behandelt)
      if (currentApp != nullptr) {
        currentApp->update();
      }
    } else if (currentState == AppState::MENU && !menuVisible) {
      // Menü geschlossen: Nur Zeit wird angezeigt, ClockApp läuft im Hintergrund
      if (menuItems.size() > 0 && menuItems[0].app != nullptr) {
        menuItems[0].app->update(); // Uhr (erstes Item) aktualisieren
      }
    }
  }
}

void MenuManager::handleMenuToggle() {
  // Langer Tasterdruck zum Öffnen/Schließen des Menüs
  if (joystick->wasLongPressed(LONG_PRESS_DURATION)) {
    if (currentState == AppState::RUNNING_APP) {
      // Aus App zurück zum Menü
      Serial.println(F("Langer Druck: Zurück zum Menü"));
      exitCurrentApp();
      menuVisible = true;
    } else {
      // Menü öffnen/schließen
      menuVisible = !menuVisible;
      Serial.print(F("Menü "));
      Serial.println(menuVisible ? F("geöffnet") : F("geschlossen"));

      if (!menuVisible) {
        // Menü geschlossen: Zurück zur Uhr
        if (currentApp != nullptr) {
          currentApp->cleanup();
          currentApp = nullptr;
        }
        currentState = AppState::MENU;
      }
    }
  }
}

void MenuManager::handleNavigation() {
  if (menuItems.empty()) return;
  unsigned long now = millis();
  int yValue = joystick->getYValue();

  // Navigation mit Joystick
  if (yValue < 1500 && !joystickMoved && (now - lastNavigationTime > JOYSTICK_DEBOUNCE)) {
    selectedIndex--;
    if (selectedIndex < 0) selectedIndex = menuItems.size() - 1;
    joystickMoved = true;
    lastNavigationTime = now;
    Serial.print(F("Menü nach oben: "));
    Serial.println(menuItems[selectedIndex].name);
  }
  else if (yValue > 2500 && !joystickMoved && (now - lastNavigationTime > JOYSTICK_DEBOUNCE)) {
    selectedIndex++;
    if (selectedIndex >= menuItems.size()) selectedIndex = 0;
    joystickMoved = true;
    lastNavigationTime = now;
    Serial.print(F("Menü nach unten: "));
    Serial.println(menuItems[selectedIndex].name);
  }
  else if (yValue >= 1500 && yValue <= 2500) {
    joystickMoved = false;
  }

  // Auswahl mit kurzem Tasterdruck
  if (joystick->wasPressed()) {
    Serial.print(F("Menü-Item ausgewählt: "));
    Serial.println(menuItems[selectedIndex].name);
    enterSelectedApp();
  }
}

void MenuManager::enterSelectedApp() {
  if (selectedIndex >= 0 && selectedIndex < menuItems.size()) {
    MenuItem& item = menuItems[selectedIndex];
    if (item.enabled && item.app != nullptr) {
      currentApp = item.app;
      currentState = AppState::RUNNING_APP;
      menuVisible = false; // Menü ausblenden wenn App läuft
      display->clear();
      currentApp->init();
      Serial.print(F("App gestartet: "));
      Serial.println(item.name);
    }
  }
}

void MenuManager::exitCurrentApp() {
  if (currentApp != nullptr) {
    currentApp->cleanup();
    currentApp = nullptr;
  }
  currentState = AppState::MENU;
  menuVisible = true;
  display->clear();
  Serial.println(F("App beendet, zurück zum Menü"));
}

void MenuManager::render() {
  display->clear();

  // Immer anzeigen: Weißer Rand oben
  drawBorderRow();

  if (currentState == AppState::RUNNING_APP && currentApp != nullptr) {
    // App läuft: App rendert den kompletten Bildschirm
    currentApp->render();
  } else {
    // Menü-Modus: Zeit + optional Menü
    drawTime();

    if (menuVisible) {
      drawMenu();
    }
  }

  display->show();
}

void MenuManager::drawBorderRow() {
  // Oberste Zeile: Weiße LEDs als Orientierungshilfe
  for (uint8_t x = 0; x < display->getWidth(); x++) {
    display->setPixel(x, BORDER_ROW, CRGB::White);
  }
}

void MenuManager::drawTime() {
  // Zeit in Rot in der Mitte anzeigen
  // Die ClockApp (erstes MenuItem) rendert die Zeit
  if (menuItems.size() > 0 && menuItems[0].app != nullptr && currentState == AppState::MENU) {
    menuItems[0].app->render();
  }
}

void MenuManager::drawMenu() {
  // PlayStation-Style horizontales Menü
  if (menuItems.empty()) return;

  const int menuY = MENU_START_ROW;
  const int spacing = 10; // Abstand zwischen Menü-Items

  // Sanfte Animation für Menü-Scroll
  float targetOffset = -selectedIndex * spacing;
  menuAnimationOffset += (targetOffset - menuAnimationOffset) * 0.2f;

  for (int i = 0; i < menuItems.size(); i++) {
    float xOffset = menuAnimationOffset + (i * spacing);
    bool selected = (i == selectedIndex);
    drawMenuItem(i, xOffset + 32, selected); // Zentriert bei x=32
  }
}

void MenuManager::drawMenuItem(int index, float xOffset, bool selected) {
  if (index < 0 || index >= menuItems.size()) return;

  const MenuItem& item = menuItems[index];
  const int yPos = MENU_START_ROW + 1;

  // Farbe je nach Auswahl und Status
  CRGB color;
  if (!item.enabled) {
    color = CRGB::Gray;
  } else if (selected) {
    color = CRGB::White; // Ausgewähltes Item in Weiß
  } else {
    color = CRGB(80, 80, 120); // Nicht ausgewählte Items gedimmt
  }

  // Einfache Text-Darstellung (erste Buchstaben des Namens)
  int startX = (int)xOffset;
  int len = strlen(item.name);

  // Zeichne einen Balken für das Item
  if (startX >= 0 && startX < display->getWidth()) {
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 8; x++) {
        int px = startX + x;
        int py = yPos + y;
        if (px >= 0 && px < display->getWidth() && py < display->getHeight()) {
          display->setPixel(px, py, color);
        }
      }
    }
  }
}

void MenuManager::menuTask(void* parameter) {
  MenuManager* menu = static_cast<MenuManager*>(parameter);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20);

  while (true) {
    menu->update();
    menu->render();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
