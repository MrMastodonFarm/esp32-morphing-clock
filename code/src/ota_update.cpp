
#include <ESP32httpUpdate.h>
#include <esp_task_wdt.h>

#include "common.h"
#include "config.h"
#include "device_identity.h"
#include "ota_update.h"
#include "rgb_display.h"

// Pull the firmware over HTTP and flash it. Call this from loop(), never from
// mqtt_callback() - see the note at the MQTT_UPDATE_CMD_TOPIC branch in mqtt.cpp.
//
// ESPhttpUpdate.update() blocks for the whole download-and-flash, which means
// loop() never reaches its esp_task_wdt_reset(). Against the normal WDT_TIMEOUT
// (60s) that is a coin flip: a fast update squeaks in, a slow one gets the
// watchdog panic instead. Because panic reboots the device before the new slot
// is validated, the failure looks exactly like a successful reboot onto the OLD
// firmware - no error on the panel, nothing in the log. That bit us on
// 2026-08-07 and cost an evening.
//
// So widen the watchdog to OTA_WDT_TIMEOUT for the duration and restore it after.
// Widening beats esp_task_wdt_delete(NULL): a genuinely hung download still gets
// caught eventually rather than wedging the clock forever.
void perform_update() {
  //Warn before performing update
  Serial.print("Starting OTA update from: ");
  Serial.println(OTA_URL);
  logStatusMessage("OTA Requested!");

  delay(500);  // Brief delay before starting update

  esp_task_wdt_init(OTA_WDT_TIMEOUT, true);
  esp_task_wdt_reset();

  t_httpUpdate_return ret = ESPhttpUpdate.update(OTA_URL);

  // Only reached if the update did NOT succeed - a successful update reboots.
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_reset();

  switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        logStatusMessage("Update failed!");
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        logStatusMessage("No updates!");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        logStatusMessage("Update OK!");
        break;
  }
}
