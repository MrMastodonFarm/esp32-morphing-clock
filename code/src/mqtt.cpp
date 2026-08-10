
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

// Parse a numeric payload, rejecting anything that is not wholly a number.
//
// Replaces bare atof()/atoi(), which return 0 for unparseable input and so cannot tell
// "the outdoor temperature is zero" from "Home Assistant said unavailable". Works on a
// local copy rather than NUL-terminating in place, so it never writes into
// PubSubClient's receive buffer.
static bool parseNumberPayload(const byte *payload, unsigned int length, float *out) {
  char text[24];
  if (length == 0 || length >= sizeof(text)) {
    return false;
  }
  memcpy(text, payload, length);
  text[length] = '\0';

  char *end = NULL;
  const double value = strtod(text, &end);
  if (end == text) {
    return false;  // nothing numeric at all - "unavailable", "unknown", ""
  }
  while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
    ++end;
  }
  if (*end != '\0') {
    return false;  // trailing junk, e.g. "72F"
  }
  *out = (float)value;
  return true;
}

// Bounded copy for the string topics.
//
// The previous strncpy(dest, payload, length) + dest[length] = 0 wrote past the end of
// the destination for any payload longer than it. That was not theoretical:
// sensorFlightDestination was 3 bytes and "unavailable" is 11, so one routine Home
// Assistant reload put a NUL nine bytes past the end of the array, into whatever global
// followed it.
//
// Rejects rather than truncates. A truncated "unavailable" would display as "un" and
// look like a real flight code; refusing leaves the last good value on screen.
//
// The flip side, learned the hard way: a destination buffer that is one byte too small
// for the *real* feed makes that field silently stop updating altogether, because a
// legitimate value is now indistinguishable from junk. Size the buffers in common.h to
// the real payload plus NUL - see the note there.
static bool copyPayload(char *dest, size_t destSize, const byte *payload,
                        unsigned int length) {
  if (length >= destSize) {
    return false;
  }
  memcpy(dest, payload, length);
  dest[length] = '\0';
  return true;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Every handler below refuses a payload it cannot parse, and - just as important -
  // does not stamp lastSensorRead when it refuses.
  //
  // Home Assistant publishes the literal strings "unavailable" and "unknown" whenever an
  // entity drops out, and an integration reload is enough to cause it (observed on
  // temperature and humidity on 2026-08-09). atoi()/atof() turn those into 0, so the
  // panel used to display a fabricated 0F while the freshness clock said the sensor was
  // perfectly healthy - and "0 0 0 0" on the train row reads as four trains arriving now.
  //
  // Refusing keeps the last good reading on screen. It also means a sustained outage now
  // ages into the dashed stale state on its own after SENSOR_DEAD_INTERVAL_SEC, because
  // nothing is refreshing lastSensorRead. That falls out of the change; it needed no
  // extra logic.
  float number = 0;

  if ( strcmp(topic, MQTT_TEMPERATURE_SENSOR_TOPIC) == 0) {
    if (parseNumberPayload(payload, length, &number)) {
      sensorTemp = number;
      lastSensorRead = millis();
      sensorDead = false;
      newSensorData = true;
    }
  }
  if ( strcmp(topic, MQTT_HUMIDITY_SENSOR_TOPIC) == 0) {
    // Parsed as a float and truncated rather than read with atoi(): humidity arrives as
    // "50.16", and a strict integer parse would reject every reading.
    if (parseNumberPayload(payload, length, &number)) {
      sensorHumi = (int)number;
      lastSensorRead = millis();
      sensorDead = false;
      newSensorData = true;
    }
  }

  // WeatherFlow's feels-like, which unlike a locally computed heat index also covers the
  // cold end (wind chill) and has wind and solar as inputs. Deliberately does NOT touch
  // lastSensorRead or sensorDead: it is a nice-to-have from a different upstream, and a
  // clock that declared its outdoor sensor alive on the strength of this arriving - while
  // temperature and humidity had actually stopped - would be lying.
  if ( strcmp(topic, MQTT_FEELS_LIKE_SENSOR_TOPIC) == 0) {
    if (parseNumberPayload(payload, length, &number)) {
      sensorFeelsLike = number;
      feelsLikeValid = true;
      lastFeelsLikeRead = millis();
      newSensorData = true;
    }
  }

  // Yellow line, then blue line. Same shape for all eight.
  {
    int *const trainTargets[] = {
        &sensorTrain1, &sensorTrain2, &sensorTrain3, &sensorTrain4,
        &sensorBlueTrain1, &sensorBlueTrain2, &sensorBlueTrain3, &sensorBlueTrain4};
    const char *const trainTopics[] = {
        MQTT_TRAIN1_SENSOR_TOPIC, MQTT_TRAIN2_SENSOR_TOPIC,
        MQTT_TRAIN3_SENSOR_TOPIC, MQTT_TRAIN4_SENSOR_TOPIC,
        MQTT_BLUE_TRAIN1_SENSOR_TOPIC, MQTT_BLUE_TRAIN2_SENSOR_TOPIC,
        MQTT_BLUE_TRAIN3_SENSOR_TOPIC, MQTT_BLUE_TRAIN4_SENSOR_TOPIC};
    for (unsigned int i = 0; i < sizeof(trainTopics) / sizeof(trainTopics[0]); ++i) {
      if (strcmp(topic, trainTopics[i]) == 0 &&
          parseNumberPayload(payload, length, &number)) {
        *trainTargets[i] = (int)number;
        lastSensorRead = millis();
        newTrainData = true;
      }
    }
  }

  if ( strcmp(topic, MQTT_NEXT_EVENT_SENSOR_TOPIC ) == 0) {
    if (copyPayload(sensorNextEvent, sizeof(sensorNextEvent), payload, length)) {
      lastSensorRead = millis();
      newCalendarData = true;
    }
  }
  if ( strcmp(topic, MQTT_NEXT_EVENT_DAYS_TILL_SENSOR_TOPIC ) == 0) {
    if (parseNumberPayload(payload, length, &number)) {
      sensorDaysTillNextEvent = (int)number;
      lastSensorRead = millis();
      newCalendarData = true;
    }
  }
  if ( strcmp(topic, MQTT_FLIGHT_NUMBER_TOPIC ) == 0) {
    if (copyPayload(sensorFlightNumber, sizeof(sensorFlightNumber), payload, length)) {
      lastSensorRead = millis();
      newFlightNumber = true;
    }
  }
  if ( strcmp(topic, MQTT_FLIGHT_DESTINATION_TOPIC ) == 0) {
    if (copyPayload(sensorFlightDestination, sizeof(sensorFlightDestination), payload,
                    length)) {
      lastSensorRead = millis();
      newFlightDestination = true;
    }
  }
  if ( strcmp(topic, MQTT_PANEL_BRIGHTNESS_TOPIC) == 0 ) {
    float requested = 0;
    const int b = parseNumberPayload(payload, length, &requested) ? (int)requested : -1;
    // Units are the library's row-width scale, not 0-255. Unparseable input lands as
    // -1 and is rejected below rather than reaching the panel: a 0 would leave a wall
    // clock dark with no indication why.
    if (b >= 1 && b <= PANEL_WIDTH) {
      dma_display->setPanelBrightness(b);
      Serial.printf("Panel brightness -> %d\n", b);
    } else {
      Serial.printf("Ignoring unparseable/out-of-range brightness (want 1..%d)\n",
                    PANEL_WIDTH);
    }
  }

#ifdef PANEL_DIAG
  if ( strcmp(topic, MQTT_DIAG_LATCHBLANK_TOPIC) == 0 ) {
    // setLatBlanking() clamps to MAX_LAT_BLANKING (4) and re-applies brightness itself,
    // and treats 0 as "back to the default", so an unparseable payload is harmless here
    // - it still goes through the parser to avoid writing into PubSubClient's buffer.
    float pulses = 0;
    parseNumberPayload(payload, length, &pulses);
    uint8_t applied = dma_display->setLatBlanking((uint8_t)pulses);
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
      Serial.println(MQTT_FEELS_LIKE_SENSOR_TOPIC);
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
      client.subscribe(MQTT_FEELS_LIKE_SENSOR_TOPIC);
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
