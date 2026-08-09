#ifndef DEVICE_IDENTITY_H
#define DEVICE_IDENTITY_H

// Per-device credential resolution.
//
// The two clocks sit on the same broker and consume the same sensor topics, so almost
// everything in creds_mqtt.h is shared. Three values must NOT be:
//
//   MQTT_CLIENT_ID         - two clients connecting with one id knock each other off
//                            the broker in a reconnect loop
//   MQTT_UPDATE_CMD_TOPIC  - a shared trigger topic makes one `1` publish flash BOTH
//                            clocks, and the 64x64 would pull the 128x64 binary
//   OTA_URL                - each variant has its own firmware.bin
//
// The unsuffixed macros in creds_mqtt.h are taken as the 128x64 values (which is what
// they have always been), so that build needs no migration. The 64x64 build requires
// explicit _64X64 overrides and fails loudly without them.
//
// TRAP: the sensor topic macros must hang off MQTT_TOPIC_PREFIX, never off
// MQTT_CLIENT_ID. A macro's replacement list is expanded at its *use* site, so if
// the topics were defined as MQTT_CLIENT_ID "/sensor/..." the #undef below would
// silently repoint every one of them at MorphingClock64/... - topics nothing
// publishes to - and the 64x64 panel would come up blank with no compile error.
// creds_mqtt.h originally did exactly that; it was split on 2026-08-07.

#include "creds_mqtt.h"
#include "config.h"

#if defined(PANEL_VARIANT_64X64)

  #if !defined(MQTT_CLIENT_ID_64X64) || !defined(MQTT_UPDATE_CMD_TOPIC_64X64) || !defined(OTA_URL_64X64)
    #error "The 64x64 build needs its own identity. Add MQTT_CLIENT_ID_64X64, MQTT_UPDATE_CMD_TOPIC_64X64 and OTA_URL_64X64 to include/creds_mqtt.h - they must differ from the unsuffixed 128x64 values. See CLAUDE.md 'Panel variants'."
  #endif

  #undef MQTT_CLIENT_ID
  #define MQTT_CLIENT_ID MQTT_CLIENT_ID_64X64

  #undef MQTT_UPDATE_CMD_TOPIC
  #define MQTT_UPDATE_CMD_TOPIC MQTT_UPDATE_CMD_TOPIC_64X64

  #undef OTA_URL
  #define OTA_URL OTA_URL_64X64

#endif

// Status/boot-announcement topic, keyed off MQTT_CLIENT_ID rather than
// MQTT_TOPIC_PREFIX. The prefix is deliberately shared - both clocks consume the same
// sensor data - but "which build am I running" is per-device, and on the shared prefix
// the two clocks would silently overwrite each other's announcement. Defined here, after
// the per-variant MQTT_CLIENT_ID is settled, so it resolves correctly for either build.
#undef MQTT_STATUS_TOPIC
#define MQTT_STATUS_TOPIC MQTT_CLIENT_ID "/state"

#endif
