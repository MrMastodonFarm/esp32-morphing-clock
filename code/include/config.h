#ifndef CONFIG_H
#define CONFIG_H

// Settings shared by every panel variant live here. Anything that depends on the
// physical panel (geometry, fonts, per-section layout) lives in the variant header
// included at the bottom of this file, selected by the -D flag set in platformio.ini.

//#define MQTT_USE_SSL 1
//#define USE_ANDROID_AP 1

// How often we refresh the time from the NTP server
#define NTP_REFRESH_INTERVAL_SEC 3600

// Timezone difference from GMT, expressed in seconds
// NOTE: must agree with the timezone= parameter in the Open-Meteo URL in weather.cpp
#define TIMEZONE_DELTA_SEC -18000
// DST delta to apply
#define TIMEZONE_DST_SEC 3600

// How long are informational messages kept on screen
#define LOG_MESSAGE_PERSISTENCE_MSEC 15000

// How long do we consider the sensor data valid before declaring the sensor dead
#define SENSOR_DEAD_INTERVAL_SEC 600

// How often to check for display updates in main loop (ms)
// Replaces the 30ms Ticker to avoid ISR context conflicts
#define DISPLAY_UPDATE_INTERVAL_MS 100

// Panel drive brightness, in the library's row-width units (0..PANEL_WIDTH), NOT 0-255.
// The library's own default is 32.
//
// 12 is a deliberate, measured trade and should not be raised without re-testing. The
// wide clock ghosts: a lit row bleeds a faint green copy of itself onto the row directly
// above, because row 54 is driven by scan address 22 and address 21 (row 53) is clocked
// immediately before it - the data lands before the address settles. It showed as a
// dashed green line under the calendar row. Measured on 2026-08-09 against a static test
// pattern: 32 obvious, 16 present, 12 barely visible, 8 essentially clean but too dim to
// read across a lit room. Raising this back toward 32 will bring the ghost back.
//
// Lower brightness shortens the output-enable pulse, so there is less time for charge to
// bleed onto the neighbouring address. It is a workaround for marginal panel timing, not
// a fix - latch blanking, the knob actually intended for this, barely moved it (see
// display_init()). Overridable at runtime over MQTT; see MQTT_PANEL_BRIGHTNESS_TOPIC.
#define PANEL_BRIGHTNESS 12

// Watchdog settings
#define WDT_TIMEOUT 60   // If the WDT is not reset within X seconds, reboot the unit
        // Do NOT set this too low, or the WDT will prevent OTA updates from completing!!

// Widened watchdog used only for the duration of an OTA. The flash write in
// perform_update() blocks for the whole download without returning to loop(), so the
// normal 60s WDT races it - and when the WDT wins it panics mid-flash, rebooting
// onto the old image with no visible error. See perform_update() in ota_update.cpp.
#define OTA_WDT_TIMEOUT 300

// OTA HTTP timeouts. OTA_FIRST_BYTE_TIMEOUT_MS is how long perform_update() waits for
// the response body to actually start arriving before giving up. The stock libraries
// allow a blind 100ms here, which this clock's ~96ms-average WiFi RTT loses more often
// than not - see the note in ota_update.cpp.
#define OTA_HTTP_TIMEOUT_MS 20000
#define OTA_FIRST_BYTE_TIMEOUT_MS 15000

//Button pin
#define BUTTON1_PIN 32

//Buzzer pin
/* #define BUZZER_PIN 2
#define BUZZER_PWM_CHANNEL 0
#define BUZZER_PWM_RESOLUTION 8 */

/* Light sensor data
#define LIGHT_DATA_X 0
#define LIGHT_DATA_Y 9
#define LIGHT_DATA_WIDTH 44
#define LIGHT_DATA_HEIGHT 8
#define LIGHT_DATA_COLOR ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0x00 >> 3)
//Maximum lux value that will be accepted as valid (sometimes the sensor will return erroneous values)
#define LIGHT_THRESHOLD 9999
#define LIGHT_READ_INTERVAL_SEC 10 */

// How often to refresh weather forecast data
#define WEATHER_REFRESH_INTERVAL_SEC 3600

// Open-Meteo location (Alexandria, VA)
#define WEATHER_LATITUDE "38.8048"
#define WEATHER_LONGITUDE "-77.0469"

// Night time hours for moon phase display
// During night hours, the main weather icon shows moon phase instead of weather
#define NIGHT_START_HOUR 20  // 8 PM - when night begins
#define NIGHT_END_HOUR 6     // 6 AM - when night ends

// Status/log line along the bottom edge. Both panels are 64 rows tall, so this
// is the same row on either; only the clear width follows the panel.
#define LOG_MESSAGE_Y 56
#define LOG_MESSAGE_HEIGHT 8
#define LOG_MESSAGE_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

// ---------------------------------------------------------------------------
// Panel variant selection. platformio.ini defines exactly one of these.
// ---------------------------------------------------------------------------
#if defined(PANEL_VARIANT_128X64)
  #include "config_128x64.h"
#elif defined(PANEL_VARIANT_64X64)
  #include "config_64x64.h"
#else
  #error "No panel variant selected. Build with -DPANEL_VARIANT_128X64 or -DPANEL_VARIANT_64X64 (see platformio.ini)."
#endif

#endif
