
#include "rgb_display.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "common.h"

//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
#include <WebSerial.h> 

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
	64, // Module width
	64, // Module height
	1, // chain length
	_pins // pin mapping
  );
  // mxconfig.gpio.e = E_PIN;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

	// MUST DO THIS FIRST!
	dma_display->begin(); // Use default values for matrix dimentions and pins supplied within ESP32-HUB75-MatrixPanel-I2S-DMA.h
  //dma_display->setPanelBrightness(110);
}

void logStatusMessage(const char *message) {
  Serial.println(message);
  //WebSerial.println(message);
  // Clear the last line first!
  dma_display->fillRect(0, 56, 128, 8, 0);

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

  dma_display->setFont();

  dma_display->setCursor(0, 56);   // Write on last line

  dma_display->setTextColor(LOG_MESSAGE_COLOR);
  dma_display->print(message);

  messageDisplayMillis = millis();
  logMessageActive = true;
}


void logStatusMessage(String message) {
  Serial.println(message);
  WebSerial.println(message);
  // Clear the last line first!
  dma_display->fillRect(0, 56, 128, 8, 0);

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

  dma_display->setFont();

  dma_display->setCursor(0, 56);   // Write on last line

  dma_display->setTextColor(dma_display->color444(255,0,0));
  dma_display->print(message);

  messageDisplayMillis = millis();
  logMessageActive = true;
}

void CJBMessage(String message) {
  // Clear the line first!
  dma_display->fillRect(0, 56, 128, 8, 0);

  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

dma_display->setFont();

  dma_display->setCursor(0, 56);   // Write on last line

  dma_display->setTextColor(dma_display->color444(255,0,0));
  dma_display->print(message);

  messageDisplayMillis = millis();
  logMessageActive = true;
}


void clearStatusMessage() {
   dma_display->fillRect(0, 56, 128, 8, 0); 
   logMessageActive = false;
   CJBMessage("Go Chrob!"); //refresh silly inside joke after the status message goes away
}

void displaySensorData() {
  if (sensorDead) {
    dma_display->fillRect(SENSOR_DATA_X, SENSOR_DATA_Y, SENSOR_DATA_WIDTH, SENSOR_DATA_HEIGHT, 0);
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setTextColor(SENSOR_ERROR_DATA_COLOR);
    
    dma_display->setFont();
    
    dma_display->setCursor(SENSOR_DATA_X, SENSOR_DATA_Y);   
    dma_display->print("No sensor data!");
  }

  if (newSensorData) {
    dma_display->fillRect(SENSOR_DATA_X, SENSOR_DATA_Y, SENSOR_DATA_WIDTH, SENSOR_DATA_HEIGHT, 0);
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setTextColor(SENSOR_DATA_COLOR);
    dma_display->setFont(&TomThumb);
    dma_display->setCursor(SENSOR_DATA_X, SENSOR_DATA_Y + 5);   
    dma_display->printf("%3.0f  F %3d%%", sensorTemp, sensorHumi);
    
    // Draw the degree symbol manually
    dma_display->fillRect(SENSOR_DATA_X + 11, SENSOR_DATA_Y, 2, 2, SENSOR_DATA_COLOR);
   
    dma_display->setFont();
    newSensorData = false;
  }
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
    dma_display->printf("%d %d %d %d", sensorTrain1, sensorTrain2, sensorTrain3, sensorTrain4);
    //Blue Line
    dma_display->fillRect(TRAIN_DATA_X, TRAIN_DATA_Y+7, TRAIN_DATA_WIDTH, TRAIN_DATA_HEIGHT, 0);
    dma_display->fillCircle(TRAIN_DATA_X+2, TRAIN_DATA_Y+9, 2, 0x04FB); //blue circle to identify Blue Line
    dma_display->setCursor(TRAIN_DATA_X+8, TRAIN_DATA_Y+12);   //Y offset because custom fonts draw from bottom instead of top
    dma_display->setTextColor(0x04FB);
    dma_display->printf("%d %d %d %d", sensorBlueTrain1, sensorBlueTrain2, sensorBlueTrain3, sensorBlueTrain4); 

    dma_display->setFont();
    newTrainData = false;
  }
}
void displayCalendarData() {
  if (newCalendarData) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->fillRect(MESSAGE_LINE_1_X, MESSAGE_LINE_1_Y, MESSAGE_LINE_1_WIDTH, MESSAGE_LINE_1_HEIGHT, 0);

    dma_display->setFont(&TomThumb);

    dma_display->setCursor(MESSAGE_LINE_1_X, MESSAGE_LINE_1_Y + 5);
    dma_display->setTextColor(MESSAGE_LINE_1_COLOR);
    dma_display->print(sensorNextEvent); 
    dma_display->printf(" -%3dd", sensorDaysTillNextEvent);
    //Serial.println(sensorNextEvent);
    newCalendarData = false; 
    dma_display->setFont();
  }
}
void displayFlightNumber() {
  if (newFlightNumber) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setFont();
    dma_display->setFont(&TomThumb);
    dma_display->fillRect(FLIGHT_DATA_X, FLIGHT_DATA_Y, FLIGHT_DATA_WIDTH, FLIGHT_DATA_HEIGHT, 0);
    dma_display->setCursor(FLIGHT_DATA_X, FLIGHT_DATA_Y + 5);
    dma_display->setTextColor(FLIGHT_DATA_COLOR);
    dma_display->print(sensorFlightNumber);
    Serial.println(sensorFlightNumber);
    newFlightNumber = false; 
    dma_display->setFont();
  }
}
void displayFlightDestination() {
  if (newFlightDestination) {
    dma_display->setTextSize(1);     // size 1 == 8 pixels high
    dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    dma_display->setFont(&TomThumb);
    dma_display->fillRect(FLIGHT_DATA_X, FLIGHT_DATA_Y + 6, FLIGHT_DATA_WIDTH, FLIGHT_DATA_HEIGHT, 0);
    dma_display->setCursor(FLIGHT_DATA_X + 5, FLIGHT_DATA_Y + 12);
    dma_display->setTextColor(FLIGHT_DATA_COLOR);
    dma_display->print(sensorFlightDestination);
    Serial.println(sensorFlightDestination);
    newFlightDestination = false; 
    dma_display->setFont();
  }
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