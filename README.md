# HTL Pixelboard

Unified ESP32 Pixelboard project based on PlatformIO and FreeRTOS.

## Current Status

The active main project is `Code/PixelBoardUnified`.

Implemented in the unified build:

- FreeRTOS-based app/task structure
- 32x16 LED matrix rendering for the dual WS2812 panel setup
- Main menu on the pixelboard
- Clock app with NTP sync
- Weather app using OpenWeatherMap
- Google Sheets logging for fetched weather data
- Snake app
- Input diagnostic screen
- Serial logging for boot, WiFi, NTP, weather and app switches

## Main Project

Path:

- `Code/PixelBoardUnified`

Important files:

- `Code/PixelBoardUnified/src/main.cpp`
- `Code/PixelBoardUnified/platformio.ini`
- `Code/PixelBoardUnified/include/secrets.h`
- `Code/PixelBoardUnified/include/secrets.example.h`

## Controls

- Joystick up/down: menu navigation
- Short press: open selected app
- Long press: return to menu
- In Snake: joystick changes direction

## Hardware Mapping

- Upper LED panel data pin: `GPIO 25`
- Lower LED panel data pin: `GPIO 26`
- Joystick button: `GPIO 32`
- Joystick X axis: `GPIO 34`
- Joystick Y axis: `GPIO 35`

## Build

From `Code/PixelBoardUnified`:

```bash
pio run -e esp32dev
```

Upload:

```bash
pio run -e esp32dev --target upload
```

Serial monitor:

```bash
pio device monitor -b 115200
```

## Repository Notes

- `Code/PixelBoardUnified` is the maintained main codebase.
- Older directories such as `WeatherAPI_PixelBoard`, `Pixelboard`, `Zeit_anzeigen`, `Snake`, and `joystick_test` remain in the repo as earlier project stages and reference implementations.
- `Code/PixelBoardMainCode` has been removed because it was obsolete and superseded by the unified PlatformIO project.
