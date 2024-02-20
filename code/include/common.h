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
extern bool sensorDead;

//The actual sensor data
extern float sensorTemp;
extern int sensorHumi;
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
extern uint8_t minTemp[5];
extern uint8_t maxTemp[5];

//OTA update
extern char updateValue;

#endif

