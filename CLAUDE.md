# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based morphing digital clock on a HUB75 RGB LED matrix. Morphing 7-segment digits, date/day-of-week, weather forecast (Open-Meteo), moon phase at night, and live data pushed in over MQTT (outdoor temp/humidity, Metro train arrivals, next calendar event, last flight out of DCA).

**Two physical clocks are built from this one source tree** — a wide 128x64 and a square 64x64. See "Panel variants" below; the short version is that everything except geometry is shared, so a bug fix or feature lands on both with no porting.

This fork is a personalized version of bogd/esp32-morphing-clock — the train/flight/calendar display sections, the Open-Meteo switch (upstream used AccuWeather), and the moon phase are local additions. Upstream README/changelog describe the original, not this code.

Repo layout: `code/` (PlatformIO firmware — the only thing you build), `sim/` (host-side panel simulator with golden images), `pcb/` (Eagle schematics/gerbers for the matrix shield), `case/` (lasercut DWG), `photos/`.

## Build

All commands run from `code/`:

```bash
pio run                          # Build BOTH variants (there is no default_envs)
pio run -e 128x64                # Build just the wide clock
pio run -e 64x64                 # Build just the square clock
pio run -e 128x64 -t upload      # Build + flash over USB
pio device monitor               # Serial monitor (115200 baud)
```

There is no linter, and the only automated test is the simulator's golden-image check (`cd sim && make check` / `make VARIANT=64x64 check`). Beyond that, verification is "it compiles" plus watching the serial log / the panel. **Run `pio run` with no `-e` before pushing a shared change** — that is what catches a config macro added to one variant and not the other.

**The build fails on a fresh clone until you create `code/include/creds_mqtt.h`.** It is gitignored and *no `.sample` exists for it* (only the SSL certs have samples). It must define: `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_SERVER`, `MQTT_PORT`, `MQTT_USERNAME`, `MQTT_PASSWORD`, `MQTT_CLIENT_ID`, `OTA_URL`, and every topic macro consumed by `mqtt.cpp` (`MQTT_UPDATE_CMD_TOPIC`, `MQTT_TEMPERATURE_SENSOR_TOPIC`, `MQTT_HUMIDITY_SENSOR_TOPIC`, `MQTT_TRAIN1..4_SENSOR_TOPIC`, `MQTT_BLUE_TRAIN1..4_SENSOR_TOPIC`, `MQTT_NEXT_EVENT_SENSOR_TOPIC`, `MQTT_NEXT_EVENT_DAYS_TILL_SENSOR_TOPIC`, `MQTT_FLIGHT_NUMBER_TOPIC`, `MQTT_FLIGHT_DESTINATION_TOPIC`).

It must also define `MQTT_TOPIC_PREFIX` (the shared topic namespace) and, for the 64x64 build, `MQTT_CLIENT_ID_64X64`, `MQTT_UPDATE_CMD_TOPIC_64X64` and `OTA_URL_64X64` — see "Panel variants". `include/device_identity.h` `#error`s with instructions if the last three are missing, so you cannot accidentally build a 64x64 image carrying the 128x64's identity.

**Optional MQTT SSL:** uncomment `#define MQTT_USE_SSL` in `config.h` and create `include/{client.crt,client.key,server_mqtt.crt}.h` from the `.sample` files.

## Panel variants

Both clocks are built from one source tree, selected by a `-D` flag in `platformio.ini`. The two repos were merged on 2026-08-07: the square clock previously lived in `MrMastodonFarm/64x64-morphing-clock`, which forked from this history at `4b3ad58` (Feb 2024) and then missed twenty commits of shared work. Its history is preserved here — `git log 64x64-orig` — and that repo is now archived.

**What is per-variant** (and nothing else should become so without a good reason):

| File | Role |
|------|------|
| `include/config_128x64.h`, `include/config_64x64.h` | all geometry, colors, per-section fonts and text formats |
| `include/panel_pins_128x64.h`, `include/panel_pins_64x64.h` | HUB75 pin map (the 64x64 panel wants B and G swapped) |
| `src/weather_layout_128x64.cpp`, `src/weather_layout_64x64.cpp` | the five `display*()` weather functions; only one is compiled, via `build_src_filter` |

