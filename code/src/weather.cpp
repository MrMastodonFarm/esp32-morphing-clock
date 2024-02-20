#include "weather.h"
#include "common.h"
#include "creds_accuweather.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
//#include <Fonts/FreeSerifBold12pt7b.h>

uint8_t forecast5Days[5] = {0,0,0,0,0};
uint8_t minTemp[5] = {0,0,0,0,0};
uint8_t maxTemp[5] = {0,0,0,0,0};
int8_t minTempToday = 0;
int8_t maxTempToday = 0;
bool weatherFailed = false;
int failCount = 0;





//Source: http://www.newdesignfile.com/post_pixelated-graphic-arts_325919/

//12x20
uint32_t static minion[] = {
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xAFAFAF, 0xAFAFAF, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 
  0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xAFAFAF, 0xFFFFFF, 0xFFFFFF, 0xAFAFAF, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 
  0x000000, 0x000000, 0x000000, 0xAFAFAF, 0xFFFFFF, 0x000000, 0x000000, 0xFFFFFF, 0xAFAFAF, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0xAFAFAF, 0xFFFFFF, 0x000000, 0x000000, 0xFFFFFF, 0xAFAFAF, 0x000000, 0x000000, 0x000000, 
  0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xAFAFAF, 0xFFFFFF, 0xFFFFFF, 0xAFAFAF, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 
  0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xAFAFAF, 0xAFAFAF, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 
  0x000000, 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFF00, 0xFFFF00, 0x000000, 
  0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 
  0x000000, 0x0000FF, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x0000FF, 0x000000, 
  0x000000, 0xFFFF00, 0x0000FF, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x0000FF, 0xFFFF00, 0x000000, 
  0x000000, 0XFFFF00, 0xFFFF00, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFF00, 0xFFFF00, 0x000000, 
  0x000000, 0XFFFF00, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFF00, 0x000000, 
  0x000000, 0XFFFF00, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFF00, 0x000000, 
  0x000000, 0XFFFF00, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFFF00, 0x000000, 
  0x000000, 0X000000, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 
  0x000000, 0x000000, 0xFFFFFF, 0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000, 0xFFFFFF, 0x000000, 0x000000, 
  0x000000, 0xFFFFFF, 0x000000, 0x000000, 0x000000, 0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000, 0x000000, 0xFFFFFF, 0x000000,
  0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF
};

/* Python code to convert 8x8 icons:
test= [ 0x78, 0x84, 0x84, 0x84, 0x64, 0x44, 0x44, 0x38]  // Column-based, from a SURE 16x32 matrix

def convert(test, width, height, color):
   for y in range(height):
     for x in range(width):
       cr_bit = (test[x] >> (height-y-1)) & 0x01
       if (cr_bit==1):
         print("{:s}, ".format(color), end='')
       else: 
         print("0x000000, ", end='')
     print()
*/

uint32_t static sun_8x8[] = {
  0xFFFF00, 0x000000, 0x000000, 0xFFFF00, 0x000000, 0x000000, 0x000000, 0xFFFF00, 
  0x000000, 0xFFFF00, 0x000000, 0x000000, 0x000000, 0x000000, 0xFFFF00, 0x000000, 
  0x000000, 0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFF00, 
  0xFFFF00, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 0x000000, 
  0x000000, 0xFFFF00, 0x000000, 0x000000, 0x000000, 0x000000, 0xFFFF00, 0x000000, 
  0xFFFF00, 0x000000, 0x000000, 0x000000, 0xFFFF00, 0x000000, 0x000000, 0xFFFF00,
};

uint32_t static cloud_8x8[] = {
  0x000000, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x00FFFF, 0x000000, 0x000000, 0x000000, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x000000, 
  0x00FFFF, 0x000000, 0x000000, 0x000000, 0x00FFFF, 0x000000, 0x000000, 0x00FFFF, 
  0x00FFFF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x00FFFF, 
  0x00FFFF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x00FFFF, 
  0x000000, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 
};

