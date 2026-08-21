#include "weather.h"
#include "common.h"
#include "config.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include <ArduinoJson.h>
#include <math.h>
//#include <Fonts/FreeSerifBold12pt7b.h>

uint8_t forecast5Days[5] = {0,0,0,0,0};
int8_t minTemp[5] = {0,0,0,0,0};
int8_t maxTemp[5] = {0,0,0,0,0};
int8_t minTempToday = 0;
int8_t maxTempToday = 0;
char sunriseToday[6] = "";
char sunsetToday[6] = "";
bool weatherFailed = false;
int failCount = 0;

// Last good forecast, persisted in NVS so a cold boot (the square clock loses power
// every night at 22:00) can draw something plausible while the first fetch is still
// failing. The forecast barely moves overnight, so yesterday's is right far more
// often than a blank panel is.
struct WeatherCache {
  uint32_t magic;
  uint8_t forecast[5];
  int8_t minT[5];
  int8_t maxT[5];
  int8_t minToday;
  int8_t maxToday;
  char sunrise[6];
  char sunset[6];
  uint32_t savedAt;  // epoch seconds, 0 if the clock had no valid time yet
};
static const uint32_t WEATHER_CACHE_MAGIC = 0x57544831;  // "WTH1"
static const uint32_t WEATHER_CACHE_MAX_AGE_SEC = WEATHER_CACHE_MAX_AGE_HOURS * 3600UL;

// When the data on the panel was actually fetched. Epoch is the authority (it survives
// a reboot via the cache); the millis stamp is the fallback for a boot that fetched
// live before NTP synced.
uint32_t weatherDataEpoch = 0;          // 0 = unknown
unsigned long weatherSuccessMillis = 0; // 0 = no live fetch yet this boot

static uint32_t nowEpochIfValid() {
  time_t now = time(nullptr);
  return now > 1600000000 ? (uint32_t)now : 0;  // before 2020 means NTP has not synced
}

static void saveWeatherCache() {
  WeatherCache c;
  c.magic = WEATHER_CACHE_MAGIC;
  memcpy(c.forecast, forecast5Days, sizeof(c.forecast));
  memcpy(c.minT, minTemp, sizeof(c.minT));
  memcpy(c.maxT, maxTemp, sizeof(c.maxT));
  c.minToday = minTempToday;
  c.maxToday = maxTempToday;
  memcpy(c.sunrise, sunriseToday, sizeof(c.sunrise));
  memcpy(c.sunset, sunsetToday, sizeof(c.sunset));
  c.savedAt = weatherDataEpoch;
  Preferences prefs;
  if (prefs.begin("weather", false)) {
    prefs.putBytes("last", &c, sizeof(c));
    prefs.end();
  }
}

bool loadWeatherCache() {
  WeatherCache c;
  Preferences prefs;
  if (!prefs.begin("weather", true)) return false;
  size_t n = prefs.getBytes("last", &c, sizeof(c));
  prefs.end();
  if (n != sizeof(c) || c.magic != WEATHER_CACHE_MAGIC) return false;
  uint32_t now = nowEpochIfValid();
  if (now && c.savedAt && now - c.savedAt > WEATHER_CACHE_MAX_AGE_SEC) {
    Serial.println("Weather cache too old, ignoring");
    return false;
  }
  memcpy(forecast5Days, c.forecast, sizeof(forecast5Days));
  memcpy(minTemp, c.minT, sizeof(minTemp));
  memcpy(maxTemp, c.maxT, sizeof(maxTemp));
  minTempToday = c.minToday;
  maxTempToday = c.maxToday;
  memcpy(sunriseToday, c.sunrise, sizeof(sunriseToday));
  memcpy(sunsetToday, c.sunset, sizeof(sunsetToday));
  sunriseToday[sizeof(sunriseToday) - 1] = 0;
  sunsetToday[sizeof(sunsetToday) - 1] = 0;
  weatherDataEpoch = c.savedAt;
  Serial.println("Weather: showing cached forecast until a fetch succeeds");
  return true;
}

