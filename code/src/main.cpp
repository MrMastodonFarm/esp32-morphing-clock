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

#include <esp_task_wdt.h>
#include <esp_wifi.h>

#include "main.h"
#include "common.h"
#include "rgb_display.h"
#include "mqtt.h"
#include "creds_mqtt.h"
#include "clock.h"
#include "weather.h"

unsigned long prevEpoch;
unsigned long lastNTPUpdate;
unsigned long lastWeatherUpdate;
unsigned long lastDisplayUpdate;

//Just a blinking heart to show the main thread is still alive...
bool blinkOn;

// WiFi event handler for automatic reconnection
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected, reconnecting...");
      WiFi.reconnect();
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi reconnected");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi got IP: ");
      Serial.println(WiFi.localIP());
      break;
    default:
      break;
  }
}

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

  // Register WiFi event handler for automatic reconnection
  WiFi.onEvent(WiFiEvent);

  // Enable auto-reconnect
  WiFi.setAutoReconnect(true);

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

  // Boost TX power to maximum for better range
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.println("WiFi TX power set to 19.5 dBm");

  // Disable WiFi power saving for low latency
  WiFi.setSleep(false);
  Serial.println("WiFi power saving disabled");

  logStatusMessage("NTP time...");
  configTime(TIMEZONE_DELTA_SEC, TIMEZONE_DST_SEC, "pool.ntp.org");
  lastNTPUpdate = millis();
  logStatusMessage("NTP done!");

  logStatusMessage("Getting weather...");
  getOpenMeteoData();
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
  lastDisplayUpdate = millis();
}

uint8_t wheelval = 0;
void loop() {
  // Update display at regular intervals (replaces Ticker to avoid ISR context issues)
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
    displayUpdater();
    lastDisplayUpdate = millis();
  }

  // WiFi reconnection is handled by event callback (WiFiEvent)
  // Only reconnect MQTT if WiFi is connected
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    logStatusMessage("MQTT lost");
    reconnect();
  }
  client.loop();

  // Periodically refresh NTP time
  if (millis() - lastNTPUpdate > 1000*NTP_REFRESH_INTERVAL_SEC) {
    logStatusMessage("NTP Refresh");
    configTime(TIMEZONE_DELTA_SEC, TIMEZONE_DST_SEC, "ro.pool.ntp.org");
    lastNTPUpdate = millis();
  }

  // Periodically refresh weather forecast
  if (millis() - lastWeatherUpdate > 1000 * WEATHER_REFRESH_INTERVAL_SEC) {
    logStatusMessage("Weather refresh");
    getOpenMeteoData();
    yield();  // Allow WiFi/MQTT processing after HTTP request
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

  delay(100);  // Reduced from 500ms for more responsive MQTT
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