`include/config.h` holds every setting that is *not* geometry (timing, location, timezone, night hours, watchdog) and then includes the right variant header. It `#error`s if no variant flag is set, so a stray `gcc` invocation can't silently pick one.

**Everything else is shared, including `clock.cpp` and `rgb_display.cpp`.** Those two look panel-specific but are not: `clock.cpp` derives all six digit positions from `CLOCK_X`/`CLOCK_SEGMENT_*`/`PANEL_HEIGHT`, and `rgb_display.cpp`'s section renderers take their font, cursor and clear-rect from config macros. The recurring pattern is that the 64x64 uses TomThumb where the 128x64 uses the built-in font — and TomThumb is a GFX *custom* font, whose cursor Y is the **baseline** rather than the top-left corner. That single fact explains every `+5` cursor offset and `-5` clear rect in the 64x64 config (`DATE_CLEAR_Y`, `CALENDAR_CURSOR_Y`, `FLIGHT_*_CURSOR_Y`).

**Adding a display section:** put its geometry in *both* config headers, and drive font/cursor/clear from macros rather than literals. If the two layouts differ structurally rather than dimensionally — as the forecast does, a vertical column with temperatures on the 128x64 versus a bare horizontal icon strip on the 64x64 — that is the signal to put it in the per-variant layout files instead.

**Per-device identity.** Both clocks sit on the same broker and subscribe to the same sensor topics, so nearly all of `creds_mqtt.h` is shared. Three values must not be, and `include/device_identity.h` enforces it:

- `MQTT_CLIENT_ID` — two clients using one id knock each other off the broker in a reconnect loop
- `MQTT_UPDATE_CMD_TOPIC` — a shared trigger means one `1` publish flashes **both** clocks, and the 64x64 would pull the 128x64's binary
- `OTA_URL` — each variant has its own `firmware.bin`

The unsuffixed macros are treated as the 128x64's values (which is what they have always been), so that build needed no migration; the 64x64 requires explicit `*_64X64` overrides. Source files include `device_identity.h`, never `creds_mqtt.h` directly.

**The subtle part — `MQTT_TOPIC_PREFIX` vs `MQTT_CLIENT_ID`.** Every sensor topic in `creds_mqtt.h` must be defined as `MQTT_TOPIC_PREFIX "/sensor/..."`, *never* `MQTT_CLIENT_ID "/sensor/..."`. A macro's replacement list is expanded at its **use** site, so keying topics off the client id means `device_identity.h`'s `#undef MQTT_CLIENT_ID` silently repoints all of them at `MorphingClock64/sensor/...` — topics nothing publishes to. The 64x64 would compile cleanly, connect fine, and sit there with a blank panel. `creds_mqtt.h` was written that way originally and was split on 2026-08-07. To verify after touching that file, preprocess it for both variants and diff — the sensor topics must be identical and only client id / update topic / OTA URL may differ.

**What the 64x64 deliberately does not have:** sunrise/sunset (the 128x64 fits them only in the two gaps flanking its 16x16 icon; there is no equivalent slack) and native 16x16 weather icons (it pixel-doubles the 8x8 artwork via `drawWeatherIcon(..., enlarged=true)`; the 128x64 calls `drawWeatherIcon16()`). `displaySunTimes()` is a deliberate no-op in `weather_layout_64x64.cpp` rather than an `#ifdef` at the call site.

**Two inherited oddities in `config_64x64.h`,** carried over verbatim rather than silently "fixed": `SENSOR_DATA_X 65` and `HEARTBEAT_X 120` are both off the right edge of a 64px-wide panel. The outdoor temp/humidity block is therefore not visible on the square clock; the heartbeat is inert either way since `drawHeartBeat()` is not called from `loop()`.

## Deploying without USB

The intended path is **HTTP-pull OTA triggered over MQTT**: publish `1` to `MQTT_UPDATE_CMD_TOPIC`, and `ota_update.cpp` pulls the binary from `OTA_URL` via `ESPhttpUpdate`. Deployed `OTA_URL` for the wide clock is `http://192.168.0.51:8123/local/morphclock/firmware.bin`; the flow is `pio run -e <variant>`, `scp -P 2222` the `firmware.bin` to Home Assistant's `/config/www/morphclock*/`, then `mosquitto_pub` the trigger.

