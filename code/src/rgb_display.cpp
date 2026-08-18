
#include "rgb_display.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include <math.h>

#include "common.h"
#include "weather.h"

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
// From: https://gist.github.com/davidegironi/3144efdc6d67e5df55438cc3cba613c8
uint16_t colorWheel(uint8_t pos) {
  if(pos < 85) {
    return dma_display->color565(pos * 3, 255 - pos * 3, 0);
  } else if(pos < 170) {
    pos -= 85;
    return dma_display->color565(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return dma_display->color565(0, pos * 3, 255 - pos * 3);
  }
}

void display_init() {
  HUB75_I2S_CFG::i2s_pins _pins={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
  HUB75_I2S_CFG mxconfig(
	PANEL_WIDTH, // Module width
	PANEL_HEIGHT, // Module height
	1, // chain length
	_pins // pin mapping
  );
  // mxconfig.gpio.e = E_PIN;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;

  // Ghost suppression, library default 1, maximum 4. This is the knob nominally meant
  // for the green row-above ghost described at PANEL_BRIGHTNESS, and on this panel it
  // is nearly useless: going 1 -> 2 -> 4 changed which pixels ghosted without making
  // them go away. PANEL_BRIGHTNESS is what actually fixed it.
  // 4 is kept because it is what the working configuration was measured with, not
  // because it earns its place - the two were never varied independently at the final
  // brightness. Re-testing it means the diagnostic build, which can sweep it live over
  // MQTT; do that rather than changing it on the assumption it does nothing.
  mxconfig.latch_blanking = 4;
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

	// MUST DO THIS FIRST!
	dma_display->begin(); // Use default values for matrix dimentions and pins supplied within ESP32-HUB75-MatrixPanel-I2S-DMA.h

  // Must follow begin(): setPanelBrightness() rewrites OE bits in the DMA buffer, which
  // does not exist until begin() has allocated it.
  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);
}

void logStatusMessage(const char *message) {
  Serial.println(message);
  // Clear the last line first!
  dma_display->fillRect(0, LOG_MESSAGE_Y, PANEL_WIDTH, LOG_MESSAGE_HEIGHT, 0);

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

  dma_display->setFont();

  dma_display->setCursor(0, LOG_MESSAGE_Y);   // Write on last line

  dma_display->setTextColor(LOG_MESSAGE_COLOR);
  dma_display->print(message);

  messageDisplayMillis = millis();
  logMessageActive = true;
}


void logStatusMessage(String message) {
  Serial.println(message);
  // Clear the last line first!
  dma_display->fillRect(0, LOG_MESSAGE_Y, PANEL_WIDTH, LOG_MESSAGE_HEIGHT, 0);

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

  dma_display->setFont();

  dma_display->setCursor(0, LOG_MESSAGE_Y);   // Write on last line

  dma_display->setTextColor(dma_display->color444(255,0,0));
  dma_display->print(message);

  messageDisplayMillis = millis();
  logMessageActive = true;
}

void CJBMessage(String message) {
  // Clear the line first!
  dma_display->fillRect(0, LOG_MESSAGE_Y, PANEL_WIDTH, LOG_MESSAGE_HEIGHT, 0);

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

dma_display->setFont();

  dma_display->setCursor(0, LOG_MESSAGE_Y);   // Write on last line

  dma_display->setTextColor(dma_display->color444(255,0,0));
  dma_display->print(message);

  messageDisplayMillis = millis();
  logMessageActive = true;
}


void clearStatusMessage() {
   dma_display->fillRect(0, LOG_MESSAGE_Y, PANEL_WIDTH, LOG_MESSAGE_HEIGHT, 0); 
   logMessageActive = false;
   CJBMessage(CJB_MESSAGE); //refresh silly inside joke after the status message goes away
}

// Which feels-like number to trust, what colour it has earned, and whether it is far
// enough from the air temperature to be worth drawing at all.
//
// Shared by both panel layouts on purpose. The 128x64 draws one row and the 64x64 two
// stacked lines, but the *decision* is identical, and it is the subtle part: the
// feed-versus-fallback distinction and its muted colours are the whole reason a dead
// upstream cannot masquerade as a live reading. Duplicating that per variant is how the
// two would quietly drift apart.
struct FeelsLikeReading {
  float value;
  uint16_t color;
  bool show;
};

static FeelsLikeReading currentFeelsLike() {
  // Prefer the feels-like pushed in over MQTT (WeatherFlow: has wind and solar as
  // inputs, and covers the cold end via wind chill). Fall back to computing a heat
  // index locally when none has arrived recently - that upstream is a cloud service
  // and it does go away, and losing the whole readout with it would be worse than a
  // slightly less informed number.
  const bool haveFeed =
      feelsLikeValid &&
      (millis() - lastFeelsLikeRead) < 1000UL * SENSOR_DEAD_INTERVAL_SEC;
  const float value = haveFeed ? sensorFeelsLike : heatIndexF(sensorTemp, sensorHumi);

  // Threshold is on the absolute difference, not just "hotter than". Wind chill puts
  // feels-like *below* air temperature, and a one-sided test would silently disable
  // this all winter - which is exactly when it is most worth showing.
  const float delta = value - sensorTemp;

  // Hue is direction, intensity is provenance: a muted colour means nobody pushed us
  // a feels-like and this is our own heat index. Without that the fallback is
  // invisible - the panel would keep showing a plausible number with the feed dead,
  // which is precisely how the flight display went stale for five days unnoticed.
  const uint16_t color =
      (delta >= 0)
          ? (haveFeed ? SENSOR_FEELSLIKE_HOT_COLOR : SENSOR_FEELSLIKE_HOT_ESTIMATED_COLOR)
          : (haveFeed ? SENSOR_FEELSLIKE_COLD_COLOR : SENSOR_FEELSLIKE_COLD_ESTIMATED_COLOR);

  return {value, color, fabsf(delta) >= FEELS_LIKE_DELTA_F};
}

#ifdef SENSOR_DATA_STACKED
// Total pen advance for a string in `font` - the distance the cursor travels, which is
// what a following glyph or a hand-drawn dot is positioned by.
//
// NOT getTextBounds(): that measures the ink's bounding box, which for these strings came
// out 4px wider than the advance and pushed everything 4px off the right edge. Ink extent
// and pen advance are different measurements, and the one that matters when you are
// placing something *after* the text is the advance.
// Direct struct access rather than the pgm_read_* macros the GFX library uses
// internally: those exist for AVR's split address space, and both targets here (ESP32 and
// the host simulator) are flat, so PROGMEM is ordinary memory. pgm_read_pointer is not
// even declared outside Adafruit_GFX.cpp.
static uint16_t textAdvance(const GFXfont *font, const char *text) {
  uint16_t advance = 0;
  for (const char *p = text; *p; ++p) {
    const uint8_t c = (uint8_t)*p;
    if (c < font->first || c > font->last) {
      continue;
    }
    advance += font->glyph[c - font->first].xAdvance;
  }
  return advance;
}

// TomThumb reserves 1px of right side bearing in every glyph's advance - the gap before
// the next character. At the end of a line nothing follows it, so it is dead space, and
// including it would leave the line 1px short of the edge everything else aligns to.
#define TOMTHUMB_RIGHT_BEARING 1

// Cursor X that puts `drawnWidth` pixels of content flush against the sensor slot's right
// edge. Clamped so an over-wide reading grows leftward into the slot rather than off the
// left of it - the slot is sized for the widest reading, but clamping means an
// unanticipated one degrades by overlapping its neighbour instead of vanishing.
static int16_t rightAlignedSensorX(uint16_t drawnWidth) {
  const int16_t x = SENSOR_DATA_X + SENSOR_DATA_WIDTH - (int16_t)drawnWidth;
  return x < SENSOR_DATA_X ? (int16_t)SENSOR_DATA_X : x;
}
#endif

// Outdoor temp/humidity, or a dashed placeholder once the feed goes stale.
//
// Both states paint the same slot in the same font and clear the same rect, so a
// recovering sensor fully overwrites the error state. That matters: the dead branch has
// no flag guard (it must keep asserting itself for as long as the feed is stale), and it
// used to draw "No sensor data!" in the built-in 8x8 font - 90px of text in a 32px clear
// rect, which sprayed across the weather icon and the forecast column and could only be
// partially cleaned up by whatever repainted next.
void displaySensorData() {
  if (!sensorDead && !newSensorData) {
    return;
  }

  const uint16_t color = sensorDead ? SENSOR_ERROR_DATA_COLOR : SENSOR_DATA_COLOR;

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
  dma_display->setFont(&TomThumb);

#ifdef SENSOR_DATA_STACKED
  // Square panel: two stacked lines in the slot above the today icon, because three
  // numbers do not fit on one row this narrow. Line 1 holds the two temperatures side by
  // side so they can be compared at a glance; line 2 holds humidity, which keeps its %
  // because it is the one number here that is not degrees.
  //
  // Both lines are RIGHT-ALIGNED against the panel edge rather than left-aligned from
  // SENSOR_DATA_X. These are variable-width numbers - two digits most of the year, three
  // in a July heat index, four with a minus sign in a freeze - and left-aligning them
  // gets it wrong at both ends: a ragged gap on the right in the common case, and ink off
  // the edge of the panel in the uncommon one. Aligning to the edge that cannot move is
  // what lets 20px of everyday content and 28px of extreme content both land correctly.
  char tempStr[8] = "--";
  char flStr[10]  = "";
  char humStr[8]  = "--%";
  FeelsLikeReading fl = {0.0f, color, false};

  if (!sensorDead) {
    // Unpadded %.0f, not the 128x64's %3.0f: right-alignment is doing the job the
    // padding used to, and a leading space would push a three-digit reading off the slot.
    snprintf(tempStr, sizeof(tempStr), "%.0f", sensorTemp);
    fl = currentFeelsLike();
    if (fl.show) {
      snprintf(flStr, sizeof(flStr), " %.0f", fl.value);
    }
    snprintf(humStr, sizeof(humStr), "%d%%", sensorHumi);
  }

  char line1[16];
  snprintf(line1, sizeof(line1), "%s%s", tempStr, flStr);

  dma_display->fillRect(SENSOR_DATA_X, SENSOR_DATA_Y, SENSOR_DATA_WIDTH, SENSOR_DATA_HEIGHT, 0);
  // The degree dot is 2px of drawn width that is not in the string, and unlike a glyph it
  // fills its cell completely - so no bearing is subtracted here.
  dma_display->setCursor(rightAlignedSensorX(textAdvance(&TomThumb, line1) + 2),
                         SENSOR_DATA_Y + 5);   //Y offset because custom fonts draw from bottom instead of top
  dma_display->setTextColor(color);
  dma_display->print(tempStr);
  if (fl.show) {
    dma_display->setTextColor(fl.color);
    dma_display->print(flStr);
  }
  // The degree dot lands after whichever number ended the line - the feels-like when it
  // is shown, otherwise the air temperature - so the line always reads as degrees.
  // Positioned from the cursor rather than a fixed offset because TomThumb is
  // variable-advance: a minus sign or a third digit moves it.
  dma_display->fillRect(dma_display->getCursorX(), SENSOR_DATA_Y, 2, 2,
                        fl.show ? fl.color : color);

  dma_display->fillRect(SENSOR_DATA_X, SENSOR_DATA_LINE2_Y, SENSOR_DATA_WIDTH, SENSOR_DATA_HEIGHT, 0);
  dma_display->setCursor(rightAlignedSensorX(textAdvance(&TomThumb, humStr) -
                                             TOMTHUMB_RIGHT_BEARING),
                         SENSOR_DATA_LINE2_Y + 5);
  dma_display->setTextColor(color);
  dma_display->print(humStr);
#else
  // Wide panel: one row, "temp F humidity-or-feelslike".
  dma_display->fillRect(SENSOR_DATA_X, SENSOR_DATA_Y, SENSOR_DATA_WIDTH, SENSOR_DATA_HEIGHT, 0);
  dma_display->setTextColor(color);
  dma_display->setCursor(SENSOR_DATA_X, SENSOR_DATA_Y+5);   //Y offset because custom fonts draw from bottom instead of top

  if (sensorDead) {
    // Same character positions as the live format below, so the slot keeps its shape.
    dma_display->print(" --  F  --%");
  } else {
    dma_display->printf("%3.0f  F ", sensorTemp);

    const FeelsLikeReading fl = currentFeelsLike();
    if (fl.show) {
      dma_display->setTextColor(fl.color);
      dma_display->printf("%3.0f", fl.value);
      // Degree dot instead of a %, so it reads as a temperature. Positioned from the
      // cursor rather than a fixed offset because TomThumb is variable-advance - the
      // width of "%3.0f" depends on which digits landed in it.
      dma_display->fillRect(dma_display->getCursorX(), SENSOR_DATA_Y, 2, 2, fl.color);
    } else {
      dma_display->printf("%3d%%", sensorHumi);
    }
  }

  // Draw the degree symbol manually
  dma_display->fillRect(SENSOR_DATA_X + 11, SENSOR_DATA_Y, 2, 2, color);
#endif

  dma_display->setFont();
  newSensorData = false;
}
void displayTrainData() {
  if (newTrainData) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setFont(&TomThumb);
    //Yellow Line
    dma_display->fillRect(TRAIN_DATA_X, TRAIN_DATA_Y, TRAIN_DATA_WIDTH, TRAIN_DATA_HEIGHT, 0);
    dma_display->fillCircle(TRAIN_DATA_X+2, TRAIN_DATA_Y+2, 2, 0xFE80); //yellow circle to identify Yellow Line
    dma_display->setCursor(TRAIN_DATA_X+8, TRAIN_DATA_Y+5);   //Y offset because custom fonts draw from bottom instead of top
    dma_display->setTextColor(TRAIN_DATA_COLOR);
#if TRAIN_ARRIVALS_SHOWN >= 4
    dma_display->printf("%d %d %d %d", sensorTrain1, sensorTrain2, sensorTrain3, sensorTrain4);
#else
    dma_display->printf("%d %d %d", sensorTrain1, sensorTrain2, sensorTrain3);
#endif
    //Blue Line
    dma_display->fillRect(TRAIN_DATA_X, TRAIN_DATA_Y+7, TRAIN_DATA_WIDTH, TRAIN_DATA_HEIGHT, 0);
    dma_display->fillCircle(TRAIN_DATA_X+2, TRAIN_DATA_Y+9, 2, 0x04FB); //blue circle to identify Blue Line
    dma_display->setCursor(TRAIN_DATA_X+8, TRAIN_DATA_Y+12);   //Y offset because custom fonts draw from bottom instead of top
    dma_display->setTextColor(0x04FB);
#if TRAIN_ARRIVALS_SHOWN >= 4
    dma_display->printf("%d %d %d %d", sensorBlueTrain1, sensorBlueTrain2, sensorBlueTrain3, sensorBlueTrain4);
#else
    dma_display->printf("%d %d %d", sensorBlueTrain1, sensorBlueTrain2, sensorBlueTrain3);
#endif

    dma_display->setFont();
    newTrainData = false;
  }
}
void displayCalendarData() {
  if (newCalendarData) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->fillRect(MESSAGE_LINE_1_X, MESSAGE_LINE_1_Y, MESSAGE_LINE_1_WIDTH, MESSAGE_LINE_1_HEIGHT, 0);

    dma_display->setFont(CALENDAR_FONT);

    dma_display->setCursor(MESSAGE_LINE_1_X, CALENDAR_CURSOR_Y);
    dma_display->setTextColor(MESSAGE_LINE_1_COLOR);
    dma_display->print(sensorNextEvent);
    dma_display->printf(CALENDAR_DAYS_FORMAT, sensorDaysTillNextEvent);
    //Serial.println(sensorNextEvent);
    dma_display->setFont();
    newCalendarData = false;
  }
}
// Flight number and destination. Drawn only where the panel has the room for them -
// the square variant spends those exact pixels on the outdoor sensor readout, so it
// sets FLIGHT_DISPLAY_ENABLED to 0. The flag is still cleared in that case: the feed
// keeps arriving, and a set flag nothing consumes would re-trigger every loop().
void displayFlightNumber() {
#if !FLIGHT_DISPLAY_ENABLED
  newFlightNumber = false;
#else
  if (newFlightNumber) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setFont(FLIGHT_FONT);
    dma_display->fillRect(FLIGHT_NUM_CLEAR_X, FLIGHT_NUM_CLEAR_Y, FLIGHT_NUM_CLEAR_WIDTH, FLIGHT_DATA_HEIGHT, 0);
    dma_display->setCursor(FLIGHT_NUM_CURSOR_X, FLIGHT_NUM_CURSOR_Y);
    dma_display->setTextColor(FLIGHT_DATA_COLOR);
    dma_display->print(sensorFlightNumber);
    Serial.println(sensorFlightNumber);
    dma_display->setFont();
    newFlightNumber = false;
  }
