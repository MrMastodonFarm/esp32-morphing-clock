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
#define PANEL_WIDTH 64
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
#define DOW_X 0
#define DOW_Y 42
#define DOW_COLOR ((0x00 & 0xF8) << 8) | ((0x40 & 0xFC) << 3) | (0xFF >> 3)
// Date
#define DATE_X DOW_X + 12
#define DATE_Y DOW_Y
#define DATE_COLOR DOW_COLOR
//Width and height are for both DATE and DOW
#define DATE_WIDTH 36
#define DATE_HEIGHT 6

// Weather sensor data
#define SENSOR_DATA_X 0
#define SENSOR_DATA_Y 43
#define SENSOR_DATA_WIDTH 32
#define SENSOR_DATA_HEIGHT 6
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
#define TRAIN_DATA_WIDTH 41
#define TRAIN_DATA_HEIGHT 5
#define TRAIN_DATA_COLOR 0xFE80
#define TRAIN_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Blue Line Train Data - position not used so that all train data is a block
#define BLUE_TRAIN_DATA_X 0 //not used
#define BLUE_TRAIN_DATA_Y 7 //not used
#define BLUE_TRAIN_DATA_WIDTH 42
#define BLUE_TRAIN_DATA_HEIGHT 5
#define BLUE_TRAIN_DATA_COLOR 0x04FB
#define TRAIN_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Message Line (currently used for calendar next event)
#define MESSAGE_LINE_1_X 0
#define MESSAGE_LINE_1_Y 49
#define MESSAGE_LINE_1_WIDTH 64
#define MESSAGE_LINE_1_HEIGHT 6
#define MESSAGE_LINE_1_COLOR 0x04FB
#define MESSAGE_LINE_1_ERROR_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Flight Data
#define FLIGHT_DATA_X 41
#define FLIGHT_DATA_Y 0
#define FLIGHT_DATA_WIDTH 24
#define FLIGHT_DATA_HEIGHT 6
#define FLIGHT_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
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
#define TEMPRANGE_X 36 
#define TEMPRANGE_Y 42 
#define TEMPRANGE_WIDTH 36
#define TEMPRANGE_HEIGHT 6
#define TEMPRANGE_COLOR ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0xFF >> 3)

// How often to refresh weather forecast data
// (limited by API throttling)
#define WEATHER_REFRESH_INTERVAL_SEC 3600

#endif
