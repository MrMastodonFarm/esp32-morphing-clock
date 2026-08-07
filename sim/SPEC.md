# Panel simulator — spec and plan

**Status:** approved 2026-08-07 after a source-verification review; implementation delegated to Codex (see `CODEX_BRIEF.md`), P0 first as go/no-go.

## Goal

Iterate on what the panel *looks like* without flashing hardware: layout geometry, colors, font fit, icon selection, morph animation. Edit a constant in `config.h`, run one command, look at a PNG.

Two consumers, both important:

* **Human** — the dev box is headless and reached over SSH, so output is rendered to image files and served over the LAN rather than drawn in a window.
* **Claude** — PNG output means the agent can open the rendered panel directly and check its own layout change, instead of asking for a flash-and-look round trip.

## Non-goals

Not an ESP32 emulator. The simulator says nothing about DMA refresh, flicker, panel gamma and real color rendition, WiFi/MQTT behavior, watchdog timing, or RAM headroom. Those still require the board. It is a *drawing* simulator.

## Core principle

**The simulator contains zero copies of firmware drawing code.** It compiles `code/src/*.cpp` unchanged and supplies fake headers for everything below them. Any change that requires editing firmware source to satisfy the simulator is a design failure — the simulator would then be testing itself, and would drift the moment the firmware moved.

Corollary: `sim/` is disposable. Deleting it must leave the firmware build untouched.

## Architecture

```
                sim_main.cpp  (scenario driver — replaces main.cpp/loop)
                      │
      ┌───────────────┼────────────────┐
      ▼               ▼                ▼
 clock.cpp       weather.cpp      rgb_display.cpp     ← firmware source,
 digit.cpp       common.cpp                             compiled verbatim
      │               │                │
      └───────────────┴────────────────┘
                      ▼
        MatrixPanel_I2S_DMA  (sim shim: Adafruit_GFX subclass)
                      ▼
              uint16_t fb[128*64]
                      ▼
              PPM  →  Python/Pillow  →  PNG / GIF / "LED look"
```

### Why this is cheap

The firmware touches exactly 14 methods on `dma_display`:

```
fillRect setFont setCursor setTextColor setTextWrap setTextSize
color565 print printf drawPixel fillCircle drawLine begin setPanelBrightness
```

All but `begin` and `setPanelBrightness` are inherited **Adafruit_GFX**, and the real `MatrixPanel_I2S_DMA` is itself an Adafruit_GFX subclass. So the panel shim is: subclass Adafruit_GFX, override `drawPixel()` to write a framebuffer, stub the other two. Text rendering comes out byte-identical because it is the same library drawing the same fonts.

The remaining Arduino surface used by the drawing files is `millis`, `delay`, `Serial.print*`, `String`, `pinMode`, `yield` — plus two `Print` methods that are **ESP32-core extensions, not standard Arduino**: `printf`, and the strftime-style `print(struct tm*, const char*)` that `displayDate()` uses (`clock.cpp:98`). Adafruit_GFX inherits from `Print`, so the shim `Print.h` must implement `write()`, the usual `print`/`println` overloads, and both extensions.

Time needs no shimming: `getLocalTime()`/`configTime()` are called only from `main.cpp`, which the simulator replaces. The scenario driver writes the global `struct tm timeinfo` directly.

Verified against source 2026-08-07: the 14-method list above is exact (grepped every `dma_display->` call); the incomplete-array definitions in `common.cpp` (`char sensorNextEvent[];` after the sized `extern`) compile cleanly on this box's g++ 13 (tested); the fork's icon arrays use no `PROGMEM`; the stream-parsing HTTP path exists only inside the fully commented-out `getOpenWeatherData()`, so the `HTTPClient` stub needs just `begin(url)` / `GET()` / `getString()` / `end()`.

### Layout

```
sim/
  SPEC.md                  this file
  Makefile                 plain g++; no PlatformIO, no cmake
  shims/
    Arduino.h Print.h pgmspace.h
    WiFi.h HTTPClient.h PubSubClient.h ESPNtpClient.h
    esp_task_wdt.h ESP32httpUpdate.h
    ESP32-HUB75-MatrixPanel-I2S-DMA.h    ← framebuffer panel + HUB75_I2S_CFG
    sim_topics.h                          ← MQTT topic macros (phase 4 only)
  vendor/
    Adafruit-GFX/          core files ONLY (see "Vendor only the GFX core" below), pinned
    ArduinoJson/           the real thing, header-only, 6.x (firmware uses DynamicJsonDocument)
  fixtures/
    open-meteo-*.json      canned API responses
  scenarios/
    *.scn                  scene definitions
  sim_main.cpp
  render.py                PPM → PNG / GIF / LED look
  out/                     generated; gitignored
```

