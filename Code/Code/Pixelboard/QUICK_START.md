# 🚀 QUICK START - PixelBoard Menü-System

## In CLion verwenden

### 1. Projekt in CLion öffnen
- **File → Open** → Wähle `/Users/gabriel/CLionProjects/Code/Pixelboard`
- CLion lädt automatisch die CMake-Konfiguration

### 2. Run-Konfigurationen verwenden
Im oberen rechten Bereich von CLion findest du jetzt diese Konfigurationen:

- **PlatformIO Build** ▶️ - Kompiliert das Projekt
- **PlatformIO Upload** ⬆️ - Kompiliert und lädt auf ESP32 hoch
- **PlatformIO Monitor** 📟 - Öffnet Serial Monitor
- **PlatformIO Clean** 🧹 - Löscht Build-Dateien

### 3. Erste Schritte
1. Wähle **"PlatformIO Build"** aus dem Dropdown
2. Klicke auf den grünen Play-Button ▶️
3. Warte bis Kompilierung fertig ist
4. ESP32 anschließen via USB
5. Wähle **"PlatformIO Upload"**
6. Klicke auf Play ▶️
7. Nach Upload: Wähle **"PlatformIO Monitor"**

## Alternative: Terminal verwenden

### Build-Script verwenden (Empfohlen!)
```bash
cd /Users/gabriel/CLionProjects/Code/Pixelboard

# Nur kompilieren
./build.sh build

# Kompilieren und hochladen
./build.sh upload

# Serial Monitor öffnen
./build.sh monitor

# Alles auf einmal (kompilieren + upload + monitor)
./build.sh all
```

### Direkte PlatformIO Commands
```bash
# Kompilieren
pio run

# Hochladen
pio run --target upload

# Serial Monitor
pio device monitor

# Aufräumen
pio run --target clean
```

## Troubleshooting

### Problem: "PlatformIO nicht gefunden"
**Lösung:** PlatformIO installieren
```bash
# Mit Homebrew (empfohlen für macOS)
brew install platformio

# Oder mit pip
pip install platformio
```

### Problem: "Port nicht gefunden"
**Lösung:** ESP32 USB-Treiber installieren
1. ESP32 anschließen
2. Prüfe: `ls /dev/cu.*` (sollte `/dev/cu.usbserial-*` zeigen)
3. Falls nicht: [CP210x Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) installieren

### Problem: "Permission denied"
**Lösung:** Berechtigungen setzen
```bash
chmod +x build.sh
```

### Problem: "Library nicht gefunden"
**Lösung:** Libraries installieren
```bash
pio lib install
```

## Hardware-Setup

### ESP32 Anschlüsse
```
Joystick:
  - Button → GPIO 32
  - X-Achse → GPIO 34
  - Y-Achse → GPIO 35
  - VCC → 3.3V
  - GND → GND

LED-Panels:
  - Panel Oben → GPIO 26
  - Panel Unten → GPIO 25
  - VCC → 5V (externes Netzteil empfohlen!)
  - GND → GND
```

⚠️ **WICHTIG:** LED-Panels brauchen externes 5V Netzteil (min. 2A)!

## Was passiert beim Start?

1. System initialisiert Display & Joystick
2. Verbindet mit WiFi "GabPhone"
3. Synchronisiert Zeit via NTP
4. Zeigt Hauptmenü mit 4 Optionen:
   - ✅ Uhr (funktioniert)
   - 🚧 Snake (Template vorhanden)
   - 🔜 Tetris (Placeholder)
   - 🔜 Pong (Placeholder)

## Bedienung

### Im Menü
- **Joystick Hoch/Runter:** Navigation
- **Joystick Button:** App auswählen

### In einer App
- **Joystick Button:** Zurück zum Menü

## Serial Monitor Output

Bei erfolgreichem Start siehst du:
```
========================================
   PixelBoard Menu System
   ESP32 + FreeRTOS
========================================

Initialisiere Joystick...
Initialisiere MenuManager...
Erstelle Apps...
Füge Apps zum Menü hinzu...
MenuManager initialisiert
DisplayManager initialisiert

========================================
   System bereit!
   Joystick: Hoch/Runter = Navigation
   Joystick: Drücken = Auswählen
   In App: Drücken = Zurück
========================================
```

## Nächste Schritte

1. ✅ Teste das Menü-System
2. 🎮 Implementiere Snake-Game (siehe `SNAKE_APP_TEMPLATE.md`)
3. 🎨 Füge eigene Apps hinzu
4. 🔧 Passe WiFi-Credentials an (`src/main.cpp`)

Viel Erfolg! 🚀
