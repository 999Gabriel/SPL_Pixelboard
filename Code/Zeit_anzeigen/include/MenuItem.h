#pragma once

#include <Arduino.h>
#include <functional>

// Basis-Klasse für alle Apps
class BaseApp {
public:
  virtual ~BaseApp() {}
  virtual void init() = 0;
  virtual void update() = 0;
  virtual void render() = 0;
  virtual void cleanup() = 0;
  virtual const char* getName() const = 0;
};

// Menü-Item Struktur
struct MenuItem {
  const char* name;
  BaseApp* app;
  bool enabled;

  MenuItem(const char* name, BaseApp* app, bool enabled = true)
    : name(name), app(app), enabled(enabled) {}
};

// App-Status Enum
enum class AppState {
  MENU,
  RUNNING_APP,
  TRANSITIONING
};
