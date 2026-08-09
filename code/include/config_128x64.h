#ifndef CONFIG_128X64_H
#define CONFIG_128X64_H

// Geometry and per-section styling for the 128x64 panel (two chained 64x64, or one
// 128x64). Included by config.h when -DPANEL_VARIANT_128X64 is set.
//
// This panel has room for the built-in 5x7 font in most sections, so UI_*/section
// font macros mostly select the default font (setFont(NULL)). TomThumb is used only
// where the section is genuinely tight.

// Human-readable variant name, printed in the boot banner on serial.
#define PANEL_VARIANT_NAME "128x64"

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

// Day of week + date. Drawn with the built-in font, whose cursor is the top-left
// corner, so DATE_BASELINE is 0 and the clear rect starts right at DOW_Y.
#define DOW_X 4
#define DOW_Y 37
#define DOW_COLOR ((0x00 & 0xF8) << 8) | ((0x40 & 0xFC) << 3) | (0xFF >> 3)
#define DATE_X DOW_X + 20
#define DATE_Y DOW_Y
#define DATE_COLOR DOW_COLOR
//Width and height are for both DATE and DOW
#define DATE_WIDTH 50
#define DATE_HEIGHT 9
#define DATE_FONT NULL
#define DATE_CLEAR_Y DOW_Y

// Weather sensor data.
// X=59 is not arbitrary: it centres the readout's ink (x=61..90) on 75.5, which is
// exactly the centre of the 16x16 today icon below it (x=68..83) and of the temp-range
// row below that. Move it and the middle column visibly stops lining up - so if this row
// ever needs more space, take it from the width, not the origin.
// The width is the clear rect and must cover the *widest* content, not the typical
// content. TomThumb is variable-advance, so a 3-digit humidity with no leading space on
// the temperature ("-12  F 100%") reaches x=93 where " 88  F  61%" stops at 90. 36 covers
// x=59..94 and still leaves 3px before displayWeatherForecast()'s clear rect at x=98.
#define SENSOR_DATA_X 59
#define SENSOR_DATA_Y 12
#define SENSOR_DATA_WIDTH 36
#define SENSOR_DATA_HEIGHT 5
#define SENSOR_DATA_COLOR ((0x00 & 0xF8) << 8) | ((0x8F & 0xFC) << 3) | (0x00 >> 3)
#define SENSOR_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
// Feels-like shown in place of humidity - see FEELS_LIKE_DELTA_F. Two bare numbers
// in one colour would be ambiguous, so this one is coloured and ends in a degree
// dot rather than a %.
// The colour also carries the direction, which matters once the value can come
// from a source that does wind chill as well as heat index: amber reads as hotter
// than the air, pale blue as colder. Amber deliberately matches SUNRISE_COLOR.
#define SENSOR_FEELSLIKE_HOT_COLOR ((0xFF & 0xF8) << 8) | ((0xA0 & 0xFC) << 3) | (0x00 >> 3)
#define SENSOR_FEELSLIKE_COLD_COLOR ((0x60 & 0xF8) << 8) | ((0xC8 & 0xFC) << 3) | (0xFF >> 3)

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

//Message Line (currently used for calendar next event)
#define MESSAGE_LINE_1_X 0
#define MESSAGE_LINE_1_Y 47
#define MESSAGE_LINE_1_WIDTH 128
#define MESSAGE_LINE_1_HEIGHT 8
#define MESSAGE_LINE_1_COLOR 0x04FB
#define MESSAGE_LINE_1_ERROR_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
#define CALENDAR_FONT NULL
#define CALENDAR_CURSOR_Y MESSAGE_LINE_1_Y
#define CALENDAR_DAYS_FORMAT " -%3d days"

//Flight Data - number on the left of the line, destination alongside it at +42.
#define FLIGHT_DATA_X 64
#define FLIGHT_DATA_Y 0
#define FLIGHT_DATA_WIDTH 64
#define FLIGHT_DATA_HEIGHT 8
#define FLIGHT_DATA_COLOR 0x04FB
#define FLIGHT_FONT NULL
#define FLIGHT_NUM_CLEAR_X FLIGHT_DATA_X
#define FLIGHT_NUM_CLEAR_Y FLIGHT_DATA_Y
#define FLIGHT_NUM_CLEAR_WIDTH FLIGHT_DATA_WIDTH - 24
#define FLIGHT_NUM_CURSOR_X FLIGHT_DATA_X
#define FLIGHT_NUM_CURSOR_Y FLIGHT_DATA_Y
#define FLIGHT_DEST_CLEAR_X FLIGHT_DATA_X + 42
#define FLIGHT_DEST_CLEAR_Y FLIGHT_DATA_Y
#define FLIGHT_DEST_CLEAR_WIDTH FLIGHT_DATA_WIDTH - 30
#define FLIGHT_DEST_CURSOR_X FLIGHT_DATA_X + 42
#define FLIGHT_DEST_CURSOR_Y FLIGHT_DATA_Y

#define BITMAP_X 0
#define BITMAP_Y 44

#define HEARTBEAT_X 120
#define HEARTBEAT_Y 21

// Weather - today, and 5-day forecast.
// The today icon is a native 16x16; the forecast is a vertical column down the right
// edge with min/max temps flanking each 8x8 icon.
#define WEATHER_TODAY_X 68
#define WEATHER_TODAY_Y 19
#define WEATHER_TODAY_SIZE 16

#define WEATHER_FORECAST_X 108
#define WEATHER_FORECAST_Y 12

//Temperature range for today
#define TEMPRANGE_X 60
#define TEMPRANGE_Y 42
#define TEMPRANGE_WIDTH 36
#define TEMPRANGE_HEIGHT 8
#define TEMPRANGE_COLOR ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0xFF >> 3)

//Sunrise and sunset, flanking today's weather icon (rise on the left, set on the right).
//These fill the only two gaps either side of the 16x16 icon, and 13px is exactly one
//TomThumb "H:MM" - there is no slack. The clock reaches x=52 on digits whose lower-right
//segment is lit (0/6/8), and displayWeatherForecast() clears from x=98.
//Both gaps exist only because the seconds digits are commented out in clock.cpp:
//digit1/digit0 sit at x=59 and x=72, so re-enabling seconds would overwrite these.
//Amber deliberately matches the sun icon's core; pink is the one warm hue that stays
//legible against the purple showers icon (violet did not) without reading as sunrise.
#define SUNRISE_X 55
#define SUNRISE_Y 29
#define SUNSET_X 85
#define SUNSET_Y 29
#define SUNTIME_WIDTH 13
#define SUNTIME_HEIGHT 6
#define SUNRISE_COLOR ((0xFF & 0xF8) << 8) | ((0xA0 & 0xFC) << 3) | (0x00 >> 3)
#define SUNSET_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0xA0 >> 3)

// Silly inside joke shown when the status line clears
#define CJB_MESSAGE "Go Team Chrob!!"

#endif