#endif
}
void displayFlightDestination() {
#if !FLIGHT_DISPLAY_ENABLED
  newFlightDestination = false;
#else
  if (newFlightDestination) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setFont(FLIGHT_FONT);
    dma_display->fillRect(FLIGHT_DEST_CLEAR_X, FLIGHT_DEST_CLEAR_Y, FLIGHT_DEST_CLEAR_WIDTH, FLIGHT_DATA_HEIGHT, 0);
    dma_display->setCursor(FLIGHT_DEST_CURSOR_X, FLIGHT_DEST_CURSOR_Y);
    dma_display->setTextColor(FLIGHT_DATA_COLOR);
    dma_display->print(sensorFlightDestination);
    Serial.println(sensorFlightDestination);
    dma_display->setFont();
    newFlightDestination = false;
  }
#endif
}
/* void displayLightData(float luxValue) {
  dma_display->fillRect(LIGHT_DATA_X, LIGHT_DATA_Y, LIGHT_DATA_WIDTH, LIGHT_DATA_HEIGHT, 0);
  
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
  dma_display->setTextColor(SENSOR_DATA_COLOR);
  //    dma_display->setFont(&FreeSerifBold12pt7b);

  dma_display->setCursor(LIGHT_DATA_X, LIGHT_DATA_Y);   
  dma_display->printf("%4.1f lx", luxValue);
    

} */

void displayForecastData() {

}

// Simple R/G/B screen fill, for testing displays
void displayTest(int delayMs) {
  dma_display->fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, dma_display->color565(255, 0, 0));
  delay(delayMs);
  dma_display->fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, dma_display->color565(0, 255, 0));
  delay(delayMs);
  dma_display->fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, dma_display->color565(0, 0, 255));
  delay(delayMs);
  //dma_display->fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, dma_display->color565(255, 255, 255));
  //delay(delayMs);
  dma_display->fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, dma_display->color565(0, 0, 0));
}