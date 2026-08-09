#ifndef CONFIG_64X64_H
#define CONFIG_64X64_H

// Geometry and per-section styling for the square 64x64 panel.
// Included by config.h when -DPANEL_VARIANT_64X64 is set.
//
// Half the width of the 128x64 means almost every section drops to TomThumb. TomThumb
// is a GFX custom font, so its cursor Y is the *baseline* rather than the top-left
// corner - that is where the recurring "+5" offsets and "-5" clear rects come from.

// Human-readable variant name, printed in the boot banner on serial.
#define PANEL_VARIANT_NAME "64x64"

// Panel size
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64

// Clock - 6mm segments instead of 8mm, tighter spacing.
// clock.cpp derives all six digit positions from these, so nothing else to set.
#define CLOCK_X 0
#define CLOCK_Y 15
#define CLOCK_SEGMENT_HEIGHT 6
#define CLOCK_SEGMENT_WIDTH 6
#define CLOCK_SEGMENT_SPACING 3
#define CLOCK_WIDTH 4*(CLOCK_SEGMENT_WIDTH+CLOCK_SEGMENT_SPACING)+4
#define CLOCK_HEIGHT 2*CLOCK_SEGMENT_HEIGHT+3
//color565 == ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)
#define CLOCK_DIGIT_COLOR  ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0xFF >> 3)
//Delay in ms for clock animation - should be below 30ms for a segment size of 8
#define CLOCK_ANIMATION_DELAY_MSEC 20

// Day of week + date, in TomThumb. Cursor Y stays at DOW_Y (the baseline) while the
// clear rect has to start 5px higher to cover the glyphs.
#define DOW_X 3
#define DOW_Y 37
#define DOW_COLOR ((0x00 & 0xF8) << 8) | ((0x40 & 0xFC) << 3) | (0xFF >> 3)
#define DATE_X DOW_X + 14
#define DATE_Y DOW_Y
#define DATE_COLOR DOW_COLOR
//Width and height are for both DATE and DOW
#define DATE_WIDTH 36
#define DATE_HEIGHT 6
#define DATE_FONT &TomThumb
#define DATE_CLEAR_Y DOW_Y - 5

// Weather sensor data.
// NOTE: X=65 puts this block off the right edge of a 64px-wide panel, so outdoor
// temp/humidity is effectively not shown on this variant. Inherited as-is from the
// 64x64 repo - move it on-panel if you want it back. The width still has to cover the
// widest drawn string (see config_128x64.h) so it works if you do.
#define SENSOR_DATA_X 65
#define SENSOR_DATA_Y 43
#define SENSOR_DATA_WIDTH 36
#define SENSOR_DATA_HEIGHT 6
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
#define TRAIN_DATA_WIDTH 41
#define TRAIN_DATA_HEIGHT 5
#define TRAIN_DATA_COLOR 0xFE80
#define TRAIN_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)

//Blue Line Train Data - position not used so that all train data is a block
#define BLUE_TRAIN_DATA_X 0 //not used
#define BLUE_TRAIN_DATA_Y 7 //not used
#define BLUE_TRAIN_DATA_WIDTH 43
#define BLUE_TRAIN_DATA_HEIGHT 5
#define BLUE_TRAIN_DATA_COLOR 0x04FB

//Message Line (currently used for calendar next event)
#define MESSAGE_LINE_1_X 0
#define MESSAGE_LINE_1_Y 49
#define MESSAGE_LINE_1_WIDTH 64
#define MESSAGE_LINE_1_HEIGHT 6
#define MESSAGE_LINE_1_COLOR 0x04FB
#define MESSAGE_LINE_1_ERROR_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
#define CALENDAR_FONT &TomThumb
#define CALENDAR_CURSOR_Y MESSAGE_LINE_1_Y + 5
#define CALENDAR_DAYS_FORMAT " -%3dd"

//Flight Data - number on the top line, destination stacked underneath it (there is no
//room to put them side by side as the 128x64 does).
#define FLIGHT_DATA_X 41
#define FLIGHT_DATA_Y 0
#define FLIGHT_DATA_WIDTH 24
#define FLIGHT_DATA_HEIGHT 6
#define FLIGHT_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
#define FLIGHT_FONT &TomThumb
#define FLIGHT_NUM_CLEAR_X FLIGHT_DATA_X
#define FLIGHT_NUM_CLEAR_Y FLIGHT_DATA_Y
#define FLIGHT_NUM_CLEAR_WIDTH FLIGHT_DATA_WIDTH
#define FLIGHT_NUM_CURSOR_X FLIGHT_DATA_X
#define FLIGHT_NUM_CURSOR_Y FLIGHT_DATA_Y + 5
#define FLIGHT_DEST_CLEAR_X FLIGHT_DATA_X
#define FLIGHT_DEST_CLEAR_Y FLIGHT_DATA_Y + 6
#define FLIGHT_DEST_CLEAR_WIDTH FLIGHT_DATA_WIDTH
#define FLIGHT_DEST_CURSOR_X FLIGHT_DATA_X + 5
#define FLIGHT_DEST_CURSOR_Y FLIGHT_DATA_Y + 12

#define BITMAP_X 0
#define BITMAP_Y 44

// NOTE: also off-panel at X=120 on a 64px-wide display. drawHeartBeat() is not
// currently called from loop(), so this is inert either way.
#define HEARTBEAT_X 120
#define HEARTBEAT_Y 21

// Weather - today, and 5-day forecast.
// Today's icon is an 8x8 pixel-doubled to 16x16 (this variant has no native 16x16
// artwork budget); the forecast is a horizontal strip along the bottom, icons only.
#define WEATHER_TODAY_X 43
#define WEATHER_TODAY_Y 14
#define WEATHER_TODAY_SIZE 16

#define WEATHER_FORECAST_X 7
#define WEATHER_FORECAST_Y 39
#define WEATHER_FORECAST_SPACING 14

//Temperature range for today
#define TEMPRANGE_X 39
#define TEMPRANGE_Y 37
#define TEMPRANGE_WIDTH 36
#define TEMPRANGE_HEIGHT 6
#define TEMPRANGE_COLOR ((0x00 & 0xF8) << 8) | ((0xFF & 0xFC) << 3) | (0xFF >> 3)

// Sunrise/sunset are not shown on this variant - the 128x64 fits them only in the two
// gaps flanking its 16x16 icon, and this panel has no equivalent slack.

// Silly inside joke shown when the status line clears
#define CJB_MESSAGE "Go Chrob!"

#endif
