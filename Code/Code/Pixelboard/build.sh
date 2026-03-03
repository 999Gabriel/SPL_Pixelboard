#!/bin/zsh

# PixelBoard Build & Upload Script
# Verwendung: ./build.sh [build|upload|monitor|clean]

echo "╔════════════════════════════════════════════════════════╗"
echo "║     PixelBoard Menü-System - Build Script             ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Prüfe ob PlatformIO installiert ist
if ! command -v pio &> /dev/null && ! command -v platformio &> /dev/null; then
    echo "❌ PlatformIO ist nicht installiert!"
    echo ""
    echo "Installation:"
    echo "  brew install platformio"
    echo ""
    echo "Oder:"
    echo "  pip install platformio"
    exit 1
fi

# Setze PIO Command
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
else
    PIO_CMD="platformio"
fi

echo "✓ PlatformIO gefunden: $PIO_CMD"
echo ""

# Bestimme Aktion
ACTION=${1:-build}

case "$ACTION" in
    build)
        echo "🔨 Kompiliere Projekt..."
        $PIO_CMD run
        ;;
    upload)
        echo "📤 Kompiliere und uploade zu ESP32..."
        $PIO_CMD run --target upload
        ;;
    monitor)
        echo "📟 Öffne Serial Monitor..."
        $PIO_CMD device monitor
        ;;
    clean)
        echo "🧹 Säubere Build-Dateien..."
        $PIO_CMD run --target clean
        ;;
    all)
        echo "🚀 Kompiliere, uploade und starte Monitor..."
        $PIO_CMD run --target upload && $PIO_CMD device monitor
        ;;
    *)
        echo "❌ Unbekannte Aktion: $ACTION"
        echo ""
        echo "Verwendung: ./build.sh [build|upload|monitor|clean|all]"
        echo ""
        echo "  build   - Kompiliert das Projekt"
        echo "  upload  - Kompiliert und lädt auf ESP32 hoch"
        echo "  monitor - Öffnet Serial Monitor"
        echo "  clean   - Löscht Build-Dateien"
        echo "  all     - Kompiliert, uploaded und startet Monitor"
        exit 1
        ;;
esac