**This was silently broken until 2026-08-07 evening — read this before trusting an OTA.** `perform_update()` was called from inside `mqtt_callback()`, so the whole blocking download-and-flash ran without `loop()` ever reaching its `esp_task_wdt_reset()`. The 60s watchdog (`panic=true`) therefore raced the update, and when it won it rebooted the device *mid-flash*, before the new slot was validated — so the bootloader fell back to the old image. The failure is indistinguishable from success at a glance: the panel shows `OTA Requested!`, the clock reboots, and comes back running the OLD firmware with no error, because it never got far enough to print one. Earlier same-day runs that "worked" took ~30s, i.e. half the watchdog budget; the margin quietly vanished as the binary grew.

The fix is two-part: `mqtt_callback()` now only sets `otaRequested` and `loop()` runs the update, and `perform_update()` widens the watchdog to `OTA_WDT_TIMEOUT` (300s) for the duration and restores `WDT_TIMEOUT` after. Widening rather than `esp_task_wdt_delete(NULL)` so a genuinely hung download is still caught. A stale TODO at the bottom of `main.cpp` ("sometimes the system hangs during OTA request") was this bug, noticed years earlier and never traced.

**A device still running pre-fix firmware cannot receive this fix over OTA** — the broken path is the one doing the delivering. It needs one USB flash (`pio run -e <variant> -t upload`), which also finally adopts the `min_spiffs` table.

Partition-table subtleties learned the hard way: `platformio.ini` declared `huge_app.csv` (single app slot — OTA-impossible) from Feb 2024 (`4b3ad58`) until 2026-08-07, yet the deployed clock's OTA works — **the table on the device is whatever the last USB flash actually wrote**, and evidently that flash didn't use huge_app. `platformio.ini` now declares `min_spiffs.csv` (two 1.92MB slots), but the device's real slots are probably the 1.28MB stock-default table. **Keep `firmware.bin` under ~1.25MB** (currently 0.98MB) until a USB flash adopts min_spiffs — pio's size check trusts the declared table and would happily pass a build too big for the device's actual slot. Expect a few minutes of MQTT reconnect churn after an OTA reboot (observed after the first flash; a subsequent reboot cleared it).

**`code/ota_push.sh <variant> [--dry-run]` wraps the whole flow in one command**: build → size guard (aborts at ≥1.25MB, see below) → md5 printout → scp to HA → MQTT trigger. The variant (`128x64` or `64x64`) is a **required** argument rather than a defaulted one — pushing one clock's binary to the other is a bricking-class mistake, so the target is always named. `--dry-run` does the build and guard but only prints the deploy commands.

It sources `creds_ota.sh` (gitignored; create from `creds_ota.sh.sample`). `HA_SSH_*`, `MQTT_HOST/USER/PASS/PAYLOAD` are shared; `HA_WWW_DIR` and `MQTT_TOPIC` are looked up per variant (`HA_WWW_DIR_64X64`, `MQTT_TOPIC_128X64`, …), with the unsuffixed names accepted as a fallback for 128x64 only. The older `code/ota_build.sh` automates a docker-nginx variant of the same idea, takes the same variant argument, and shares the creds file.

