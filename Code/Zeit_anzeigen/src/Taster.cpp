#include "Button/Taster.h"

// Konstruktor: Initialisiert den Tasterpin und Variablen
Taster::Taster(int pin) {
    tasterPin = pin;                   // Pin speichern
    pinMode(tasterPin, INPUT_PULLUP);  // Pin als Eingang mit Pull-up Widerstand setzen
    zustandTaster = false;              // Initialzustand des Tasters (nicht gedrückt)
    letzterEntprellterZustand = false;  // Anfangszustand: Taster ist nicht gedrückt
    gedruecktZeit = 0;                  // Zeitpunkt für den Start des Drucks initialisieren
    letzteZeit = 0;                     // Setzt den Zeitpunkt der letzten Änderung auf 0
    langeGedrueckt = false;             // Flag für langen Druck initialisieren
}

// Methode zur Aktualisierung des entprellten Zustands
void Taster::aktualisiere() {
    bool aktuellerZustand = !digitalRead(tasterPin); // Negation für korrekte Logik (LOW = gedrückt)
    unsigned long jetzt = millis();                   // Aktuelle Zeit erfassen

    // Wenn der Zustand sich geändert hat und die Entprellzeit vorbei ist
    if (aktuellerZustand != zustandTaster && (jetzt - letzteZeit) > entprellZeit) {
        zustandTaster = aktuellerZustand;   // Aktualisiere den entprellten Zustand
        letzteZeit = jetzt;                 // Merke die Zeit der letzten Zustandsänderung

        // Wenn der Taster gedrückt wurde, merken wir uns den Zeitpunkt
        if (zustandTaster) {
            gedruecktZeit = jetzt;         // Startzeit des Drucks
            langeGedrueckt = false;        // Zurücksetzen des Flags
        }
    }
}

// Methode, um zu überprüfen, ob der Taster kurz gedrückt wurde
bool Taster::wurdeGedrueckt() {
    unsigned long jetzt = millis();
    
    // Wenn der Taster losgelassen wurde
    if (!zustandTaster && letzterEntprellterZustand) {
        unsigned long gedruecktDauer = jetzt - gedruecktZeit; // Berechne die Druckdauer
        letzterEntprellterZustand = zustandTaster;            // Update des letzten entprellten Zustands

        // Wenn der Taster kürzer als 1 Sekunde gehalten wurde
        if (gedruecktDauer < 1000) {
            return true; // Signalisiert kurzen Tastendruck
        }
    }
    letzterEntprellterZustand = zustandTaster; // Update des letzten entprellten Zustands
    return false;                               // Kein kurzer Tastendruck erkannt
}

// Methode, um zu überprüfen, ob der Taster lang gedrückt wurde
bool Taster::wurdeLangeGedrueckt() {
    unsigned long jetzt = millis();
    
    // Wenn der Taster weiterhin gedrückt ist
    if (zustandTaster) {
        unsigned long gedruecktDauer = jetzt - gedruecktZeit; // Berechne die Druckdauer

        // Wenn der Taster länger als 1 Sekunde gehalten wurde
        if (gedruecktDauer >= 1000 && !langeGedrueckt) {
            langeGedrueckt = true; // Flag setzen, dass der Taster lange gedrückt wurde
            return true;           // Signalisiert langen Tastendruck
        }
    } else {
        langeGedrueckt = false; // Zurücksetzen, wenn der Taster losgelassen wird
    }
    
    // Update des letzten entprellten Zustands
    letzterEntprellterZustand = zustandTaster;
    return false;                               // Kein langer Tastendruck erkannt
}

