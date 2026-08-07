# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based morphing digital clock on a 128x64 HUB75 RGB LED matrix. Morphing 7-segment digits, date/day-of-week, weather forecast (Open-Meteo), moon phase at night, and live data pushed in over MQTT (outdoor temp/humidity, Metro train arrivals, next calendar event, last flight out of DCA).

This fork is a personalized version of bogd/esp32-morphing-clock — the train/flight/calendar display sections, the Open-Meteo switch (upstream used AccuWeather), and the moon phase are local additions. Upstream README/changelog describe the original, not this code.

Repo layout: `code/` (PlatformIO firmware — the only thing you build), `pcb/` (Eagle schematics/gerbers for the matrix shield), `case/` (lasercut DWG), `photos/`.

## Build

All commands run from `code/`:

```bash
pio run                    # Build
pio run --target upload     # Build + flash over USB
pio device monitor          # Serial monitor (115200 baud)
```

There are no tests and no linter — verification is "it compiles" plus watching the serial log / the panel.

**The build fails on a fresh clone until you create `code/include/creds_mqtt.h`.** It is gitignored and *no `.sample` exists for it* (only the SSL certs have samples). It must define: `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_SERVER`, `MQTT_PORT`, `MQTT_USERNAME`, `MQTT_PASSWORD`, `MQTT_CLIENT_ID`, `OTA_URL`, and every topic macro consumed by `mqtt.cpp` (`MQTT_UPDATE_CMD_TOPIC`, `MQTT_TEMPERATURE_SENSOR_TOPIC`, `MQTT_HUMIDITY_SENSOR_TOPIC`, `MQTT_TRAIN1..4_SENSOR_TOPIC`, `MQTT_BLUE_TRAIN1..4_SENSOR_TOPIC`, `MQTT_NEXT_EVENT_SENSOR_TOPIC`, `MQTT_NEXT_EVENT_DAYS_TILL_SENSOR_TOPIC`, `MQTT_FLIGHT_NUMBER_TOPIC`, `MQTT_FLIGHT_DESTINATION_TOPIC`).

**Optional MQTT SSL:** uncomment `#define MQTT_USE_SSL` in `config.h` and create `include/{client.crt,client.key,server_mqtt.crt}.h` from the `.sample` files.

## Deploying without USB

The working path is **HTTP-pull OTA triggered over MQTT**: publish `1` to `MQTT_UPDATE_CMD_TOPIC`, and `ota_update.cpp` pulls the binary from `OTA_URL` via `ESPhttpUpdate`. `code/ota_build.sh` automates the whole thing — builds, serves `firmware.bin` from a throwaway nginx container on :8080, publishes the trigger with `mosquitto_pub`, waits, tears the container down. It sources `creds_ota.sh` (create from `creds_ota.sh.sample`; note `MQTT_ESP_HOSTNAME` must match the filename in `OTA_URL`).

