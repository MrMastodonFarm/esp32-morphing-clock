
#include <Arduino.h>
//#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "common.h"
#include "mqtt.h"
#include "creds_mqtt.h"
#include "ota_update.h"

//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <WebSerial.h>

char mqtt_buffer[MQTT_BUFMAX];

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  //WebSerial.println("MQTT Message");

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
    if ( strcmp(topic, MQTT_UPDATE_CMD_TOPIC)==0 ) {
    Serial.println("Starting update process...");
    // Start update if a 1 was received as first character
    updateValue = (char)payload[0]; // now-superfluous extra step
    if (updateValue == '1') {
      perform_update();
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