### Firmware files compiled into the simulator

| File | In? | Note |
|---|---|---|
| `common.cpp` | yes | the globals every renderer reads |
| `rgb_display.cpp` | yes | needs `HUB75_I2S_CFG` mirrored in the shim (`i2s_pins`, `clkphase`, `driver`/`FM6124`) |
| `clock.cpp`, `digit.cpp` | yes | |
| `weather.cpp` | yes | needs the `HTTPClient` stub; ArduinoJson is real |
| `mqtt.cpp` | phase 4 | the only file that drags in topic macros |
| `main.cpp` | no | replaced by `sim_main.cpp` |
| `ota_update.cpp`, `light_sensor.cpp`, `buzzer.cpp` | no | unused or irrelevant |

Phase 1 needs **no credentials header at all** — `creds_mqtt.h` is included only by `main.cpp`, `mqtt.cpp` and `ota_update.cpp`, none of which are in the phase-1 build.

## Key design decisions

**Virtual time.** `millis()` returns a counter the simulator controls, never wall-clock. Runs are then bit-for-bit reproducible, which is what makes "re-render and diff the PNG" a usable check.

**Vendor only the GFX core.** `Adafruit_GFX.{h,cpp}`, `gfxfont.h`, `glcdfont.c`, and `Fonts/` (the firmware uses only the built-in 5×7 font and `TomThumb`). Do not compile the rest of the library repo — `Adafruit_SPITFT.cpp` drags in SPI/BusIO and won't build on host.

**Stubs must not fail silently.** `getOpenMeteoData()` returns early unless `WiFi.status() == WL_CONNECTED` (`weather.cpp:623`), so the WiFi stub must report connected or P1 renders an empty forecast with no error. The `String` shim is backed by `std::string` and exposes `c_str()`/`length()` so ArduinoJson's Arduino-String integration works against it.

**Bound the weather retry.** On failure with `failCount > 3`, `getOpenMeteoData()` `delay(5000)`s and *recurses* (`weather.cpp:669-673`) — against a stub that always fails, that recursion never terminates, and `delay()` is the frame hook. Failure simulation is out of scope: the `HTTPClient` stub always returns 200 with the fixture body. If failure injection is ever added, the stub must fail at most 3 times.

**`sim_main` replicates `setup()`'s draw order.** Sections overwrite each other with `fillRect`, so the faithful sequence is the firmware's own: `display_init()` → `drawTestBitmap()` → `displayWeatherData()` → the flag-driven `display*()` sections → `displayClock()`. A different order can produce a scene the hardware never shows.

**Golden-frame regression.** Because renders are bit-reproducible, `make check` re-renders each committed scenario and byte-compares against a golden PPM in `sim/golden/`. That turns "look at the PNG" into an automated layout-regression test — exactly what an agent iterating on `config.h` needs. (P2.)

**`delay()` is the frame hook.** `Digit::Morph*()` blocks on `delay(CLOCK_ANIMATION_DELAY_MSEC)` between animation steps. The shim's `delay()` advances virtual time and, when capture is armed, snapshots the framebuffer. The morph animation therefore falls out as a frame sequence with **no change to `digit.cpp`**.

**Real ArduinoJson against a canned response.** Stubbing only `HTTPClient` (return a fixture string) means `getOpenMeteoData()` and `wmoWeatherCodeMapping()` actually execute. Forecast parsing and icon selection get exercised, not just the drawing.

**C++ writes PPM; Python makes it pretty.** The C++ side emits P6 PPM — a dozen lines, no image library, no third-party C++ dependency beyond the two vendored headers. `render.py` (Pillow, already installed) does upscaling, the GIF, and the "LED look" pass. ffmpeg is available if the GIF path wants it.

**"LED look" render.** Beyond a flat nearest-neighbor upscale, an optional pass draws each pixel as a round dot with a slight bloom on a dark grid. Much closer to what the panel reads like behind the plexi, and it exposes contrast problems that flat pixels hide.

## Scenario definition

A scene is a small text file — everything a renderer reads:

```
time         2026-08-07 19:42:00
temp         88.5
humidity     61
sensor_dead  false
trains       4 12 19 27
bluetrains   2 9 16 24
event        Beach trip
event_days   12
flight       AA123
flight_dest  MI
weather      fixtures/open-meteo-summer.json
```

Plus generated sweeps, which is where most of the value is:

```
./sim --scenario scenarios/default.scn        # one frame
./sim --sweep hour                            # 24 frames, one per hour of the day
./sim --sweep moon                            # 8 frames, one per lunar phase
./sim --sweep icons                           # all 6 weather icons, 16x16 and 8x8
./sim --morph 09:59:50                        # animate a rollover to GIF
./sim --stress                                # long strings, 3-digit trains, and a
                                              # below-zero winter fixture (see suspect list)
```

## Viewing

`python3 -m http.server 8099 --bind 0.0.0.0` in `out/`, plus a UFW rule scoped to the LAN, gives `http://192.168.0.60:8099`. Stable output filenames mean a browser refresh shows the newest render.

Phase 3 replaces this with a small Python handler: a form of the scenario fields, submit re-runs `make && ./sim`, page auto-refreshes. Server-side re-render rather than WASM — Emscripten is not installed on this box, and a rebuild is fast enough that it isn't worth the toolchain.

## What to point it at first

Concrete things worth checking the moment phase 1 runs, in rough order of suspicion:

1. **The 12-hour tens digit.** `clock.cpp` skips drawing `digit5` at startup when the hour is single-digit, but the morph path calls `digit5.Morph(hh/10)` unconditionally — which morphs to a **0** rather than blanking when the hour drops below 10. The 12→1 rollover would then read `01:00`. There is a comment in the code hinting at exactly this ("doesn't suppress the zero when flipping over from 12"). The simulator settles it in one command.
2. **Midnight.** `displayClock()` maps only `hh >= 13` down to 12-hour; `hh = 0` is never mapped to 12, so midnight renders **`0:00`** instead of `12:00` — in both the startup and morph paths. `--sweep hour` shows it.
3. **Long calendar event names.** The bottom line is 128px of default 6px font, and `sensorNextEvent` holds 64 chars. Where does it truncate, and does it collide with anything?
4. **Negative temperatures**, two separate problems: (a) `printf("%3d/%3d  F")` field widths at `-12/  4` versus the manually-drawn degree dot at a fixed offset; (b) the 4-day strip stores temps as **`uint8_t`** (`minTemp[5]`/`maxTemp[5]`, `common.h:83-84`) while today's are `int8_t` — a −5° forecast day stores as 251 and the strip prints garbage.
5. **Three-digit train times**, which widen a row that was laid out for two.
6. **The night/day boundary** at `NIGHT_END_HOUR`, and every moon phase icon rendered back to back for comparison.

## Risks

| Risk | Mitigation |
|---|---|
| Adafruit_GFX version drift changes text metrics, so the simulator lies about whether text fits | Pin `platformio.ini` to `1.10.6` exactly and vendor the same; confirm with one Windows build. Practical risk is low — the firmware uses only the built-in 5×7 font and TomThumb, both unchanged in GFX for years |
| The HUB75 library changes its API and the shim no longer matches | Shim only what the firmware calls — 14 methods; breakage is a compile error, not silent |
| **This box has no PlatformIO**, so a simulator-driven change can't be verified to still build for ESP32 here | Never edit firmware source for the simulator's benefit; real builds stay on the Windows machine |
| The `delay()`-based timing model is fake | Fine for layout; never use the simulator to reason about performance or responsiveness |
| `.gitignore` pattern `creds*` matches at any depth and would silently swallow a simulator credentials header | Name it `sim/shims/sim_topics.h` (phase 4 only) |

## Phases

| | Scope | Done when | Effort |
|---|---|---|---|
| **P0** | Skeleton: shims compile, Adafruit_GFX builds natively | `make && ./sim --p0` writes `out/p0.ppm` showing a filled rect, built-in-font text, and TomThumb text, with zero edits under `code/` | ~30 min — de-risks everything below |
| **P1** | Full static scene: all display sections, `render.py`, LAN viewer | the default scenario renders every section (clock+date, today's icon, temp range, 4-day strip, sensor, trains, calendar, flight) in one PNG; viewer reachable at `http://192.168.0.60:8099` | ~half a day |
| **P2** | Scenario files, sweeps, morph GIF, LED-look render, golden checks | all four sweeps run; a rollover GIF animates; `make check` passes against committed goldens | ~2 h |
| **P3** | Browser UI with live controls | edit a field → submit → new render visible, round trip well under 10 s | ~half a day |
| **P4** | Compile `mqtt.cpp` against a fake broker; replay recorded MQTT traffic end to end | a recorded topic sequence reproduces the same frames as the equivalent scenario file | ~2 h |

P0 is the honest checkpoint: if Adafruit_GFX and the shims don't compile cleanly together in half an hour, the approach needs rethinking before more is spent on it.
