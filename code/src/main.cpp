/*
ESP32 Matrix Clock - Copyright (C) 2021 Bogdan Sass

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Ticker.h>
#include <esp_task_wdt.h>

#include "main.h"
#include "common.h"
#include "rgb_display.h"
#include "mqtt.h"
#include "creds_mqtt.h"
#include "clock.h"
#include "weather.h"

//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <WebSerial.h>

//AsyncWebServer server(80);

void recvMsg(uint8_t *data, size_t len){
  //WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  //WebSerial.println(d);
}

Ticker displayTicker;
unsigned long prevEpoch;
unsigned long lastNTPUpdate;
unsigned long lastWeatherUpdate;

//Just a blinking heart to show the main thread is still alive...
bool blinkOn;

void setup(){
  display_init();

  Serial.begin(115200);
  delay(10);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);

  displayTest(300);
// Set ESP32 host name
  String hostname = "MorphingClock";
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); //define hostname

  logStatusMessage("Connecting to WiFi...");
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  logStatusMessage("WiFi connected!");

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  //WebSerial.begin(&server);
  //WebSerial.msgCallback(recvMsg);
  //server.begin();

  logStatusMessage("NTP time...");
  configTime(TIMEZONE_DELTA_SEC, TIMEZONE_DST_SEC, "pool.ntp.org");
  lastNTPUpdate = millis();
  logStatusMessage("NTP done!");

  logStatusMessage("Getting weather...");
  getAccuWeatherData();
  lastWeatherUpdate = millis();
  logStatusMessage("Weather recvd!");

  logStatusMessage("MQTT connect...");

  #ifdef MQTT_USE_SSL
  wifiClient.setCACert(server_crt_str);       
  wifiClient.setCertificate(client_crt_str);  
  wifiClient.setPrivateKey(client_key_str);   
  #endif

  client.setServer( MQTT_SERVER, MQTT_PORT );
  client.setCallback(mqtt_callback);
  reconnect();
  lastStatusSend = 0;
  logStatusMessage("MQTT done!");

  logStatusMessage("Setting up watchdog...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  logStatusMessage("Woof!");
 
  logStatusMessage(WiFi.localIP().toString());
  drawTestBitmap();
  displayWeatherData();
  
  CJBMessage("Go Team Chrob!!"); //just a silly inside joke
  displayTicker.attach_ms(30, displayUpdater);
}

uint8_t wheelval = 0;
void loop() {
   if (WiFi.status() != WL_CONNECTED) {
    logStatusMessage("WiFi lost!");
    WiFi.reconnect();
  }
  //Checking current transmit power
  //int txPower = WiFi.getTxPower();
  //Serial.print("Current transmit power: ");
  //Serial.println(txPower);

  if ( !client.connected() ) {
    logStatusMessage("MQTT lost");
    reconnect();
  } 
  client.loop();

  //WebSerial.println("Hello!");
  //delay(1000);

  // Periodically refresh NTP time
  if (millis() - lastNTPUpdate > 1000*NTP_REFRESH_INTERVAL_SEC) {
    logStatusMessage("NTP Refresh");
    configTime(TIMEZONE_DELTA_SEC, TIMEZONE_DST_SEC, "ro.pool.ntp.org");
    lastNTPUpdate = millis();
  }

  // Periodically refresh weather forecast
  if (millis() - lastWeatherUpdate > 1000 * WEATHER_REFRESH_INTERVAL_SEC) {
    logStatusMessage("Weather refresh");
    getAccuWeatherData();
    displayWeatherData();
    lastWeatherUpdate = millis();
  }

  //Do we need to clear the status message from the screen?
  if (logMessageActive) {
    if (millis() > messageDisplayMillis + LOG_MESSAGE_PERSISTENCE_MSEC) {
      clearStatusMessage();
      drawTestBitmap();
      CJBMessage("Go Team Chrob!!");
    }
  }

  // Do we have new sensor data?
  if (newSensorData) {
    //logStatusMessage("Sensor data in");
    displaySensorData();
    displayTodaysWeather();
  }
  // Do we have new train data?
  if (newTrainData) {
    //logStatusMessage("Sensor data in");
    displayTrainData();
  }
  //Do we have new calendar data?
    if (newCalendarData) {
    //logStatusMessage("Sensor data in");
    displayCalendarData();
  }
  //Do we have new flight data?
    if (newFlightNumber) {
    //logStatusMessage("Sensor data in");
    displayFlightNumber();
    displayFlightDestination();
  }
    //Do we have new flight data?
    if (newFlightDestination) {
    //logStatusMessage("Sensor data in");
    displayFlightDestination();
  }
  // Is the sensor data too old?
  if (millis() - lastSensorRead > 1000*SENSOR_DEAD_INTERVAL_SEC) {
    sensorDead = true;
    displaySensorData();
    displayTodaysWeather();
  }

  heartBeat = !heartBeat;
  /* drawHeartBeat();
  if (millis() - lastLightRead > 1000*LIGHT_READ_INTERVAL_SEC) {
    lightUpdate();
    //displayTodaysWeather();
  } */

  //Reset the watchdog timer as long as the main task is running
  esp_task_wdt_reset();
  delay(500);
}

void displayUpdater() {
  if(!getLocalTime(&timeinfo)){
    logStatusMessage("Failed to get time!");
    return;
  }

  unsigned long epoch = mktime(&timeinfo);
  if (epoch != prevEpoch) {
    displayClock();
    prevEpoch = epoch;
  }
}


//TODO: http://www.rinkydinkelectronics.com/t_imageconverter565.php

//TODO - add heartbeat in loop(), reboot in interrupt if heartbeat lost (sometimes the system hangs during OTA request)
//https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/

//TODO - get and print weather forecast every X interval (4h?)
//TODO - use light sensor data to set display brightness
//TODO - add option to turn off display via MQTT
//TODO - replace bitmap arrays with color565 values!
//TODO - add event-based wifi disconnect/reconnect - https://randomnerdtutorials.com/solved-reconnect-esp32-to-wifi/

//TODO - check asynchronously for buzzer stop
//TODO - move TSL read to async task
