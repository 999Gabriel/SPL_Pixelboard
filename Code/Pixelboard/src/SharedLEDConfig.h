#ifndef SHARED_LED_CONFIG_H
#define SHARED_LED_CONFIG_H

#include <Arduino.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>

// --- Hardware-Konfiguration --------------------------------------------------
#define pinTop         25   // alter "Top"-Pin, jetzt physisch unten
#define pinBottom      26   // alter "Bottom"-Pin, jetzt physisch oben

#define panelWidth     32
#define panelHeight     8

#define ledsPerPanel   (panelWidth * panelHeight)

#define brightness     25
#define colorOrder     GRB
#define chipset        WS2812

// --- Virtuelle Canvas -------------------------------------------------------
#define canvasWidth8   64
#define canvasHeight8   8

#define canvasWidth16  64
#define canvasHeight16 16

// --- Extern Declarations (defined in Laufschrift_2_panels.cpp) -------
extern CRGB canvas8Leds[canvasWidth8 * canvasHeight8];
extern CRGB canvas16Leds[canvasWidth16 * canvasHeight16];

extern cLEDMatrix<canvasWidth8,
           canvasHeight8,
           HORIZONTAL_MATRIX> canvas8;

extern cLEDMatrix<canvasWidth16,
           canvasHeight16,
           HORIZONTAL_MATRIX> canvas16;

// --- Physische Panels --------------------------------------------------------
// ACHTUNG: ledsTop = Pin 25 (physisch unten), ledsBottom = Pin 26 (physisch oben)
extern CRGB ledsTop[ledsPerPanel];
extern CRGB ledsBottom[ledsPerPanel];

extern cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelTop;     // logisches TOP
extern cLEDMatrix<panelWidth,
           panelHeight,
           VERTICAL_ZIGZAG_MATRIX> panelBottom;  // logisches BOTTOM

// --- Laufschrift & Timing ----------------------------------------------------
extern cLEDText scrollingText;
extern uint32_t lastFrameMs;
extern const uint16_t frameIntervalMs;
extern uint32_t lastWeatherUpdateMs;
extern const uint32_t weatherUpdateIntervalMs;

#endif // SHARED_LED_CONFIG_H
