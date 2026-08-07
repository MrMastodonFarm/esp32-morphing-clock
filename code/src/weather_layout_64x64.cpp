// Weather section layout for the square 64x64 panel.
// Compiled only into the 64x64 env (see build_src_filter in platformio.ini).
//
// Layout: today's icon is an 8x8 pixel-doubled to 16x16, the today min/max sits just
// below it, and the 4-day forecast is a horizontal strip of bare 8x8 icons along the
// bottom - there is no room here for the per-day temperatures the 128x64 shows.
//
// displaySunTimes() is a no-op on this variant: the 128x64 only fits sunrise/sunset in
// the two gaps flanking its 16x16 icon, and this panel has no equivalent slack.

#include "common.h"
#include "rgb_display.h"
#include "weather.h"

void displayTodaysWeather() {
  // Clear the area first (16x16 once the 8x8 is doubled)
  dma_display->fillRect(WEATHER_TODAY_X, WEATHER_TODAY_Y, WEATHER_TODAY_SIZE, WEATHER_TODAY_SIZE, 0);

  // At night, show moon phase instead of weather
  if (isNightTime()) {
    drawMoonPhase(WEATHER_TODAY_X, WEATHER_TODAY_Y, 8, 8, true);
  } else {
    drawWeatherIcon(WEATHER_TODAY_X, WEATHER_TODAY_Y, 8, 8, forecast5Days[0], true);
  }
}

void displayTodaysTempRange() {
  dma_display->fillRect(TEMPRANGE_X, TEMPRANGE_Y - 5, TEMPRANGE_WIDTH, TEMPRANGE_HEIGHT, 0);
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
  dma_display->setTextColor(TEMPRANGE_COLOR);
  dma_display->setFont(&TomThumb);
  dma_display->setCursor(TEMPRANGE_X, TEMPRANGE_Y);
  dma_display->printf("%3d/%3d", minTempToday, maxTempToday);

  // Draw the degree symbol manually
  dma_display->fillRect(TEMPRANGE_X + 23, TEMPRANGE_Y - 5, 2, 2, TEMPRANGE_COLOR);
  dma_display->setFont();
}

void displayWeatherForecast() {
  dma_display->fillRect(WEATHER_FORECAST_X, WEATHER_FORECAST_Y,
                        WEATHER_FORECAST_SPACING * 4, 8, 0);
  for (int i=1; i<5; i++) {  //skip day 0, since we are already displaying it somewhere else using displayTodaysWeather()
    drawWeatherIcon(WEATHER_FORECAST_X + WEATHER_FORECAST_SPACING*(i-1), WEATHER_FORECAST_Y,
                    8, 8, forecast5Days[i], false);
  }
  dma_display->setFont();
}

void displaySunTimes() {
  // No room on this panel - see the header comment.
}

void displayWeatherData() {
  displayTodaysWeather();
  displayTodaysTempRange();
  displayWeatherForecast();
  displaySunTimes();
}