**espota (`ArduinoOTA`) is gone on purpose; MQTT pull is the only wireless path.** Commit `0665892` removed the `ArduinoOTA` handler and mDNS from `main.cpp` during WiFi-stability debugging (standing UDP listener + mDNS overhead), keeping the MQTT-triggered pull as the deliberate update mechanism; the now-dead `[env:ota]` block was deleted from `platformio.ini` on 2026-08-07. (WebSerial/AsyncWebServer were removed separately in `7c1e23b` for ~50KB RAM and the ISR/CPU-load problems — don't reintroduce ArduinoOTA or WebSerial without accounting for those costs.)

## Architecture

**Everything is cooperative in `loop()`; there are no tasks or ISRs.** This is deliberate. A 30ms `Ticker` used to drive the clock animation and caused visual glitches and WiFi/MQTT starvation; `loop()` now polls `displayUpdater()` every `DISPLAY_UPDATE_INTERVAL_MS` (100ms) and ends with `delay(100)`. Anything blocking that you add to `loop()` shows up directly as dropped MQTT messages and a stuttering morph. `esp_task_wdt_reset()` is called once per iteration against a 60s watchdog — keep any long operation (notably OTA) well under that.

**Flag-driven repaint.** `mqtt_callback()` only parses the payload into a global in `common.h` and sets a flag (`newSensorData`, `newTrainData`, `newCalendarData`, `newFlightNumber`, `newFlightDestination`, `otaRequested`); `loop()` sees the flag and calls the matching `display*()` function so only that region of the panel is redrawn. Drawing from the callback is what you're avoiding. **Each renderer clears its own flag as its last act** — a new display section must do the same, or it repaints every iteration. The one deliberate exception is the `sensorDead` branch of `displaySensorData()`, which has no flag guard and does repaint continuously once the sensor goes stale. `otaRequested` follows the same discipline for a non-drawing reason: running the OTA in the callback blocked PubSubClient for the whole download and skipped the watchdog feed — see "Deploying without USB".

**Clock rendering.** `clock.cpp` holds six `Digit` objects (`digit0`..`digit5`, laid out right-to-left) and repaints only on a minute/hour change; `Digit::Morph*()` in `digit.cpp` animates one old→new digit transition, blocking for `CLOCK_ANIMATION_DELAY_MSEC` per animation step. The seconds digits are commented out. **The `Digit` class uses a bottom-origin Y axis** (inherited from HariFun's original) while everything else on the panel is top-origin — hence the `PANEL_HEIGHT-CLOCK_Y-...` arithmetic in `clock.cpp`. Time is displayed 12-hour without AM/PM, and the leading hour digit is only drawn when the hour is ≥ 10.

**Display helpers** split three ways. `rgb_display.cpp` has matrix init, the status/log line, and the sensor/train/calendar/flight sections — all shared, all parameterized by config macros. `weather.cpp` has the icon bitmaps and every panel-agnostic primitive: `color565()`, both `drawBitmap()` overloads, `drawWeatherIcon()`/`drawWeatherIcon16()`, `drawMoonPhase()`, `getMoonPhase()`, `formatSunTime()`, `wmoWeatherCodeMapping()` and `getOpenMeteoData()`. The five `display*()` weather-layout functions live in `weather_layout_<variant>.cpp`. `rgb_display.h` and `weather.h` still declare overlapping prototypes.

**Weather.** `getOpenMeteoData()` hits `http://api.open-meteo.com/v1/forecast` (plain HTTP, 5-day daily block, Fahrenheit, hardcoded `America/New_York` timezone in the URL), parses with ArduinoJson into `forecast5Days[5]` / `minTemp[]` / `maxTemp[]` / `sunriseToday` / `sunsetToday`. Sunrise/sunset come back as local-time ISO strings (`2026-08-07T06:15`) because the request pins the timezone, so `formatSunTime()` just lifts `HH:MM` from offset 11 and converts to 12-hour. The response is 764 bytes and parses to 2087 of the 4096-byte `DynamicJsonDocument` (measured on 64-bit, where ArduinoJson slots are larger than on the ESP32) — adding more daily fields is possible but check that number. `wmoWeatherCodeMapping()` collapses WMO codes into six internal icon ids (0 sun, 1 cloud, 2 showers, 3 rain, 4 storm, 5 snow). On failure it *recurses* after a 5s delay once `failCount > 3` — be careful adding to that path.

**Icons are `uint32_t` RGB arrays** in `weather.cpp`, converted per-pixel via `color565()`. Two entry points: `drawWeatherIcon()` uses the 8x8 artwork and its `enlarged` flag pixel-doubles it, while `drawWeatherIcon16()` uses the native 16x16 arrays. The 128x64 calls `drawWeatherIcon16()` for today and `drawWeatherIcon(..., false)` for the forecast column; the 64x64 uses the 8x8 path for both, pixel-doubling today's. The moon is always 8x8 pixel-doubled.

**Sunrise/sunset** (`displaySunTimes()`, 128x64 only — a no-op on the 64x64) flank the big weather icon — rise on its left, set on its right — in the only two gaps the layout has left. There is no slack: 13px is exactly one TomThumb `H:MM`, the clock's lower-right segment reaches x=52 on digits like 0/6/8, and `displayWeatherForecast()` clears from x=98. Those gaps exist only because the seconds digits are commented out; `digit1`/`digit0` would sit at x=59 and x=72. Verified against a 24-hour sweep and worst-case minute digits in the simulator.

**Moon phase.** Between `NIGHT_START_HOUR` and `NIGHT_END_HOUR`, the big "today" icon is replaced by one of eight 8x8 moon bitmaps, on both variants. `getMoonPhase()` computes a Julian day from `timeinfo` and takes the synodic-month remainder from the 2000-01-06 new moon — no network involved.

## Configuration

**`include/config.h`** — everything that is not geometry: timing (`NTP_REFRESH_INTERVAL_SEC`, `WEATHER_REFRESH_INTERVAL_SEC`, `SENSOR_DEAD_INTERVAL_SEC`, `DISPLAY_UPDATE_INTERVAL_MS`, `WDT_TIMEOUT`), location (`WEATHER_LATITUDE`/`LONGITUDE`, currently Alexandria VA), timezone (`TIMEZONE_DELTA_SEC`, `TIMEZONE_DST_SEC`), night hours, and the status-line row. It then includes the variant header.

**`include/config_128x64.h` / `include/config_64x64.h`** — all display geometry (`*_X`, `*_Y`, `*_WIDTH`, `*_HEIGHT` per section), colors as inline color565 expressions, and the per-section font/cursor/format macros. Layout changes belong here, not in the drawing code — and a new macro must be added to **both** files or the other variant stops compiling.

**`include/panel_pins_128x64.h` / `include/panel_pins_64x64.h`** — HUB75 pin mapping per board (alternate mappings are commented out in each). This is *not* the library default; changing boards means editing these. `rgb_display.h` picks the right one from the variant flag.

Timezone is set two places that must agree: `TIMEZONE_*_SEC` for `configTime()` and the `timezone=America%2FNew_York` parameter inside the Open-Meteo URL in `weather.cpp`.

## MQTT topics

Actual topic strings live in `creds_mqtt.h`; the deployed convention is a `MorphingClock/` prefix. **Both clocks subscribe to the same sensor topics** — they show the same trains, weather and calendar — so only the update-request topic is per-device (see "Panel variants").

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
| `.../update/req` | `1` | Triggers the HTTP OTA pull (**per-device topic** — see "Panel variants") |

Any sensor message refreshes `lastSensorRead`; after `SENSOR_DEAD_INTERVAL_SEC` with nothing, `sensorDead` flips and the temp/humidity block renders in the error color. Payloads are copied into fixed-size buffers with no length check (`sensorNextEvent[65]`, `sensorFlightNumber[6]`, `sensorFlightDestination[3]`) — an over-long publish will smash memory.

## Hardware

- ESP32 dev board on the custom shield in `pcb/` (v0.3 is current; gerbers included)
- **Wide clock:** 128x64 HUB75 matrix (two chained 64x64, or one 128x64)
- **Square clock:** one 64x64 HUB75 matrix (a DC2512-style module, which needs B and G swapped relative to the wide clock's panel — hence the separate pin header)
- `min_spiffs.csv` partition table (OTA-capable; was `huge_app.csv` until 2026-08-07). The 64x64 repo still declared `huge_app.csv` — a single app slot, so OTA was impossible there — until the merge.
- TSL2591 light sensor and the buzzer are wired for but **disabled** — `light_sensor.cpp`/`buzzer.cpp` are effectively dead code, their call sites in `main.cpp` and their config blocks are commented out. The library dep for the TSL2591 is still in `platformio.ini`.

## Simulator

`sim/` builds the real rendering sources (`common`, `clock`, `digit`, `rgb_display`, `weather`, and the variant's layout file) against host shims for Adafruit_GFX and the HUB75 driver, then writes a PPM of the panel. It is the only automated test in the repo.

```bash
cd sim
make                       # build the 128x64 simulator (default)
make VARIANT=64x64         # build the square one
make check                 # render the scenarios and diff against goldens
make VARIANT=64x64 check
make golden                # re-bless goldens after an intended layout change
```

Build products and goldens are namespaced per variant (`out/<variant>/`, `golden/<variant>/`), and the binary is `sim-<variant>`, so the two never clobber each other. Scenarios in `sim/scenarios/*.scn` set the clock time, sensor values and weather fixture; changing the `time` line past `NIGHT_START_HOUR` is how you exercise the moon-phase path.
