#ifndef CONFIG_H5
#define CONFIG_H

//#define MQTT_USE_SSL 1
//#define USE_ANDROID_AP 1

// How often we refresh the time from the NTP server
#define NTP_REFRESH_INTERVAL_SEC 3600

// Timezone difference from GMT, expressed in seconds
#define TIMEZONE_DELTA_SEC -18000
// DST delta to apply
#define TIMEZONE_DST_SEC 3600

// How long are informational messages kept on screen
#define LOG_MESSAGE_PERSISTENCE_MSEC 15000

// How long do we consider the sensor data valid before declaring the sensor dead
#define SENSOR_DEAD_INTERVAL_SEC 600

//Button pin
#define BUTTON1_PIN 32

//Buzzer pin
/* #define BUZZER_PIN 2
#define BUZZER_PWM_CHANNEL 0
#define BUZZER_PWM_RESOLUTION 8 */

// Screen positioning settings
// Panel size
#define PANEL_WIDTH 128
#define PANEL_HEIGHT 64

// Clock
#define CLOCK_X 1
#define CLOCK_Y 15
#define CLOCK_SEGMENT_HEIGHT 8
#define CLOCK_SEGMENT_WIDTH 8
#define CLOCK_SEGMENT_SPACING 5
#define CLOCK_WIDTH 6*(CLOCK_SEGMENT_WIDTH+CLOCK_SEGMENT_SPACING)+4
#define CLOCK_HEIGHT 2*CLOCK_SEGMENT_HEIGHT+3
//color565 == ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)
#define CLOCK_DIGIT_COLOR  ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0xFF >> 3)
//Delay in ms for clock animation - should be below 30ms for a segment size of 8
#define CLOCK_ANIMATION_DELAY_MSEC 20

// Day of week
#define DOW_X 4
#define DOW_Y 37
#define DOW_COLOR ((0x00 & 0xF8) << 8) | ((0x40 & 0xFC) << 3) | (0xFF >> 3)
// Date
#define DATE_X DOW_X + 20
#define DATE_Y DOW_Y
#define DATE_COLOR DOW_COLOR
//Width and height are for both DATE and DOW
#define DATE_WIDTH 50
#define DATE_HEIGHT 9

// Weather sensor data
#define SENSOR_DATA_X 59
#define SENSOR_DATA_Y 12
#define SENSOR_DATA_WIDTH 32 //128
#define SENSOR_DATA_HEIGHT 5
#define SENSOR_DATA_COLOR ((0x00 & 0xF8) << 8) | ((0x8F & 0xFC) << 3) | (0x00 >> 3)
#define SENSOR_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

/* Light sensor data
#define LIGHT_DATA_X 0
#define LIGHT_DATA_Y 9
#define LIGHT_DATA_WIDTH 44
#define LIGHT_DATA_HEIGHT 8
#define LIGHT_DATA_COLOR ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0x00 >> 3)
//Maximum lux value that will be accepted as valid (sometimes the sensor will return erroneous values)
#define LIGHT_THRESHOLD 9999
#define LIGHT_READ_INTERVAL_SEC 10 */

// Yellow Line Train data
#define TRAIN_DATA_X 0
#define TRAIN_DATA_Y 0
#define TRAIN_DATA_WIDTH 52
#define TRAIN_DATA_HEIGHT 5
#define TRAIN_DATA_COLOR 0xFE80
#define TRAIN_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Blue Line Train Data - position not used so that all train data is a block
#define BLUE_TRAIN_DATA_X 0 //not used
#define BLUE_TRAIN_DATA_Y 7 //not used
#define BLUE_TRAIN_DATA_WIDTH 52
#define BLUE_TRAIN_DATA_HEIGHT 5
#define BLUE_TRAIN_DATA_COLOR 0x04FB
#define TRAIN_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Message Line (currently used for calendar next event)
#define MESSAGE_LINE_1_X 0
#define MESSAGE_LINE_1_Y 47
#define MESSAGE_LINE_1_WIDTH 128
#define MESSAGE_LINE_1_HEIGHT 8
#define MESSAGE_LINE_1_COLOR 0x04FB
#define MESSAGE_LINE_1_ERROR_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Flight Data
#define FLIGHT_DATA_X 64
#define FLIGHT_DATA_Y 0
#define FLIGHT_DATA_WIDTH 64
#define FLIGHT_DATA_HEIGHT 8
#define FLIGHT_DATA_COLOR 0x04FB
#define MESSAGE_LINE_1_ERROR_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

// Log messages at the bottom
#define LOG_MESSAGE_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

#define BITMAP_X 0
#define BITMAP_Y 44

#define HEARTBEAT_X 120
#define HEARTBEAT_Y 21

// Watchdog settings
#define WDT_TIMEOUT 60   // If the WDT is not reset within X seconds, reboot the unit
        // Do NOT set this too low, or the WDT will prevent OTA updates from completing!!

// Weather - today, and 5-day forecast
#define WEATHER_TODAY_X 68 
#define WEATHER_TODAY_Y 19 

#define WEATHER_FORECAST_X 108 
#define WEATHER_FORECAST_Y 12 

//Temperature range for today
#define TEMPRANGE_X 60 
#define TEMPRANGE_Y 42 
#define TEMPRANGE_WIDTH 36
#define TEMPRANGE_HEIGHT 8
#define TEMPRANGE_COLOR ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0xFF >> 3)

// How often to refresh weather forecast data
#define WEATHER_REFRESH_INTERVAL_SEC 3600

// Open-Meteo location (Alexandria, VA)
#define WEATHER_LATITUDE "38.8048"
#define WEATHER_LONGITUDE "-77.0469"

// Night time hours for moon phase display
// During night hours, the main weather icon shows moon phase instead of weather
#define NIGHT_START_HOUR 20  // 8 PM - when night begins
#define NIGHT_END_HOUR 7     // 7 AM - when night ends

#endif
