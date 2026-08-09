#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

void displaySensorData();
void displayTrainData();
void displayCalendarData();
void displayFlightNumber();
void displayFlightDestination();
void drawTestBitmap();

// Weather section layout. Defined per panel variant in weather_layout_<variant>.cpp;
// exactly one of those is compiled in, selected by build_src_filter.
void displayTodaysWeather();
void displayWeatherForecast();
void displayWeatherData();
void displayTodaysTempRange();
void displaySunTimes();

// Icon drawing. drawWeatherIcon() uses the 8x8 artwork (enlarged=true pixel-doubles
// it); drawWeatherIcon16() uses the native 16x16 artwork.
void drawWeatherIcon(int startx, int starty, int width, int height, uint8_t icon, bool enlarged);
void drawWeatherIcon16(int startx, int starty, uint8_t icon);
void drawHeartBeat();
void drawBitmap(int startx, int starty, int width, int height, uint32_t *bitmap);
void drawBitmap(int startx, int starty, int width, int height, uint32_t *bitmap, bool enlarged);
void getOpenMeteoData();

// Moon phase functions
uint8_t getMoonPhase();

// NWS heat index ("feels like") in Fahrenheit, from air temperature and relative
// humidity. Returns the air temperature unchanged when humidity is not a factor.
float heatIndexF(float tempF, float humidity);
bool isNightTime();
void drawMoonPhase(int startx, int starty, int width, int height, bool enlarged);

#endif