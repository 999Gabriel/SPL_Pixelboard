# PixelBoard – Projektdokumentation

> Zuletzt aktualisiert: März 2026

---

## 1. Projektübersicht

Das PixelBoard ist ein ESP32-basiertes LED-Matrix-Display (32×16 Pixel), das Wetterdaten von der **OpenWeatherMap API** für Innsbruck abruft und zentriert auf zwei WS2812B-Panels anzeigt. Die Werte (Temperatur, Luftfeuchtigkeit, Windgeschwindigkeit) rotieren alle 3 Sekunden in verschiedenen Farben.

---

## 2. Hardware

### 2.1 Mikrocontroller

| Eigenschaft     | Wert                   |
|-----------------|------------------------|
| Board           | **ESP32 DevKit v1**    |
| Framework       | Arduino                |
| Plattform       | Espressif32            |
| Monitor-Speed   | 115200 Baud            |

**Wichtig:** Der ESP32 unterstützt **ausschließlich 2,4 GHz WLAN** – kein 5 GHz!

### 2.2 LED-Panels

| Eigenschaft        | Wert                          |
|--------------------|-------------------------------|
| Typ                | **WS2812B** (NeoPixel-kompatibel) |
| Farbordnung        | **GRB**                       |
| Panels             | 2 Stück, je **8×32 Pixel** (256 LEDs) |
| Gesamtauflösung    | **32×16 Pixel** (512 LEDs)    |
| Helligkeit          | 50 (von 255)                 |

### 2.3 Panel-Anordnung & Verkabelung

```
        ┌─────────────────────────────────┐
        │       OBERES PANEL (leds_upper) │  ← Pin 25 (DATA_PIN_UPPER)
        │       8 Zeilen (y = 8..15)      │
        │       X ist gespiegelt          │
        │       (flippedX = 31 - x)       │
        ├─────────────────────────────────┤  ← Naht (y=7 / y=8)
        │       UNTERES PANEL (leds_lower)│  ← Pin 26 (DATA_PIN_LOWER)
        │       8 Zeilen (y = 0..7)       │
        │       X ist normal              │
        └─────────────────────────────────┘
```

- Beide Panels sind **VERTICAL_ZIGZAG_MATRIX** (Spalten abwechselnd auf/ab)
- Jede Spalte hat 8 LEDs
- Gerade Spalten (x=0,2,4...): LEDs von oben nach unten (y aufsteigend)
- Ungerade Spalten (x=1,3,5...): LEDs von unten nach oben (y absteigend → `7 - y`)

### 2.4 Pin-Belegung

| Pin | Funktion                |
|-----|-------------------------|
| 25  | DATA_PIN_UPPER (oberes Panel) |
| 26  | DATA_PIN_LOWER (unteres Panel) |
| 32  | Joystick Button (INPUT_PULLUP) – *für Snake-Modus* |
| 34  | Joystick X-Achse (analog) – *für Snake-Modus*      |
| 35  | Joystick Y-Achse (analog) – *für Snake-Modus*      |

### 2.5 Joystick (optional, für Snake-Spiel)

- Analoger Joystick mit Button
- Mittelposition: ~2048 (12-bit ADC)
- Threshold für Richtungserkennung: ±1000 vom Mittelpunkt
- Button: Active LOW mit internem Pull-Up

---

## 3. Software

### 3.1 PlatformIO-Konfiguration (`platformio.ini`)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    fastled/FastLED@^3.6.0
    https://github.com/AaronLiddiment/LEDMatrix
    bblanchon/ArduinoJson @ ^7
