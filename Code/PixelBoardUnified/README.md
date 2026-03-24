# PixelBoardUnified

Unified PlatformIO project for the ESP32 PixelBoard.

## Included apps

- `CLOCK`
- `WEATHER`
- `SNAKE`
- `SCROLL`
- `INPUT`

All app logic is coordinated from a single `src/main.cpp` and split into FreeRTOS tasks.

## Setup

1. Copy `include/secrets.example.h` to `include/secrets.h`
2. Fill in `WIFI_SSID`, `WIFI_PASSWORD`, and `WEATHER_API_KEY`
3. Build with PlatformIO

## Controls

- Joystick up/down: navigate menu
- Short press: open selected app
- Long press: return to menu
- In `SNAKE`: joystick controls direction