uint32_t static showers_8x8[] = {
  0x000000, 0x8000FF, 0x8000FF, 0x8000FF, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x8000FF, 0x000000, 0x000000, 0x000000, 0x8000FF, 0x8000FF, 0x8000FF, 0x000000, 
  0x8000FF, 0x000000, 0x000000, 0x000000, 0x8000FF, 0x000000, 0x000000, 0x8000FF, 
  0x8000FF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x8000FF, 
  0x8000FF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x8000FF, 
  0x000000, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x8000FF, 0x000000, 
  0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 
};

uint32_t static rain_8x8[] = {
  0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 
  0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 
  0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 
  0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 
  0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 
  0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 
  0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 
  0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000, 0x0000FF, 0x000000
};

uint32_t static storm_8x8[] = {
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x0000FF, 0x0000FF, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x000000, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x000000, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 0x000000, 
  0x0000FF, 0x000000, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x0000FF, 0x0000FF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x0000FF, 0x0000FF, 0x0000FF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 
};

uint32_t static snow_8x8[] = {
  0x8080FF, 0x000000, 0x8080FF, 0x000000, 0x000000, 0x8080FF, 0x000000, 0x8080FF, 
  0x000000, 0x8080FF, 0x000000, 0x000000, 0x000000, 0x000000, 0x8080FF, 0x000000, 
  0x8080FF, 0x000000, 0x8080FF, 0x000000, 0x000000, 0x8080FF, 0x000000, 0x8080FF, 
  0x000000, 0x000000, 0x000000, 0x8080FF, 0x8080FF, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x8080FF, 0x8080FF, 0x000000, 0x000000, 0x000000, 
  0x8080FF, 0x000000, 0x8080FF, 0x000000, 0x000000, 0x8080FF, 0x000000, 0x8080FF, 
  0x000000, 0x8080FF, 0x000000, 0x000000, 0x000000, 0x000000, 0x8080FF, 0x000000, 
  0x8080FF, 0x000000, 0x8080FF, 0x000000, 0x000000, 0x8080FF, 0x000000, 0x8080FF, 
};

uint32_t static heart_8x8[] = {
  0x000000, 0xFF0000, 0xFF0000, 0x000000, 0xFF0000, 0xFF0000, 0x000000, 0x000000, 
  0xFF0000, 0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000, 0xFF0000, 0x000000, 
  0xFF0000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xFF0000, 0x000000, 
  0x000000, 0xFF0000, 0x000000, 0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000, 
  0x000000, 0xFF0000, 0x000000, 0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0xFF0000, 0x000000, 0xFF0000, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0xFF0000, 0x000000, 0x000000, 0x000000, 0x000000, 
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};
//Moon phases
uint32_t static waxing_gibbous_16x16[] = {
  // 'waxing_gibbous_16', 16x16px
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 
0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 
0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 
0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 
0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 
0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 
0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 
0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 
0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 
0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};


// Get color565 directly from 24-bit RGB value
// TODO - replace arrays with color565 values!
uint16_t color565(uint32_t rgb) {
  return (((rgb>>16) & 0xF8) << 8) | 
    (((rgb>>8) & 0xFC) << 3) | 
    ((rgb & 0xFF) >> 3);
};

void drawTestBitmap() {
  /*drawBitmap(BITMAP_X, BITMAP_Y, 8, 8, sun_8x8);
  drawBitmap(BITMAP_X+9, BITMAP_Y, 8, 8, cloud_8x8);
  drawBitmap(BITMAP_X+18, BITMAP_Y, 8, 8, rain_8x8);
  drawBitmap(BITMAP_X+27, BITMAP_Y, 8, 8, showers_8x8);
  drawBitmap(BITMAP_X+36, BITMAP_Y, 8, 8, snow_8x8);
  drawBitmap(BITMAP_X+45, BITMAP_Y, 8, 8, storm_8x8);*/
  //drawBitmap(BITMAP_X+58, BITMAP_Y, 12, 20, minion);
}

