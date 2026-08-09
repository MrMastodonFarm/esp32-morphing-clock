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

#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>

#include "main.h"
#include "common.h"
#include "rgb_display.h"
#include "mqtt.h"
#include "device_identity.h"
#include "clock.h"
#include "weather.h"
#include "ota_update.h"
#include "panel_diag.h"

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

// Why this exists: twice on 2026-08-07/08 the question "which firmware is this clock
// actually running?" cost real time, once needing binaries byte-compared to answer it.
// The build stamp settles it in one serial line.
//
// The reset reason matters just as much. A failed OTA is invisible otherwise: the panel
// shows "OTA Requested!", the clock reboots on cue, and comes back on the OLD image with
// no error, because a watchdog panic never gets far enough to print one. ESP_RST_TASK_WDT
// vs ESP_RST_SW tells those apart immediately.
//
// NOTE: the ROM's own "rst:0x..." boot log is NOT available on this board. OE_PIN is
// GPIO15 (MTDO), an ESP32 strapping pin, and HUB75 output-enable is active-low - so the
// panel holds it low at every reset, which silences the ROM log on U0TXD. esp_reset_reason()
// is read from our own code after Serial.begin(), so strapping cannot suppress it.
static const char *resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:  return "POWERON (cold boot)";
    case ESP_RST_SW:       return "SW (ESP.restart - normal after a successful OTA)";
    case ESP_RST_PANIC:    return "PANIC (exception/abort)";
    case ESP_RST_INT_WDT:  return "INT_WDT (interrupt watchdog)";
    case ESP_RST_TASK_WDT: return "TASK_WDT (task watchdog - the OTA failure mode)";
    case ESP_RST_WDT:      return "WDT (other watchdog)";
    case ESP_RST_BROWNOUT: return "BROWNOUT (supply sagged)";
    case ESP_RST_DEEPSLEEP:return "DEEPSLEEP";
    case ESP_RST_EXT:      return "EXT (external reset pin)";
    case ESP_RST_SDIO:     return "SDIO";
    default:               return "UNKNOWN";
  }
}

static const char *bootReason = "UNSET";

void setup(){
  display_init();

  Serial.begin(115200);
  delay(10);

  bootReason = resetReasonName(esp_reset_reason());
  Serial.println();
  Serial.printf("=== MorphingClock %s | built %s %s ===\n",
                PANEL_VARIANT_NAME, __DATE__, __TIME__);
  Serial.printf("Reset reason: %s\n", bootReason);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);

  displayTest(300);
// Set ESP32 host name
  String hostname = MQTT_CLIENT_ID;  // per-variant, so the two clocks do not collide on the network
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

  // Announce what booted, retained, on a per-device topic. This makes "did that OTA
  // actually land?" answerable with one mosquitto_sub from anywhere - no serial cable,
  // no USB. That question cost hours on 2026-08-07/08, twice needing binaries
  // byte-compared to answer it, because a failed OTA and a successful one looked
  // identical from outside. Retained so a subscriber that connects later still sees it.
  {
    char boot[192];
#ifdef PANEL_DIAG
    // Say so loudly: this build shows a test pattern instead of the clock, and the
    // only way back is another OTA.
    const char *buildKind = " | PANEL DIAG BUILD";
#else
    const char *buildKind = "";
#endif

    // __DATE__/__TIME__ are baked in wherever they appear - here, in main.cpp - so they
    // only move when *this* file is recompiled. Edit any other translation unit and the
    // stamp is unchanged, which makes it useless as "which firmware is on there?"
    // exactly when you most need it (it reported an identical stamp across two different
    // diagnostic builds on 2026-08-09). The ELF SHA-256 comes from the app descriptor
    // the bootloader already stores and changes with any recompile at all.
    char elfsha[9] = "unknown";
    const esp_app_desc_t *desc = esp_ota_get_app_description();
    if (desc != NULL) {
      snprintf(elfsha, sizeof(elfsha), "%02x%02x%02x%02x", desc->app_elf_sha256[0],
               desc->app_elf_sha256[1], desc->app_elf_sha256[2], desc->app_elf_sha256[3]);
    }

    snprintf(boot, sizeof(boot), "online | %s%s | built %s %s | elf %s | reset: %s",
             PANEL_VARIANT_NAME, buildKind, __DATE__, __TIME__, elfsha, bootReason);
    if (client.publish(MQTT_STATUS_TOPIC, boot, true)) {
      Serial.printf("Announced on %s: %s\n", MQTT_STATUS_TOPIC, boot);
    } else {
      // Non-fatal: PubSubClient's default buffer is 256 bytes, so an over-long
      // announcement silently fails rather than truncating. Worth knowing about.
      Serial.println("WARNING: boot announcement publish failed");
    }
  }

  logStatusMessage("Setting up watchdog...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  logStatusMessage("Woof!");
 
  logStatusMessage(WiFi.localIP().toString());
  drawTestBitmap();
  displayWeatherData();
  
  CJBMessage(CJB_MESSAGE); //just a silly inside joke
  lastDisplayUpdate = millis();
}

uint8_t wheelval = 0;
void loop() {
#ifdef PANEL_DIAG
  // Diagnostic build: the panel shows only the test pattern, so nothing else draws.
  // MQTT, the OTA path and the watchdog feed are all still here on purpose - this
  // build has to be replaceable over the air, or recovering the clock means USB.
  panelDiagUpdate();

  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnect();
  }
  client.loop();

  if (otaRequested) {
    otaRequested = false;
    perform_update();
  }

  esp_task_wdt_reset();
  delay(100);
  return;
#endif

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

  // Run a requested OTA here, out of the MQTT callback, so the long blocking
  // download is not holding up PubSubClient - and so the watchdog handling in
  // perform_update() is the only thing standing between us and a 60s timeout.
  if (otaRequested) {
    otaRequested = false;
    perform_update();
  }

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
      CJBMessage(CJB_MESSAGE);
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
