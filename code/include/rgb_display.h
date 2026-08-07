#ifndef RGB_DISPLAY_H
#define RGB_DISPLAY_H

#include "config.h"

// HUB75 pin mapping is per-board; config.h has already validated that exactly one
// panel variant is selected.
#if defined(PANEL_VARIANT_128X64)
  #include "panel_pins_128x64.h"
#elif defined(PANEL_VARIANT_64X64)
  #include "panel_pins_64x64.h"
#endif

#define MATRIX_WIDTH PANEL_WIDTH
#define MATRIX_HEIGHT PANEL_HEIGHT
#define NUM_LEDS PANEL_WIDTH*PANEL_HEIGHT

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
//#include <Fonts/Picopixel.h>  //Smaller fonts not currently used
//#include <Fonts/Org_01.h>
//#include <Fonts/Tiny3x3a2pt7b.h>
#include <Fonts/TomThumb.h>

uint16_t colorWheel(uint8_t pos);
//void drawText(int colorWheelOffset);
void display_init();
//void display_drawText();
void logStatusMessage(const char *message);
void logStatusMessage(String message);
void CJBMessage(String message);
void clearStatusMessage();
void displaySensorData();
void displayTrainData();
void displayCalendarData();
void displayFlightNumber();
void displayFlightDestination();
void displayTest(int delayMs);

#endif
