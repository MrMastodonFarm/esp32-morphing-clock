#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include "rgb_display.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPNtpClient.h>

#ifdef MQTT_USE_SSL
#include <HTTPClient.h>
extern WiFiClientSecure wifiClient;
#else
extern WiFiClient wifiClient;
#endif

extern int status;

// Initialize MQTT client
extern PubSubClient client;

//Time of last status update
extern unsigned long lastStatusSend;

//Time of last sensor events
extern unsigned long lastSensorRead;
extern unsigned long lastLightRead;
extern unsigned long lastLedBlink;

//Log message persistence
//Is a log message currently displayed?
extern bool logMessageActive;
//When was the message shown?
extern unsigned long messageDisplayMillis;

// NTP
extern bool wifiFirstConnected;

extern bool syncEventTriggered; // True if a time event has been triggered

//RGB display
extern MatrixPanel_I2S_DMA *dma_display;

//Current time and date
extern struct tm timeinfo;

//Flags to trigger display section updates
extern bool clockStartingUp;
extern bool newSensorData;
extern bool newTrainData;
extern bool newCalendarData;
extern bool newFlightNumber;
extern bool newFlightDestination;
extern volatile bool otaRequested;
extern bool sensorDead;

//The actual sensor data
extern float sensorTemp;
extern int sensorHumi;
// Feels-like pushed in over MQTT (WeatherFlow). feelsLikeValid is an explicit flag
// rather than a lastFeelsLikeRead == 0 sentinel: millis() is 0 at startup, so the
// sentinel made "never received" and "received at t=0" indistinguishable, and the
// staleness subtraction underflowed to ~49 days. displaySensorData() falls back to
// computing a heat index locally while this is false.
extern float sensorFeelsLike;
extern bool feelsLikeValid;
extern unsigned long lastFeelsLikeRead;
extern int sensorTrain1;
extern int sensorTrain2;
extern int sensorTrain3;
extern int sensorTrain4;
extern int sensorBlueTrain1;
extern int sensorBlueTrain2;
extern int sensorBlueTrain3;
extern int sensorBlueTrain4;
extern char sensorNextEvent[65];
extern int sensorDaysTillNextEvent;
extern char sensorFlightDestination[3];
extern char sensorFlightNumber[6];

//Just a heartbeat for the watchdog...
extern bool heartBeat;

//Light sensor
//extern Adafruit_TSL2591 tsl;

//Weather data
extern uint8_t forecast5Days[5];
extern int8_t minTempToday;
extern int8_t maxTempToday;
extern int8_t minTemp[5];
extern int8_t maxTemp[5];
extern char sunriseToday[6];
extern char sunsetToday[6];

//OTA update
extern char updateValue;

#endif