```

### 3.2 Bibliotheken

| Bibliothek      | Version | Zweck                                |
|-----------------|---------|--------------------------------------|
| FastLED         | ^3.6.0  | WS2812B LED-Steuerung               |
| LEDMatrix       | GitHub  | Matrix-Layout-Hilfe (cLEDMatrix)     |
| ArduinoJson     | ^7      | JSON-Parsing der API-Antwort         |
| WiFi            | Built-in| WLAN-Verbindung (ESP32)              |
| HTTPClient      | Built-in| HTTP-Requests an die API             |
| FreeRTOS        | Built-in| Multitasking (Tasks)                 |

### 3.3 WLAN-Konfiguration

| Eigenschaft  | Wert              |
|--------------|-------------------|
| SSID         | `GabPhone`        |
| Passwort     | `Covquf012wud`    |
| Band         | **Nur 2,4 GHz!**  |

**iPhone-Hotspot einrichten:**
1. Einstellungen → Persönlicher Hotspot
2. **"Kompatibilität maximieren" AKTIVIEREN** (erzwingt 2,4 GHz)
3. Hotspot einschalten

**WLAN-Verbindungslogik:**
- `WiFi.mode(WIFI_STA)` + `WiFi.disconnect(true)` vor Verbindungsaufbau
- 40 Versuche (20 Sekunden Timeout)
- Automatischer Neuversuch nach 10 Sekunden
- `ensureWiFi()` prüft vor jedem API-Call und verbindet ggf. neu

### 3.4 OpenWeatherMap API

| Eigenschaft  | Wert                                     |
|--------------|------------------------------------------|
| Endpunkt     | `http://api.openweathermap.org/data/2.5/weather` |
| Stadt        | `Innsbruck,AT`                           |
| Einheiten    | `metric` (Celsius, m/s)                  |
| API-Key      | Im Code hinterlegt                       |
| Protokoll    | **HTTP** (nicht HTTPS – spart Speicher auf dem ESP32) |

**Abgerufene Daten:**
- `main.temp` → Temperatur (°C)
- `main.humidity` → Luftfeuchtigkeit (%)
- `wind.speed` → Windgeschwindigkeit (m/s)

**Update-Intervall:** 60 Sekunden (`vTaskDelay(60000)`)

### 3.5 FreeRTOS Tasks

| Task             | Stack  | Core | Priorität | Funktion                        |
|------------------|--------|------|-----------|----------------------------------|
| `fetchWeatherData` | 8192 | 1    | 1         | API-Abruf alle 60s              |
| `updateDisplay`    | 4096 | 1    | 1         | Display-Rendering (3s Rotation) |

---

## 4. Display-Mapping (KRITISCH)

### 4.1 mapXY-Funktion

Die `mapXY(x, y)` Funktion wandelt logische (x,y)-Koordinaten in die physische LED-Adresse um:

```
Logische Koordinaten:          Physische Zuordnung:
(0,0) = oben links             → leds_lower, Spalte 0
(31,0) = oben rechts           → leds_lower, Spalte 31
(0,8) = Mitte links            → leds_upper, flippedX=31
(0,15) = unten links           → leds_upper, flippedX=31
```

**Unteres Panel (y < 8):**
- Normales X
- `led = x * 8 + (gerade Spalte ? y : 7 - y)`
- Array: `leds_lower`

**Oberes Panel (y >= 8):**
- Gespiegeltes X: `flippedX = 31 - x`
- Gespiegeltes Y: `flippedY = 15 - y`
- `led = flippedX * 8 + (gerade Spalte ? flippedY : 7 - flippedY)`
- Array: `leds_upper`

### 4.2 Die 180°-Drehung (setPixel)

Da das Display physisch auf dem Kopf steht, wird in `setPixel()` eine 180°-Drehung durchgeführt:

```cpp
void setPixel(int x, int y, CRGB color) {
  LedAddress addr = mapXY(MATRIX_WIDTH - 1 - x, MATRIX_HEIGHT - 1 - y);
  addr.array[addr.index] = color;
}
```

**Das bedeutet:** `setPixel(x, y)` → `mapXY(31-x, 15-y)`

> **MERKE:** Die `mapXY`-Funktion selbst NICHT ändern! Die 180°-Korrektur passiert **nur** in `setPixel()`. Wenn du `mapXY` direkt aufrufst (z.B. für Snake), musst du die Drehung selbst berücksichtigen oder auch dort `setPixel` verwenden.

### 4.3 clearAll()

`FastLED.clear()` funktioniert NICHT zuverlässig mit zwei getrennten LED-Arrays! Immer `clearAll()` verwenden:

```cpp
void clearAll() {
  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    leds_upper[i] = CRGB::Black;
    leds_lower[i] = CRGB::Black;
  }
}
```

---

## 5. Font-System (3×5 Pixel)

### 5.1 Zeichensatz

Eigener minimalistischer 3×5-Pixel-Font. Jedes Zeichen ist 3 Pixel breit, 5 Pixel hoch.

| Index | Zeichen | Code im String |
|-------|---------|----------------|
| 0–9   | `0`–`9` | `'0'`–`'9'`  |
| 10    | `.`     | `'.'`          |
| 11    | `C`     | `'C'`          |
| 12    | `%`     | `'%'`          |
| 13    | `m`     | `'m'`          |
| 14    | `/`     | `'/'`          |
| 15    | `s`     | `'s'`          |
| 16    | `°`     | `'*'`          |
| 17    | `-`     | `'-'`          |

