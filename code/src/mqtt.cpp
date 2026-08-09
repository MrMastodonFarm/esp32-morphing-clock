
#include <Arduino.h>
#include <PubSubClient.h>

#include "common.h"
#include "mqtt.h"
#include "device_identity.h"
#include "ota_update.h"

#include "rgb_display.h"

// Panel brightness is a real setting on a wall clock - room light changes, and it is the
// control that suppresses the green ghost (see PANEL_BRIGHTNESS in config.h), so being
// able to retune it without a reflash is worth the topic. Publish it RETAINED and it is
// reapplied on every reconnect; otherwise a reboot falls back to PANEL_BRIGHTNESS.
// Defined here off MQTT_TOPIC_PREFIX rather than in creds_mqtt.h so a fresh clone does
// not need another macro to compile.
#define MQTT_PANEL_BRIGHTNESS_TOPIC MQTT_TOPIC_PREFIX "/panel/brightness"

#ifdef PANEL_DIAG
// Latch blanking stays diagnostic-only - it is driver internals with no user-facing
// meaning, and sweeping it needs the static test pattern to judge against anyway.
#define MQTT_DIAG_LATCHBLANK_TOPIC MQTT_TOPIC_PREFIX "/diag/latchblank"
#endif

char mqtt_buffer[MQTT_BUFMAX];

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ( strcmp(topic, MQTT_TEMPERATURE_SENSOR_TOPIC) == 0) {
    payload[length]=0;
    sensorTemp = atof((char *)payload);
    lastSensorRead = millis();
    sensorDead = false;
    newSensorData = true;
  }
  if ( strcmp(topic, MQTT_HUMIDITY_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorHumi = atoi((char *)payload);
    lastSensorRead = millis();
    sensorDead = false;
    newSensorData = true;
  }

    if ( strcmp(topic, MQTT_TRAIN1_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorTrain1 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }

    if ( strcmp(topic, MQTT_TRAIN2_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorTrain2 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }
    if ( strcmp(topic, MQTT_TRAIN3_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorTrain3 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }   if ( strcmp(topic, MQTT_TRAIN4_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorTrain4 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }
    if ( strcmp(topic, MQTT_BLUE_TRAIN1_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorBlueTrain1 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }
    if ( strcmp(topic, MQTT_BLUE_TRAIN2_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorBlueTrain2 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }
    if ( strcmp(topic, MQTT_BLUE_TRAIN3_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorBlueTrain3 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }   
    if ( strcmp(topic, MQTT_BLUE_TRAIN4_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorBlueTrain4 = atoi((char *)payload);
    lastSensorRead = millis();
    newTrainData = true;
  }
    if ( strcmp(topic, MQTT_TEMPERATURE_SENSOR_TOPIC) == 0) {
    payload[length]=0;
    sensorTemp = atof((char *)payload);
    lastSensorRead = millis();
    sensorDead = false;
    newSensorData = true;
  }
  if ( strcmp(topic, MQTT_HUMIDITY_SENSOR_TOPIC) == 0) {
    payload[length] = 0;
    sensorHumi = atoi((char *)payload);
    lastSensorRead = millis();
    sensorDead = false;
    newSensorData = true;
  }
  if ( strcmp(topic, MQTT_NEXT_EVENT_SENSOR_TOPIC ) == 0) {
    strncpy(sensorNextEvent, (char*)payload, length);
    sensorNextEvent[length] = '\0';
    lastSensorRead = millis();
    newCalendarData = true;
  } 
  if ( strcmp(topic, MQTT_NEXT_EVENT_DAYS_TILL_SENSOR_TOPIC ) == 0) {
    payload[length] = 0;
    sensorDaysTillNextEvent = atoi((char *)payload);
    lastSensorRead = millis();
    newCalendarData = true; 
  } 
  if ( strcmp(topic, MQTT_FLIGHT_NUMBER_TOPIC ) == 0) {
    strncpy(sensorFlightNumber, (char*)payload, length);
    sensorFlightNumber[length] = '\0';
    lastSensorRead = millis();
    newFlightNumber = true;
  } 
  if ( strcmp(topic, MQTT_FLIGHT_DESTINATION_TOPIC ) == 0) {
    strncpy(sensorFlightDestination, (char*)payload, length);
    sensorFlightDestination[length] = '\0';
    lastSensorRead = millis();
    newFlightDestination = true;
  } 
  if ( strcmp(topic, MQTT_PANEL_BRIGHTNESS_TOPIC) == 0 ) {
    payload[length] = 0;
    int b = atoi((char *)payload);
    // Units are the library's row-width scale, not 0-255. Reject junk rather than
    // blanking the panel: atoi() returns 0 for any non-numeric payload, and 0 would
    // leave a wall clock dark with no way to see that anything is wrong.
    if (b >= 1 && b <= PANEL_WIDTH) {
      dma_display->setPanelBrightness(b);
      Serial.printf("Panel brightness -> %d\n", b);
    } else {
      Serial.printf("Ignoring out-of-range brightness '%s' (want 1..%d)\n",
                    (char *)payload, PANEL_WIDTH);
    }
  }

#ifdef PANEL_DIAG
  if ( strcmp(topic, MQTT_DIAG_LATCHBLANK_TOPIC) == 0 ) {
    payload[length] = 0;
    // setLatBlanking() clamps to MAX_LAT_BLANKING (4) and re-applies brightness itself,
    // and treats 0 as "back to the default", so no validation is needed here.
    uint8_t applied = dma_display->setLatBlanking(atoi((char *)payload));
    Serial.printf("[diag] latch_blanking -> %u\n", applied);
  }
#endif

    if ( strcmp(topic, MQTT_UPDATE_CMD_TOPIC)==0 ) {
    Serial.println("Starting update process...");
    // Start update if a 1 was received as first character
    updateValue = (char)payload[0]; // now-superfluous extra step
    if (updateValue == '1') {
      // Only raise a flag - loop() runs the update. Doing it here would block
      // inside PubSubClient's callback for the whole download, which starves the
      // MQTT keepalive and, worse, never reaches the esp_task_wdt_reset() at the
      // bottom of loop(): the watchdog then panics mid-flash and the device
      // reboots onto the OLD image. That is what happened on 2026-08-07.
      otaRequested = true;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to MQTT node...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD) ) {
      Serial.println( "[DONE]" );
      Serial.println("Subscribing to topics:");
      Serial.println(MQTT_UPDATE_CMD_TOPIC);
      Serial.println(MQTT_TEMPERATURE_SENSOR_TOPIC);
      Serial.println(MQTT_HUMIDITY_SENSOR_TOPIC);
      Serial.println(MQTT_TRAIN1_SENSOR_TOPIC);
      Serial.println(MQTT_TRAIN2_SENSOR_TOPIC);
      Serial.println(MQTT_TRAIN3_SENSOR_TOPIC);
      Serial.println(MQTT_TRAIN4_SENSOR_TOPIC);
      Serial.println(MQTT_BLUE_TRAIN1_SENSOR_TOPIC);
      Serial.println(MQTT_BLUE_TRAIN2_SENSOR_TOPIC);
      Serial.println(MQTT_BLUE_TRAIN3_SENSOR_TOPIC);
      Serial.println(MQTT_BLUE_TRAIN4_SENSOR_TOPIC);
      Serial.println(MQTT_NEXT_EVENT_SENSOR_TOPIC);
      Serial.println(MQTT_NEXT_EVENT_DAYS_TILL_SENSOR_TOPIC);
      Serial.println(MQTT_FLIGHT_NUMBER_TOPIC);
      Serial.println(MQTT_FLIGHT_DESTINATION_TOPIC);
      Serial.println("... done");

      client.subscribe(MQTT_UPDATE_CMD_TOPIC);
      client.subscribe(MQTT_TEMPERATURE_SENSOR_TOPIC);
      client.subscribe(MQTT_HUMIDITY_SENSOR_TOPIC);
      client.subscribe(MQTT_TRAIN1_SENSOR_TOPIC);
      client.subscribe(MQTT_TRAIN2_SENSOR_TOPIC);
      client.subscribe(MQTT_TRAIN3_SENSOR_TOPIC);
      client.subscribe(MQTT_TRAIN4_SENSOR_TOPIC);
      client.subscribe(MQTT_BLUE_TRAIN1_SENSOR_TOPIC);
      client.subscribe(MQTT_BLUE_TRAIN2_SENSOR_TOPIC);
      client.subscribe(MQTT_BLUE_TRAIN3_SENSOR_TOPIC);
      client.subscribe(MQTT_BLUE_TRAIN4_SENSOR_TOPIC);
      client.subscribe(MQTT_NEXT_EVENT_SENSOR_TOPIC);
      client.subscribe(MQTT_NEXT_EVENT_DAYS_TILL_SENSOR_TOPIC);
      client.subscribe(MQTT_FLIGHT_NUMBER_TOPIC);
      client.subscribe(MQTT_FLIGHT_DESTINATION_TOPIC);
      client.subscribe(MQTT_PANEL_BRIGHTNESS_TOPIC);
#ifdef PANEL_DIAG
      client.subscribe(MQTT_DIAG_LATCHBLANK_TOPIC);
      Serial.println("[diag] latch blanking topic subscribed");
#endif
    } else {
      logStatusMessage("Can't Stop Team Chrob!!"); //silly inside joke
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  } 
}