// Draw one of the available weather icons in the specified space
void drawWeatherIcon(int startx, int starty, int width, int height, uint8_t icon, bool enlarged) {
  switch (icon) {
    case 0:
      drawBitmap(startx, starty, width, height, sun_8x8, enlarged);
      break;
    case 1:
      drawBitmap(startx, starty, width, height, cloud_8x8, enlarged);
      break;
    case 2:
      drawBitmap(startx, starty, width, height, showers_8x8, enlarged);
      break;
    case 3:
      drawBitmap(startx, starty, width, height, rain_8x8, enlarged);
      break;
    case 4:
      drawBitmap(startx, starty, width, height, storm_8x8, enlarged);
      break;
    case 5:
      drawBitmap(startx, starty, width, height, snow_8x8, enlarged);
      break;
    case 6:
      drawBitmap(startx, starty, width, height, waxing_gibbous_16x16, false);
      break;
  }
}

void displayTodaysWeather() {
  //drawWeatherIcon(WEATHER_TODAY_X, WEATHER_TODAY_Y, 8, 8, forecast5Days[0], true);
  drawWeatherIcon(WEATHER_TODAY_X, WEATHER_TODAY_Y, 16, 16, 6, false);
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
    //drawWeatherIcon(WEATHER_FORECAST_X, WEATHER_FORECAST_Y + 9*(i-1), 8, 8, forecast5Days[i], false);
    drawWeatherIcon(WEATHER_FORECAST_X, WEATHER_FORECAST_Y + 9*(i-1), 8, 8, 6, false);
    dma_display->setCursor(WEATHER_FORECAST_X - 10, WEATHER_FORECAST_Y + 6 + 9*(i-1)); 
    dma_display->printf("%3d", minTemp[i]);
    dma_display->setCursor(WEATHER_FORECAST_X + 7, WEATHER_FORECAST_Y + 6 + 9*(i-1)); 
    dma_display->printf("%3d", maxTemp[i]);
    dma_display->setFont();
  }
  dma_display->setFont();
}

void displayWeatherData() {
  displayTodaysWeather();
  displayTodaysTempRange();
  displayWeatherForecast();
}

//Source: https://github.com/witnessmenow/LED-Matrix-Display-Examples/blob/master/LED-Matrix-Mario-Display/LED-Matrix-Mario-Display.ino
void drawBitmap(int startx, int starty, int width, int height, uint32_t *bitmap) {
  int counter = 0;
  for (int yy = 0; yy < height; yy++) {
    for (int xx = 0; xx < width; xx++) {
      dma_display->drawPixel(startx+xx, starty+yy, color565(bitmap[counter]));
      counter++;
    }
  }
}

// Draw the bitmap, with an option to enlarge it by a factor of two
void drawBitmap(int startx, int starty, int width, int height, uint32_t *bitmap, bool enlarged) {
  int counter = 0;
  if (enlarged) {
    for (int yy = 0; yy < height; yy++) {
      for (int xx = 0; xx < width; xx++) {
        dma_display->drawPixel(startx+2*xx, starty+2*yy, color565(bitmap[counter]));
        dma_display->drawPixel(startx+2*xx+1, starty+2*yy, color565(bitmap[counter]));
        dma_display->drawPixel(startx+2*xx, starty+2*yy+1, color565(bitmap[counter]));
        dma_display->drawPixel(startx+2*xx+1, starty+2*yy+1, color565(bitmap[counter]));
        counter++;
      }
    }
  }
  else drawBitmap(startx, starty, width, height, bitmap);
}

void drawHeartBeat() {
  if (!heartBeat) {
    dma_display->fillRect(HEARTBEAT_X, HEARTBEAT_Y, 8, 8, 0);
  }
  else {
    drawBitmap(HEARTBEAT_X, HEARTBEAT_Y, 8, 8, heart_8x8);
  }
}