// True when what the panel shows is older than WEATHER_STALE_AFTER_SEC - or cannot be
// dated at all. Rendered as the temp-range line in the error colour, so a feed that has
// been quietly failing for days is visible rather than just a forecast that looks odd.
bool weatherStale() {
  uint32_t now = nowEpochIfValid();
  if (now && weatherDataEpoch) {
    return now - weatherDataEpoch > WEATHER_STALE_AFTER_SEC;
  }
  if (weatherSuccessMillis) {
    return (millis() - weatherSuccessMillis) / 1000UL > WEATHER_STALE_AFTER_SEC;
  }
  return true;  // cached data with no usable timestamp, or nothing fetched at all
}





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

// ============================================================================
// Native 16x16 weather icons (for display without scaling)
// ============================================================================

#define _ 0x000000      // Black (background)
#define Y 0xFFFF00      // Yellow (sun)
#define O 0xFFA000      // Orange (sun accent)
#define C 0x00DDFF      // Cyan (cloud)
#define W 0xFFFFFF      // White (cloud highlight)
#define G 0x808090      // Gray (cloud shadow)
#define B 0x0080FF      // Blue (rain)
#define P 0x8000FF      // Purple (shower cloud)
#define L 0x00FFFF      // Light blue (lightning glow)
#define S 0xA0A0FF      // Snow color

// Sun 16x16 - bright sun with rays
uint32_t static sun_16x16[] = {
  _,_,_,_,_,_,Y,_,_,Y,_,_,_,_,_,_,
  _,_,Y,_,_,_,_,Y,Y,_,_,_,_,Y,_,_,
  _,_,_,Y,_,_,_,_,_,_,_,_,Y,_,_,_,
  _,_,_,_,_,_,Y,Y,Y,Y,_,_,_,_,_,_,
  _,_,_,_,_,Y,Y,Y,Y,Y,Y,_,_,_,_,_,
  _,_,_,_,Y,Y,Y,O,O,Y,Y,Y,_,_,_,_,
  Y,_,_,Y,Y,Y,O,O,O,O,Y,Y,Y,_,_,Y,
  _,Y,_,Y,Y,O,O,O,O,O,O,Y,Y,_,Y,_,
  _,Y,_,Y,Y,O,O,O,O,O,O,Y,Y,_,Y,_,
  Y,_,_,Y,Y,Y,O,O,O,O,Y,Y,Y,_,_,Y,
  _,_,_,_,Y,Y,Y,O,O,Y,Y,Y,_,_,_,_,
  _,_,_,_,_,Y,Y,Y,Y,Y,Y,_,_,_,_,_,
  _,_,_,_,_,_,Y,Y,Y,Y,_,_,_,_,_,_,
  _,_,_,Y,_,_,_,_,_,_,_,_,Y,_,_,_,
  _,_,Y,_,_,_,_,Y,Y,_,_,_,_,Y,_,_,
  _,_,_,_,_,_,Y,_,_,Y,_,_,_,_,_,_,
};

// Cloud 16x16 - fluffy cloud with shading
uint32_t static cloud_16x16[] = {
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,_,_,C,C,C,C,_,_,_,_,_,_,_,
  _,_,_,_,C,W,W,W,W,C,_,_,_,_,_,_,
  _,_,_,C,W,W,W,W,W,W,C,C,C,_,_,_,
  _,_,C,W,W,W,W,W,W,W,W,W,W,C,_,_,
  _,C,W,W,W,W,W,W,W,W,W,W,W,W,C,_,
  _,C,W,W,W,W,W,W,W,W,W,W,W,W,C,_,
  C,W,W,W,W,W,W,W,W,W,W,W,W,W,W,C,
  C,W,W,W,W,W,W,W,W,W,W,W,W,W,W,C,
  C,C,W,W,W,W,W,W,W,W,W,W,W,W,C,C,
  _,C,C,C,C,C,C,C,C,C,C,C,C,C,C,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
};