**`pio run -e ota --target upload` (espota) does not work.** The `[env:ota]` block survives in `platformio.ini`, but commit 7c1e23b stripped the `ArduinoOTA` handler out of `main.cpp`, so nothing is listening on 3232. Either re-add `ArduinoOTA.begin()` or use the MQTT path above. (The same commit removed WebSerial for RAM — don't reintroduce either without accounting for the ~50KB and the ISR/CPU-load problems that motivated the removal.)

## Architecture

**Everything is cooperative in `loop()`; there are no tasks or ISRs.** This is deliberate. A 30ms `Ticker` used to drive the clock animation and caused visual glitches and WiFi/MQTT starvation; `loop()` now polls `displayUpdater()` every `DISPLAY_UPDATE_INTERVAL_MS` (100ms) and ends with `delay(100)`. Anything blocking that you add to `loop()` shows up directly as dropped MQTT messages and a stuttering morph. `esp_task_wdt_reset()` is called once per iteration against a 60s watchdog — keep any long operation (notably OTA) well under that.

**Flag-driven repaint.** `mqtt_callback()` only parses the payload into a global in `common.h` and sets a flag (`newSensorData`, `newTrainData`, `newCalendarData`, `newFlightNumber`, `newFlightDestination`); `loop()` sees the flag and calls the matching `display*()` function so only that region of the panel is redrawn. Drawing from the callback is what you're avoiding. Note the flags are never cleared, so those sections repaint every iteration — a known wart, not a bug to "fix" casually since some redraws are relied on.

**Clock rendering.** `clock.cpp` holds six `Digit` objects (`digit0`..`digit5`, laid out right-to-left) and repaints only on a minute/hour change; `Digit::Morph*()` in `digit.cpp` animates one old→new digit transition, blocking for `CLOCK_ANIMATION_DELAY_MSEC` per animation step. The seconds digits are commented out. **The `Digit` class uses a bottom-origin Y axis** (inherited from HariFun's original) while everything else on the panel is top-origin — hence the `PANEL_HEIGHT-CLOCK_Y-...` arithmetic in `clock.cpp`. Time is displayed 12-hour without AM/PM, and the leading hour digit is only drawn when the hour is ≥ 10.

**Display helpers** live across `rgb_display.cpp` (matrix init, status/log line, sensor/train/calendar/flight sections) and `weather.cpp` (icons, forecast, temp range, moon). Both declare overlapping prototypes in their headers; the sensor/train/flight functions are actually defined in `weather.cpp`.

**Weather.** `getOpenMeteoData()` hits `http://api.open-meteo.com/v1/forecast` (plain HTTP, 5-day daily block, Fahrenheit, hardcoded `America/New_York` timezone in the URL), parses with ArduinoJson into `forecast5Days[5]` / `minTemp[]` / `maxTemp[]`. `wmoWeatherCodeMapping()` collapses WMO codes into six internal icon ids (0 sun, 1 cloud, 2 showers, 3 rain, 4 storm, 5 snow). On failure it *recurses* after a 5s delay once `failCount > 3` — be careful adding to that path.

**Icons are `uint32_t` RGB arrays** in `weather.cpp`, converted per-pixel via `color565()`. Today's icon is a native 16x16 array; the 4-day forecast strip uses 8x8. `drawBitmap(..., enlarged=true)` pixel-doubles an 8x8 — that path is now only used for the moon.

**Moon phase.** Between `NIGHT_START_HOUR` and `NIGHT_END_HOUR`, the big "today" icon is replaced by one of eight 8x8 moon bitmaps. `getMoonPhase()` computes a Julian day from `timeinfo` and takes the synodic-month remainder from the 2000-01-06 new moon — no network involved.

## Configuration

**`include/config.h`** — all display geometry (`*_X`, `*_Y`, `*_WIDTH`, `*_HEIGHT` per section), colors as inline color565 expressions, timing (`NTP_REFRESH_INTERVAL_SEC`, `WEATHER_REFRESH_INTERVAL_SEC`, `SENSOR_DEAD_INTERVAL_SEC`, `DISPLAY_UPDATE_INTERVAL_MS`, `WDT_TIMEOUT`), location (`WEATHER_LATITUDE`/`LONGITUDE`, currently Alexandria VA), timezone (`TIMEZONE_DELTA_SEC`, `TIMEZONE_DST_SEC`), and night hours. Layout changes belong here, not in the drawing code.

**`include/rgb_display.h`** — HUB75 pin mapping for the custom shield (an alternate mapping is commented out above it). This is *not* the library default; changing boards means editing this block.

Timezone is set two places that must agree: `TIMEZONE_*_SEC` for `configTime()` and the `timezone=America%2FNew_York` parameter inside the Open-Meteo URL in `weather.cpp`.

## MQTT topics

Actual topic strings live in `creds_mqtt.h`; the deployed convention is a `MorphingClock/` prefix.

| Topic | Payload | Effect |
|-------|---------|--------|
| `.../sensor/temperature` | float | Outdoor temp; also refreshes today's weather block |
| `.../sensor/humidity` | int | Outdoor humidity |
| `.../sensor/train1`..`train4` | int | Yellow line arrivals (minutes) |
| `.../sensor/bluetrain1`..`bluetrain4` | int | Blue line arrivals (minutes) |
| `.../sensor/vacationCalendarEvent` | string | Next event name (≤64 chars) |
| `.../sensor/vacationCalendarDaysTill` | int | Days until that event |
| `.../lastFlight/flightNumber` | string | Flight number (≤5 chars) |
| `.../lastFlight/destination` | string | 2-char destination code |
| `.../update/req` | `1` | Triggers the HTTP OTA pull |

Any sensor message refreshes `lastSensorRead`; after `SENSOR_DEAD_INTERVAL_SEC` with nothing, `sensorDead` flips and the temp/humidity block renders in the error color. Payloads are copied into fixed-size buffers with no length check (`sensorNextEvent[65]`, `sensorFlightNumber[6]`, `sensorFlightDestination[3]`) — an over-long publish will smash memory.

## Hardware

- ESP32 dev board on the custom shield in `pcb/` (v0.3 is current; gerbers included)
- 128x64 HUB75 matrix (two chained 64x64, or one 128x64), `huge_app.csv` partition table
- TSL2591 light sensor and the buzzer are wired for but **disabled** — `light_sensor.cpp`/`buzzer.cpp` are effectively dead code, their call sites in `main.cpp` and their config blocks are commented out. The library dep for the TSL2591 is still in `platformio.ini`.