// Return a mapping from the Accuweather icons to the
// internal icons: 
// 0 - sun
// 1 - clouds
// 2 - showers
// 3 - rain
// 4 - storm
// 5 - snow
// 6 - waxing gibbous
// Based on https://apidev.accuweather.com/developers/weatherIcons
int accuWeatherIconMapping(int icon) {
  //if (icon <= 5)  return 0;
  if (icon <= 5)  return 6;
  if (icon <= 11) return 1;
  if (icon <= 14) return 2;
  if (icon <= 17) return 4;
  if (icon == 18) return 3;
  if (icon <= 29) return 5;
  //if (icon <= 5)  return 0;
  if (icon <= 5)  return 6;
  if (icon <= 32) return 2;
  //return 0;
  return 6;
  
}

void getAccuWeatherData() {
  HTTPClient http;
  char url[256];
  DynamicJsonDocument doc(16384); // Might be overkill, since the Accuweather JSONs are about 3-5K in length - but better safe...

  StaticJsonDocument<300> filter;
  filter["DailyForecasts"][0]["Date"] = true;
  filter["DailyForecasts"][0]["Temperature"]["Minimum"]["Value"] = true;
  filter["DailyForecasts"][0]["Temperature"]["Maximum"]["Value"] = true;
  filter["DailyForecasts"][0]["Day"]["Icon"] = true;

  /* Filter should look like this:
  {
	"DailyForecasts": [
		"Date": True,
		"Temperature": {
			"Minimum": {
				"Value": True
			},
      "Maximum": {
        "Value": True
      }
		},
	"Day": {
		"Icon": true
	}
	]
  } */

  snprintf( url, 256, "http://dataservice.accuweather.com/forecasts/v1/daily/5day/%s?apikey=%s&metric=false", 
      ACCUWEATHER_CITY_CODE, ACCUWEATHER_API_KEY);

  http.begin(url);
  http.GET(); //TODO - check status code!

  DeserializationError error = deserializeJson( doc, http.getStream(), 
          DeserializationOption::Filter(filter));

  if (error) {
    Serial.print(F("deserialization failed: "));
    Serial.println(error.f_str());
    logStatusMessage("Weather data error!");
    weatherFailed = true; //The next ~15 lines of code are to give the weather pull a couple more tries before giving up
    failCount++;
  }

  if (!error) {
    logStatusMessage("Weather success!");
    weatherFailed = false;
    failCount = 0;
    
  }
  if (weatherFailed && failCount > 3) {
    delay(5000);
    logStatusMessage("Weather trying again");
    getAccuWeatherData();
  }

  doc.shrinkToFit();

  //Just in case we need to debug...
  //serializeJsonPretty(doc, Serial);

  //Populate the variables: 
  minTempToday = round( double(doc["DailyForecasts"][0]["Temperature"]["Minimum"]["Value"]) );
  maxTempToday = round( double(doc["DailyForecasts"][0]["Temperature"]["Maximum"]["Value"]) );
  
  Serial.println(minTempToday);
  Serial.println(maxTempToday);

  for (int i=0; i<5; i++) {
    forecast5Days[i] = accuWeatherIconMapping(doc["DailyForecasts"][i]["Day"]["Icon"]);
  }
  for (int i=0; i<5; i++) {
    minTemp[i] = round( double(doc["DailyForecasts"][i]["Temperature"]["Minimum"]["Value"]) );
    maxTemp[i] = round( double(doc["DailyForecasts"][i]["Temperature"]["Maximum"]["Value"]) );
  }
}

/* Start of code to get data from openweathermap - based on work by https://github.com/lefty01 
*/
void getOpenWeatherData() { /*
  // sanity check units ...
  // strcmp(units, "standard") ... "metric", or "imperial"
  snprintf(url, 128, "http://api.openweathermap.org/data/2.5/forecast?id=%u&units=%s&appid=%s",
	   loc_id, units, appid);

  
  // Allocate the largest possible document (platform dependent)
  // DynamicJsonDocument doc(ESP.getMaxFreeBlockSize());
  DynamicJsonDocument doc(8192);

  http.useHTTP10(true);
  http.begin(url);
  http.GET();

  DeserializationError error = deserializeJson(doc, http.getStream(),
					       DeserializationOption::Filter(filter));
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return 1;
  }
  */
}