// Showers 16x16 - cloud with scattered rain drops
uint32_t static showers_16x16[] = {
  _,_,_,_,_,P,P,P,P,_,_,_,_,_,_,_,
  _,_,_,_,P,P,P,P,P,P,_,_,_,_,_,_,
  _,_,_,P,P,P,P,P,P,P,P,P,P,_,_,_,
  _,_,P,P,P,P,P,P,P,P,P,P,P,P,_,_,
  _,P,P,P,P,P,P,P,P,P,P,P,P,P,P,_,
  _,P,P,P,P,P,P,P,P,P,P,P,P,P,P,_,
  P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,
  _,P,P,P,P,P,P,P,P,P,P,P,P,P,P,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,B,_,_,_,B,_,_,_,B,_,_,_,_,_,
  _,_,B,_,_,_,B,_,_,_,B,_,_,_,B,_,
  _,_,_,_,B,_,_,_,B,_,_,_,B,_,_,_,
  _,_,_,_,B,_,_,_,B,_,_,_,B,_,_,_,
  _,B,_,_,_,_,B,_,_,_,B,_,_,_,_,_,
  _,B,_,_,_,_,B,_,_,_,B,_,_,_,B,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
};

// Rain 16x16 - heavy rain diagonal pattern
uint32_t static rain_16x16[] = {
  _,_,B,_,_,_,B,_,_,_,B,_,_,_,B,_,
  _,_,B,_,_,_,B,_,_,_,B,_,_,_,B,_,
  _,B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,
  _,B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,
  B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,_,
  B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,_,
  _,_,_,B,_,_,_,B,_,_,_,B,_,_,_,B,
  _,_,_,B,_,_,_,B,_,_,_,B,_,_,_,B,
  _,_,B,_,_,_,B,_,_,_,B,_,_,_,B,_,
  _,_,B,_,_,_,B,_,_,_,B,_,_,_,B,_,
  _,B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,
  _,B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,
  B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,_,
  B,_,_,_,B,_,_,_,B,_,_,_,B,_,_,_,
  _,_,_,B,_,_,_,B,_,_,_,B,_,_,_,B,
  _,_,_,B,_,_,_,B,_,_,_,B,_,_,_,B,
};

// Storm 16x16 - lightning bolt
uint32_t static storm_16x16[] = {
  _,_,_,_,_,_,_,_,_,Y,Y,Y,_,_,_,_,
  _,_,_,_,_,_,_,_,Y,Y,Y,_,_,_,_,_,
  _,_,_,_,_,_,_,Y,Y,Y,_,_,_,_,_,_,
  _,_,_,_,_,_,Y,Y,Y,_,_,_,_,_,_,_,
  _,_,_,_,_,Y,Y,Y,_,_,_,_,_,_,_,_,
  _,_,_,_,Y,Y,Y,_,_,_,_,_,_,_,_,_,
  _,_,_,Y,Y,Y,Y,Y,Y,Y,_,_,_,_,_,_,
  _,_,_,_,_,_,_,Y,Y,Y,_,_,_,_,_,_,
  _,_,_,_,_,_,Y,Y,Y,_,_,_,_,_,_,_,
  _,_,_,_,_,Y,Y,Y,_,_,_,_,_,_,_,_,
  _,_,_,_,Y,Y,Y,_,_,_,_,_,_,_,_,_,
  _,_,_,Y,Y,Y,_,_,_,_,_,_,_,_,_,_,
  _,_,Y,Y,Y,_,_,_,_,_,_,_,_,_,_,_,
  _,_,Y,Y,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,Y,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
};

// Snow 16x16 - snowflakes pattern
uint32_t static snow_16x16[] = {
  _,_,_,S,_,_,_,_,_,_,_,_,S,_,_,_,
  _,_,S,S,S,_,_,_,_,_,_,S,S,S,_,_,
  _,_,_,S,_,_,_,S,_,_,_,_,S,_,_,_,
  _,_,_,_,_,_,S,S,S,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,S,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,_,S,_,_,_,_,_,_,_,S,_,_,_,_,
  _,_,S,S,S,_,_,_,_,_,S,S,S,_,_,_,
  _,_,_,S,_,_,_,S,_,_,_,S,_,_,_,_,
  _,_,_,_,_,_,S,S,S,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,S,_,_,_,_,_,_,_,_,
  _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
  _,_,S,_,_,_,_,_,_,_,S,_,_,_,_,_,
  _,S,S,S,_,_,_,_,_,S,S,S,_,_,_,_,
  _,_,S,_,_,_,S,_,_,_,S,_,_,_,S,_,
  _,_,_,_,_,S,S,S,_,_,_,_,_,S,S,S,
};

