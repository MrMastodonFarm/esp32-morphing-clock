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

// Weather sensor data. Lives in the slot the flight number/destination used to occupy,
// directly above the 16x16 today icon - which is why FLIGHT_DISPLAY_ENABLED is 0 below.
// These are literally the same pixels; the panel has no other opening (measured against
// every golden plus 10/11/12 o'clock renders, the only two free regions large enough for
// a line of TomThumb were this one and the status line's row, and the status line clears
// its full width on every message).
//
// Was X=65 - off the right edge of a 64px panel - inherited from the standalone 64x64
// repo, so this variant showed no live outdoor reading at all.
//
// TWO STACKED LINES, unlike the 128x64's single row, because three numbers do not fit
// across 28px:
//
//     line 1   air temperature, then feels-like beside it when it diverges    "88 97."
//     line 2   humidity, with the % that stops it reading as a temperature    "61%"
//
// Width is set by the WIDEST line this can ever draw, not the typical one. From
// TomThumb's real metrics (digit xAdvance 4, space xAdvance 2 - note the space is *half*
// a digit, not the same - plus the hand-drawn 2px degree dot):
//
//     "88 97."      20px   the everyday case
//     "101 114."    28px   an ordinary Alexandria July afternoon
//     "-12 -20."    28px   a cold snap with wind; '-' advances a full 4px
//
// So the slot is 28px, X=36..63, and both lines are drawn RIGHT-ALIGNED inside it (see
// rgb_display.cpp) rather than left-aligned from X. Alignment is what makes one slot serve
// both extremes: everyday content sits flush at x44..63 and a heat wave grows leftward to
// x41 without the edge ever moving.
//
// It was 24px at X=40 and left-aligned first, sized off the everyday case, and 101F with a
// heat index of 114 ran the last digit and the degree dot straight off the right edge -
// a summer bug that would not have shown up until July, on a wall, where it reads as a
// glitch rather than a layout error. sim/scenarios/hot.scn exists to keep it out.
//
// X=36 is only affordable because TRAIN_ARRIVALS_SHOWN is 3 (ink ends x32, so a 3px gap).
// Restoring the fourth arrival would push train ink to x42 and straight through this.
// The overlap with the train row's clear rect (x0..40) is harmless - both clear to black.
//
// The feels-like keeps the 128x64's FEELS_LIKE_DELTA_F threshold rather than always
// drawing: below it the value is within a couple of degrees of the air temperature, and
// "88 88" is noise. When it is hidden the degree dot moves onto the air temperature so
// the line still reads as a temperature rather than a bare number.
#define SENSOR_DATA_STACKED 1
#define SENSOR_DATA_X 36
#define SENSOR_DATA_Y 0
#define SENSOR_DATA_WIDTH 28
#define SENSOR_DATA_HEIGHT 6
// Second line's top edge. 7 keeps a 1px gap under line 1's 6px band and still lands the
// glyphs clear of the today icon, which starts at row 14.
#define SENSOR_DATA_LINE2_Y 7
#define SENSOR_DATA_COLOR ((0x00 & 0xF8) << 8) | ((0x8F & 0xFC) << 3) | (0x00 >> 3)
#define SENSOR_ERROR_DATA_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
// Feels-like shown in place of humidity - see FEELS_LIKE_DELTA_F. Two bare numbers in
// one colour would be ambiguous, so this one is coloured and ends in a degree dot
// rather than a %.
//
// Two axes. Hue is direction: red hotter than the air, blue colder. Intensity is
// provenance: full strength when WeatherFlow pushed the value, muted when it is our own
// heat index because the feed went quiet. A silent fallback is how the flight display
// sat stale for five days looking perfectly plausible.
//
// NOTE the hot hue is the same red as SENSOR_ERROR_DATA_COLOR. They cannot appear at
// once - the dead-sensor state replaces the whole line with dashes - but if that ever
// stops being true, change one of them.
#define SENSOR_FEELSLIKE_HOT_COLOR ((0xFF & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
#define SENSOR_FEELSLIKE_COLD_COLOR ((0x30 & 0xF8) << 8) | ((0x70 & 0xFC) << 3) | (0xFF >> 3)
#define SENSOR_FEELSLIKE_HOT_ESTIMATED_COLOR ((0x90 & 0xF8) << 8) | ((0x00 & 0xFC) << 3) | (0x00 >> 3)
// Near-unreachable in this climate: a heat index only lands 5F BELOW air temperature in
// very dry heat (110F/10% -> 104). Defined so the four states are total, not because it
// is expected to show up in Alexandria.
#define SENSOR_FEELSLIKE_COLD_ESTIMATED_COLOR ((0x18 & 0xF8) << 8) | ((0x38 & 0xFC) << 3) | (0x80 >> 3)

// Three arrivals per line, not the wide panel's four. This is arithmetic, not taste:
// "12 19 27 34" is 38px of TomThumb advance, and starting at TRAIN_DATA_X+8 its ink
// reaches x42 - through the sensor readout, whose left edge is x36. Four two-digit arrivals is
// the common case, not an edge one, so the row would overrun most of the time. Measured:
// three arrivals end at x32, leaving a 3px gap. (Pathological 3-digit values still
// overlap, which is what sim/scenarios/stress.scn shows on purpose; Metro headways mean a
// 100-minute arrival is not reachable.)
//
// The same overrun existed before, when these pixels held the flight code; it simply
// mattered less because a flight number changed a few times a day while the sensor block
// repaints every minute, so the clipping would now flicker.
//
// The fourth arrival is the one worth losing: it is 30-40 minutes out and not actionable.
#define TRAIN_ARRIVALS_SHOWN 3

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

// Flight display is OFF on this variant: the outdoor sensor readout took these pixels
// (see SENSOR_DATA_X above). The MQTT feed is still subscribed and parsed - only the
// drawing is skipped - so turning this back to 1 restores it with no other change,
// at the cost of the temp/humidity/feels-like block.
#define FLIGHT_DISPLAY_ENABLED 0

//Flight Data - number on the top line, destination stacked underneath it (there is no
//room to put them side by side as the 128x64 does). Unused while
//FLIGHT_DISPLAY_ENABLED is 0, kept so flipping it back needs no archaeology.
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
