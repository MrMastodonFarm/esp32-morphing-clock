#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

void displaySensorData();
void displayTrainData();
void displayCalendarData();
void displayFlightNumber();
void displayFlightDestination();
void drawTestBitmap();
void displayTodaysWeather();
void displayWeatherForecast();
void displayWeatherData();
void displayTodaysTempRange();
void drawHeartBeat();
void drawBitmap(int startx, int starty, int width, int height, uint32_t *bitmap);
void drawBitmap(int startx, int starty, int width, int height, uint32_t *bitmap, bool enlarged);
void getOpenMeteoData();

// Moon phase functions
uint8_t getMoonPhase();
bool isNightTime();
void drawMoonPhase(int startx, int starty, int width, int height, bool enlarged);

#endif