#undef _
#undef Y
#undef O
#undef C
#undef W
#undef G
#undef B
#undef P
#undef L
#undef S

// Moon phase bitmaps (8x8)
// Moon colors: 0xFFFFC0 = pale yellow (lit), 0x303030 = dark gray (shadow)
#define MOON_LIT 0xFFFFC0
#define MOON_SHADOW 0x303030

// New Moon (phase 0) - mostly dark with faint outline
uint32_t static moon_new_8x8[] = {
  0x000000, 0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000, 0x000000,
  0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000,
  0x000000, 0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000, 0x000000,
};

// Waxing Crescent (phase 1) - small sliver lit on right
uint32_t static moon_waxing_crescent_8x8[] = {
  0x000000, 0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000, 0x000000,
  0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, 0x000000,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT,
  0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, 0x000000,
  0x000000, 0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000, 0x000000,
};

// First Quarter (phase 2) - right half lit
uint32_t static moon_first_quarter_8x8[] = {
  0x000000, 0x000000, 0x303030, 0x303030, MOON_LIT, MOON_LIT, 0x000000, 0x000000,
  0x000000, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000,
  0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x000000, 0x303030, 0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000,
  0x000000, 0x000000, 0x303030, 0x303030, MOON_LIT, MOON_LIT, 0x000000, 0x000000,
};

// Waxing Gibbous (phase 3) - mostly lit, small shadow on left
uint32_t static moon_waxing_gibbous_8x8[] = {
  0x000000, 0x000000, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000, 0x000000,
  0x000000, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000,
  0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x303030, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x000000, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000,
  0x000000, 0x000000, 0x303030, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000, 0x000000,
};

// Full Moon (phase 4) - fully lit
uint32_t static moon_full_8x8[] = {
  0x000000, 0x000000, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000, 0x000000,
  0x000000, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT,
  0x000000, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000,
  0x000000, 0x000000, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x000000, 0x000000,
};

// Waning Gibbous (phase 5) - mostly lit, small shadow on right
uint32_t static moon_waning_gibbous_8x8[] = {
  0x000000, 0x000000, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x000000, 0x000000,
  0x000000, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x000000,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030,
  0x000000, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x000000,
  0x000000, 0x000000, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x000000, 0x000000,
};

// Last Quarter (phase 6) - left half lit
uint32_t static moon_last_quarter_8x8[] = {
  0x000000, 0x000000, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x000000, 0x000000,
  0x000000, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x000000,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030,
  0x000000, MOON_LIT, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x000000,
  0x000000, 0x000000, MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x000000, 0x000000,
};

