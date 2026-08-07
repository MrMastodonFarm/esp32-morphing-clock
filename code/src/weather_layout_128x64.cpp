// Weather section layout for the 128x64 panel.
// Compiled only into the 128x64 env (see build_src_filter in platformio.ini).
//
// Layout: a native 16x16 today icon in the middle of the panel, sunrise and sunset
// tucked into the gaps either side of it, the today min/max underneath, and a vertical
// 4-day forecast column down the right edge with min/max flanking each 8x8 icon.

#include "common.h"
#include "rgb_display.h"
#include "weather.h"

void displayTodaysWeather() {
  // Clear the area first (16x16 for the native icon)
  dma_display->fillRect(WEATHER_TODAY_X, WEATHER_TODAY_Y, WEATHER_TODAY_SIZE, WEATHER_TODAY_SIZE, 0);

  // At night, show moon phase instead of weather
  if (isNightTime()) {
    drawMoonPhase(WEATHER_TODAY_X, WEATHER_TODAY_Y, 8, 8, true);
  } else {
    drawWeatherIcon16(WEATHER_TODAY_X, WEATHER_TODAY_Y, forecast5Days[0]);
  }
}

void displayTodaysTempRange() {
  dma_display->fillRect(TEMPRANGE_X, TEMPRANGE_Y - 5, TEMPRANGE_WIDTH, TEMPRANGE_HEIGHT, 0);
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
  dma_display->setTextColor(TEMPRANGE_COLOR);
  dma_display->setFont(&TomThumb);
  dma_display->setCursor(TEMPRANGE_X, TEMPRANGE_Y);
  dma_display->printf("%3d/%3d  F", minTempToday, maxTempToday);

  // Draw the degree symbol manually
  dma_display->fillRect(TEMPRANGE_X + 24, TEMPRANGE_Y - 5, 2, 2, TEMPRANGE_COLOR);
  dma_display->setFont();
}

void displayWeatherForecast() {
  dma_display->fillRect(WEATHER_FORECAST_X - 10, WEATHER_FORECAST_Y, 32, 36, 0);
  dma_display->setFont(&TomThumb);
  for (int i=1; i<5; i++) {  //skip day 0, since we are already displaying it somewhere else using displayTodaysWeather()
    drawWeatherIcon(WEATHER_FORECAST_X, WEATHER_FORECAST_Y + 9*(i-1), 8, 8, forecast5Days[i], false);
    dma_display->setCursor(WEATHER_FORECAST_X - 10, WEATHER_FORECAST_Y + 6 + 9*(i-1));
    dma_display->printf("%3d", minTemp[i]);
    dma_display->setCursor(WEATHER_FORECAST_X + 7, WEATHER_FORECAST_Y + 6 + 9*(i-1));
    dma_display->printf("%3d", maxTemp[i]);
  }
  dma_display->setFont();
}

void displaySunTimes() {
  dma_display->fillRect(SUNRISE_X, SUNRISE_Y - 5, SUNTIME_WIDTH, SUNTIME_HEIGHT, 0);
  dma_display->fillRect(SUNSET_X, SUNSET_Y - 5, SUNTIME_WIDTH, SUNTIME_HEIGHT, 0);
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
  dma_display->setFont(&TomThumb);

  dma_display->setTextColor(SUNRISE_COLOR);
  dma_display->setCursor(SUNRISE_X, SUNRISE_Y);
  dma_display->print(sunriseToday);

  dma_display->setTextColor(SUNSET_COLOR);
  dma_display->setCursor(SUNSET_X, SUNSET_Y);
  dma_display->print(sunsetToday);

  dma_display->setFont();
}

void displayWeatherData() {
  displayTodaysWeather();
  displayTodaysTempRange();
  displayWeatherForecast();
  displaySunTimes();
}