**Zeichenbreite:** 3px Glyph + 1px Abstand = **4px pro Zeichen**
**Leerzeichen:** 2px breit

### 5.2 Zentrierung

- **Horizontal:** `x = (32 - textBreite) / 2`
- **Vertikal:** `y = 6` → zentriert den 5px-Font in der 16px-Höhe (Zeilen 6–10)

### 5.3 Grad-Zeichen

Das Grad-Symbol (°) wird im String als `*` kodiert:
```cpp
snprintf(buffer, sizeof(buffer), "%.1f*C", temperature);
// Wird angezeigt als: 21.5°C
```

---

## 6. Anzeige-Rotation

Die drei Wetterwerte werden zyklisch mit je 3 Sekunden angezeigt:

| Wert              | Farbe   | Beispiel   |
|-------------------|---------|------------|
| Temperatur        | 🟢 Grün  | `21.5°C`  |
| Luftfeuchtigkeit  | 🟡 Gelb  | `65%`     |
| Windgeschwindigkeit | 🔵 Blau | `3.2m/s`  |

Negative Temperaturen werden mit Minus angezeigt: `-5.3°C`

---

## 7. Bekannte Probleme & Hinweise

### ESP32 & 5 GHz WLAN
Der ESP32 hat **keinen 5-GHz-Chip**. Der iPhone-Hotspot muss auf 2,4 GHz stehen ("Kompatibilität maximieren").

### FastLED.clear() vs clearAll()
**Immer `clearAll()` verwenden!** `FastLED.clear()` setzt bei zwei getrennten LED-Arrays (`leds_upper` / `leds_lower`) nicht zuverlässig alle LEDs zurück.

### cLEDMatrix-Objekt
Das `cLEDMatrix<-MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds` Objekt wird aktuell **nicht für das Rendering verwendet**. Alles läuft über `mapXY` → `setPixel`. Es wird nur in `setup()` initialisiert und ist für zukünftige Erweiterungen vorhanden.

### API-Key
Der OpenWeatherMap API-Key ist direkt im Code hinterlegt. Für Produktivnutzung sollte er ausgelagert werden.

### Speicherverbrauch (aktuell)
- **RAM:** ~16% (52 KB / 328 KB)
- **Flash:** ~73% (951 KB / 1311 KB)

---

## 8. Projektstruktur

```
WeatherAPI_PixelBoard/
├── platformio.ini          # PlatformIO-Konfiguration & Dependencies
├── PIXELBOARD.md           # Diese Dokumentation
├── src/
│   └── main.cpp            # Gesamter Quellcode
├── include/
│   └── README
├── lib/
│   └── README
└── test/
    └── README
```

---

## 9. Erweiterungsmöglichkeiten

- **Snake-Spiel:** War vorher implementiert (Joystick an Pin 32/34/35). Kann als weiterer FreeRTOS-Task mit Joystick-Button-Umschaltung wieder hinzugefügt werden. Dabei `setPixel()` für korrekte Orientierung verwenden!
- **Weitere Wetterdaten:** OpenWeatherMap liefert auch Wolkenbedeckung, Sonnenauf-/-untergang, etc.
- **Animationen:** Übergangseffekte zwischen den Werten (Fade, Slide)
- **Uhrzeit:** NTP-basierte Zeitanzeige
- **Mehrere Städte:** Rotation zwischen verschiedenen Standorten
- **Wettericons:** Kleine Symbole (Sonne, Wolke, Regen) neben den Werten

---

## 10. Schnellreferenz für Entwicklung

### Pixel setzen
```cpp
setPixel(x, y, CRGB::Red);  // Immer setPixel verwenden, nie mapXY direkt!
```

### Text zentriert anzeigen
```cpp
clearAll();
drawTextCentered("12.3*C", 6, CRGB::Green);  // * = °
FastLED.show();
```

### Neues Zeichen zum Font hinzufügen
1. Neuen 3×5 Bitmap-Eintrag in `font3x5[]` anhängen
2. Index in `drawTextCentered()` bei der char-Erkennung hinzufügen
3. In `getTextWidth()` den char mit `width += 4` hinzufügen

### WLAN-Status prüfen
```cpp
ensureWiFi();  // Verbindet automatisch neu falls getrennt
```