// Waning Crescent (phase 7) - small sliver lit on left
uint32_t static moon_waning_crescent_8x8[] = {
  0x000000, 0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000, 0x000000,
  0x000000, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000,
  MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  MOON_LIT, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030,
  0x000000, MOON_LIT, 0x303030, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000,
  0x000000, 0x000000, 0x303030, 0x303030, 0x303030, 0x303030, 0x000000, 0x000000,
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

// Calculate moon phase (0-7) based on current date
// Uses a known new moon reference date (Jan 6, 2000) and the synodic month (~29.53 days)
// Returns: 0=new, 1=waxing crescent, 2=first quarter, 3=waxing gibbous,
//          4=full, 5=waning gibbous, 6=last quarter, 7=waning crescent
// NWS heat index. This is the Rothfusz regression the National Weather Service actually
// publishes, not a rule of thumb, so the panel agrees with whatever weather app you
// check it against.
//
// The two-step structure is the NWS's own and matters: the regression is fitted for hot
// conditions and misbehaves below about 80F, so a simpler linear form is tried first and
// only handed over to the regression once it clears 80. The two correction terms cover
// the corners where the regression is known to be weak - very dry heat, and very humid
// weather in the low 80s.
float heatIndexF(float tempF, float humidity) {
  // Simple form first, averaged with the air temperature as the NWS specifies.
  const float simple =
      0.5f * (tempF + 61.0f + ((tempF - 68.0f) * 1.2f) + (humidity * 0.094f));
  if ((simple + tempF) / 2.0f < 80.0f) {
    return (simple + tempF) / 2.0f;
  }

  const float t = tempF;
  const float r = humidity;
  float hi = -42.379f + 2.04901523f * t + 10.14333127f * r - 0.22475541f * t * r -
             0.00683783f * t * t - 0.05481717f * r * r + 0.00122874f * t * t * r +
             0.00085282f * t * r * r - 0.00000199f * t * t * r * r;

  if (r < 13.0f && t >= 80.0f && t <= 112.0f) {
    hi -= ((13.0f - r) / 4.0f) * sqrtf((17.0f - fabsf(t - 95.0f)) / 17.0f);
  } else if (r > 85.0f && t >= 80.0f && t <= 87.0f) {
    hi += ((r - 85.0f) / 10.0f) * ((87.0f - t) / 5.0f);
  }

  return hi;
}

uint8_t getMoonPhase() {
  // Reference: Jan 6, 2000 was a new moon
  // Calculate days since reference using timeinfo
  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;

  // Convert to Julian Day Number (simplified calculation)
  int a = (14 - month) / 12;
  int y = year + 4800 - a;
  int m = month + 12 * a - 3;
  long jd = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

  // Reference JD for Jan 6, 2000 (known new moon)
  long refJd = 2451550;

  // Days since reference new moon
  double daysSinceNew = (double)(jd - refJd);

  // Synodic month is approximately 29.53 days
  double synodicMonth = 29.53058867;

  // Calculate position in current lunar cycle (0.0 to 1.0)
  double lunarAge = fmod(daysSinceNew, synodicMonth);
  if (lunarAge < 0) lunarAge += synodicMonth;

  // Convert to phase (0-7)
  double phase = lunarAge / synodicMonth * 8.0;
  return (uint8_t)phase % 8;
}

// Check if it's currently night time
bool isNightTime() {
  int hour = timeinfo.tm_hour;
  // Night is between NIGHT_START_HOUR (evening) and NIGHT_END_HOUR (morning)
  return (hour >= NIGHT_START_HOUR || hour < NIGHT_END_HOUR);
}

// Draw moon phase icon
void drawMoonPhase(int startx, int starty, int width, int height, bool enlarged) {
  uint8_t phase = getMoonPhase();
  uint32_t* moonBitmap;

  switch (phase) {
    case 0: moonBitmap = moon_new_8x8; break;
    case 1: moonBitmap = moon_waxing_crescent_8x8; break;
    case 2: moonBitmap = moon_first_quarter_8x8; break;
    case 3: moonBitmap = moon_waxing_gibbous_8x8; break;
    case 4: moonBitmap = moon_full_8x8; break;
    case 5: moonBitmap = moon_waning_gibbous_8x8; break;
    case 6: moonBitmap = moon_last_quarter_8x8; break;
    case 7: moonBitmap = moon_waning_crescent_8x8; break;
    default: moonBitmap = moon_full_8x8; break;
  }

  drawBitmap(startx, starty, width, height, moonBitmap, enlarged);
}

// Draw one of the available weather icons from the 8x8 artwork.
// enlarged=true pixel-doubles it into a 16x16 space.
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
  }
}

