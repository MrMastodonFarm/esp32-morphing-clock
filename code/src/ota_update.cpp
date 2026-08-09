
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClient.h>
#include <esp_task_wdt.h>

#include "common.h"
#include "config.h"
#include "device_identity.h"
#include "ota_update.h"
#include "rgb_display.h"

// Pull the firmware over HTTP and flash it. Call this from loop(), never from
// mqtt_callback() - see the note at the MQTT_UPDATE_CMD_TOPIC branch in mqtt.cpp.
//
// This deliberately drives HTTPClient and Update directly instead of using
// ESPhttpUpdate. That library (and the core's HTTPUpdate, which shares the code)
// does a blind `delay(100)` after the GET and then hands the stream straight to
// Update.writeStream(), whose first act is `_verifyHeader(data.peek())`. Stream::peek()
// returns -1 when nothing is buffered yet; that lands in a uint8_t parameter as 0xFF,
// fails the != 0xE9 test, and aborts with UPDATE_ERROR_MAGIC_BYTE - reported as
// "Wrong Magic Byte", which reads like a corrupt download rather than a timing bug.
//
// Measured 2026-08-08: Home Assistant's time-to-first-byte is ~1.5ms, but this clock's
// own WiFi RTT averages 96ms and peaks near 400ms. The 100ms grace is therefore right on
// the edge and usually loses, which is why OTA failed identically every attempt while the
// staged binary verified byte-for-byte at the server every time.
//
// So: wait for the first byte to actually arrive, then flash. Everything else here is
// error reporting, because the previous failure mode taught us that a silent OTA is far
// more expensive than a noisy one.
//
// The watchdog is widened to OTA_WDT_TIMEOUT for the duration: writeStream() blocks for
// the whole download without returning to loop(), so the normal WDT_TIMEOUT would race it
// and - because a panic reboots before the new slot is validated - the failure would look
// exactly like a successful reboot onto the old image.

static void otaFail(const char *what, HTTPClient &http) {
  Serial.printf("OTA FAILED: %s\n", what);
  logStatusMessage("Update failed!");
  Update.abort();
  http.end();
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_reset();
}

void perform_update() {
  Serial.print("Starting OTA update from: ");
  Serial.println(OTA_URL);
  logStatusMessage("OTA Requested!");

  delay(500);  // let the status line paint before we block

  esp_task_wdt_init(OTA_WDT_TIMEOUT, true);
  esp_task_wdt_reset();

  HTTPClient http;
  WiFiClient net;
  if (!http.begin(net, OTA_URL)) {
    otaFail("http.begin() rejected the URL", http);
    return;
  }
  http.setTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setConnectTimeout(OTA_HTTP_TIMEOUT_MS);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("OTA FAILED: HTTP %d (%s)\n", code, http.errorToString(code).c_str());
    logStatusMessage("Update failed!");
    http.end();
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_reset();
    return;
  }

  int len = http.getSize();
  Serial.printf("OTA: HTTP 200, Content-Length %d\n", len);
  if (len <= 0) {
    otaFail("server gave no Content-Length; refusing a stream of unknown size", http);
    return;
  }

  WiFiClient *stream = http.getStreamPtr();

  // THE FIX: wait for the first body byte instead of assuming 100ms was enough.
  uint32_t waitStart = millis();
  while (stream->available() == 0) {
    if (millis() - waitStart > OTA_FIRST_BYTE_TIMEOUT_MS) {
      otaFail("no response body arrived before the first-byte timeout", http);
      return;
    }
    if (!net.connected()) {
      otaFail("server closed the connection before sending a body", http);
      return;
    }
    esp_task_wdt_reset();
    delay(10);
  }
  Serial.printf("OTA: first byte after %lums, %d buffered\n",
                (unsigned long)(millis() - waitStart), stream->available());

  if (!Update.begin(len)) {
    Serial.printf("OTA FAILED: Update.begin(%d) - %s\n", len, Update.errorString());
    logStatusMessage("Update failed!");
    http.end();
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_reset();
    return;
  }

  size_t written = Update.writeStream(*stream);
  if (written != (size_t)len) {
    Serial.printf("OTA FAILED: wrote %u of %d bytes - %s\n",
                  (unsigned)written, len, Update.errorString());
    otaFail("short write", http);
    return;
  }

  if (!Update.end()) {
    Serial.printf("OTA FAILED: Update.end() - %s\n", Update.errorString());
    otaFail("finalise failed", http);
    return;
  }
  if (!Update.isFinished()) {
    otaFail("Update.end() returned true but the update is not finished", http);
    return;
  }

  http.end();
  Serial.println("OTA OK - rebooting into the new image");
  logStatusMessage("Update OK!");
  delay(250);
  ESP.restart();
}