// Draw a weather icon from the native 16x16 artwork - more detail than pixel-doubling
// the 8x8. Only the 128x64 layout has the room for these.
void drawWeatherIcon16(int startx, int starty, uint8_t icon) {
  switch (icon) {
    case 0:
      drawBitmap(startx, starty, 16, 16, sun_16x16, false);
      break;
    case 1:
      drawBitmap(startx, starty, 16, 16, cloud_16x16, false);
      break;
    case 2:
      drawBitmap(startx, starty, 16, 16, showers_16x16, false);
      break;
    case 3:
      drawBitmap(startx, starty, 16, 16, rain_16x16, false);
      break;
    case 4:
      drawBitmap(startx, starty, 16, 16, storm_16x16, false);
      break;
    case 5:
      drawBitmap(startx, starty, 16, 16, snow_16x16, false);
      break;
  }
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

// Open-Meteo returns sunrise/sunset as local-time ISO timestamps ("2026-08-07T06:15")
// because the request pins timezone=America/New_York, so the clock face just needs the
// "HH:MM" at offset 11 converted to 12-hour to match the main clock (no AM/PM).
void formatSunTime(const char *iso, char *out, size_t outLen) {
  if (iso == NULL || strlen(iso) < 16) {
    out[0] = '\0';   // nothing to show yet - leave the gap empty rather than crowd the icon
    return;
  }
  int hh = (iso[11] - '0') * 10 + (iso[12] - '0');
  int mm = (iso[14] - '0') * 10 + (iso[15] - '0');
  if (hh == 0) hh = 12;
  else if (hh >= 13) hh -= 12;
  snprintf(out, outLen, "%d:%02d", hh, mm);
}

// Return a mapping from WMO weather codes to internal icons:
// 0 - sun
// 1 - clouds
// 2 - showers
// 3 - rain
// 4 - storm
// 5 - snow
// Based on https://open-meteo.com/en/docs (WMO Weather interpretation codes)
int wmoWeatherCodeMapping(int code) {
  if (code == 0) return 0;           // Clear sky -> sun
  if (code <= 3) return 1;           // Partly cloudy to overcast -> clouds
  if (code <= 48) return 1;          // Fog -> clouds
  if (code <= 55) return 2;          // Drizzle -> showers
  if (code <= 67) return 3;          // Rain/freezing rain -> rain
  if (code <= 77) return 5;          // Snow -> snow
  if (code <= 82) return 2;          // Rain showers -> showers
  if (code <= 86) return 5;          // Snow showers -> snow
  if (code >= 95) return 4;          // Thunderstorm -> storm
  return 1;                          // Default -> clouds
}

bool getOpenMeteoData() {
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather: WiFi not connected, skipping");
    weatherFailed = true;
    failCount++;
    return false;
  }

  HTTPClient http;
  char url[256];
  DynamicJsonDocument doc(4096);  // Open-Meteo responses are smaller than AccuWeather

  snprintf(url, 256,
      "http://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
      "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset"
      "&temperature_unit=fahrenheit&timezone=America%%2FNew_York&forecast_days=5",
      WEATHER_LATITUDE, WEATHER_LONGITUDE);

  http.begin(url);  // Use plain HTTP - Open-Meteo supports it
  // Bounded, so a hung attempt cannot eat into the 60s watchdog. Retries are the
  // caller's job (see loop() in main.cpp) - this function makes exactly one attempt.
  http.setConnectTimeout(WEATHER_HTTP_TIMEOUT_MS);
  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("Weather HTTP error: %d\n", httpCode);
    logStatusMessage("Weather HTTP error!");
    weatherFailed = true;
    failCount++;
    http.end();
    return false;
  }

  String payload = http.getString();
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserialization failed: "));
    Serial.println(error.f_str());
    logStatusMessage("Weather data error!");
    weatherFailed = true;
    failCount++;
  }

  if (!error) {
    logStatusMessage("Weather success!");
    weatherFailed = false;
    failCount = 0;
  }

  http.end();

  // The old code retried here, recursively, behind a blocking delay(5000) - and only
  // once failCount exceeded 3, which a single call can never reach, so in practice a
  // failed fetch was simply left until the next hourly refresh. Retries now live in
  // loop() on a short backoff, where they cannot block the watchdog feed.
  if (weatherFailed) return false;

  // Populate the variables from Open-Meteo response
  JsonArray temps_max = doc["daily"]["temperature_2m_max"];
  JsonArray temps_min = doc["daily"]["temperature_2m_min"];
  JsonArray weather_codes = doc["daily"]["weather_code"];

  minTempToday = round(temps_min[0].as<double>());
  maxTempToday = round(temps_max[0].as<double>());

  formatSunTime(doc["daily"]["sunrise"][0].as<const char*>(), sunriseToday, sizeof(sunriseToday));
  formatSunTime(doc["daily"]["sunset"][0].as<const char*>(), sunsetToday, sizeof(sunsetToday));

  for (int i = 0; i < 5; i++) {
    forecast5Days[i] = wmoWeatherCodeMapping(weather_codes[i].as<int>());
    minTemp[i] = round(temps_min[i].as<double>());
    maxTemp[i] = round(temps_max[i].as<double>());
  }

  weatherDataEpoch = nowEpochIfValid();
  weatherSuccessMillis = millis();
  saveWeatherCache();
  return true;
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

