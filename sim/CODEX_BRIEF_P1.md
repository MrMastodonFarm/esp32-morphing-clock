# Codex brief — panel simulator, Phase 1

Phase 0 passed review (your work; verified independently). This brief is **Phase 1 only**
of `sim/SPEC.md`: the full static scene, with the firmware's own drawing code compiled
verbatim. Read `SPEC.md` in full again before starting; `CODEX_BRIEF.md` (P0) still
applies wherever it doesn't conflict with this file.

## Mission

Compile these firmware files **unchanged** into the simulator and render one complete
default scene — every display section — to `out/scene.ppm`, plus a `render.py` that
turns PPMs into viewable PNGs:

```
code/src/common.cpp
code/src/clock.cpp
code/src/digit.cpp
code/src/rgb_display.cpp
code/src/weather.cpp
```

(`main.cpp`, `mqtt.cpp`, `ota_update.cpp`, `light_sensor.cpp`, `buzzer.cpp` stay out.)

## Hard rules (unchanged from P0)

1. **Zero edits under `code/`.** If a firmware file won't compile against your shims,
   fix the shim. If that's impossible without touching firmware, stop and report the
   exact error — do not work around it in `code/`.
2. No file named `creds*` anywhere.
3. `sim/vendor/` is read-only.
4. Plain g++ + make, no new dependencies, no network. Python may use Pillow (installed).
5. `./sim --p0` must still pass — don't break P0 while extending the shims.

## Known from P0 (apply these)

- The shim `Arduino.h` must pull in `<cstring>` — vendored Adafruit_GFX relies on the
  Arduino umbrella header for `strncpy`/`strlen`/`memset`.
- The `Print` extensions (`printf`, `print(const tm*, fmt)`) are already in the shim and
  are used for real this phase (`clock.cpp:98,101`, `weather.cpp`, `rgb_display.cpp`).

## New shims needed (headers included by the firmware files above)

Write only what these five files and their headers (`common.h`, `config.h`,
`rgb_display.h`, `weather.h`, `clock.h`, `digit.h`, `mqtt.h`) actually require —
compile-error-driven, minimal:

- `WiFi.h` — `WiFi.status()` **must return `WL_CONNECTED`** (`weather.cpp:623` silently
  skips the fetch otherwise — this is load-bearing, see SPEC "Stubs must not fail
  silently"); `WL_IDLE_STATUS` constant (`common.cpp:11`); a `WiFiClient` class
  (constructed in `common.cpp:9`).
- `PubSubClient.h` — class constructible from `WiFiClient&` (`common.cpp:14`); add
  methods only if the compiled files reference them.
- `ESPNtpClient.h` — likely empty or near-empty; it's included by `common.h`.
- `HTTPClient.h` — `begin(const char*)`, `GET()` → always 200, `getString()` → a
  `String` holding the fixture file's contents, `end()`. Expose a hook the driver calls
  first, e.g. `void sim_set_http_fixture(const char* path)`. Never simulate failure
  (SPEC: "Bound the weather retry").
- Extend the panel shim so `display_init()` (`rgb_display.cpp:24-38`) compiles and runs
  as written: the exact `HUB75_I2S_CFG` constructor call and members it uses, and
  `new MatrixPanel_I2S_DMA(mxconfig)` wiring the global `dma_display` to the framebuffer.
  The `Digit` class calls whatever GFX methods it calls — inherited, should just work.

Makefile: add `-Icode/include` (relative: `-I../code/include` or via VPATH from `sim/`)
and compile the five firmware files into `out/`. Firmware files may emit warnings —
tolerate them (like vendor); your own shims/driver stay warning-clean.

## The scene (driver rewrite of `sim_main.cpp`)

Default flow, mirroring the firmware's steady state (SPEC: "`sim_main` replicates
`setup()`'s draw order"):

1. `display_init()`
2. Set virtual state: `timeinfo` = **2026-08-07 19:42:00 (Friday)**; sensor globals:
   temp 88.5, humidity 61, trains 4/12/19/27, blue trains 2/9/16/24, event
   "Beach trip" in 12 days, flight AA123 → MI (5 chars max — the firmware buffer is
   `char[6]`); `sensorDead = false`;
   `sim_set_http_fixture("fixtures/open-meteo-summer.json")`.
3. `getOpenMeteoData()` — the real parser against the fixture.
4. `drawTestBitmap();` `displayWeatherData();`
5. The flag-driven renderers: `displaySensorData(); displayTodaysWeather();
   displayTrainData(); displayCalendarData(); displayFlightNumber();
   displayFlightDestination();`
6. `displayClock();` (leave `clockStartingUp` true so it takes the startup draw path)
7. Steady-state status area: emulate the post-timeout state — `clearStatusMessage()`
   then `drawTestBitmap()` and `CJBMessage("Go Team Chrob!!")`, matching
   `main.cpp:166-172`. Read `main.cpp` for ordering context only; do not compile it.
8. Write `out/scene.ppm`.

Keep `--p0` working; make the scene the default (no args) and/or `--scene`.

## Fixture

`sim/fixtures/open-meteo-summer.json` — hand-write a realistic Open-Meteo response
matching the URL in `weather.cpp:632-636`: a `daily` object with `weather_code`,
`temperature_2m_max`, `temperature_2m_min`, each 5 elements. Make the five days
visually distinct (e.g. codes 0, 2, 61, 95, 3; temps ~90s/70s). Keys must match what
the parser reads (`weather.cpp:678-689`).

## render.py

`python3 render.py` converts every `out/*.ppm` to `out/*.png`, nearest-neighbor
upscaled ×6, stable output names (`scene.ppm` → `scene.png`). Pillow only. The fancy
"LED look" is Phase 2 — don't build it yet.

## Done when

- `make clean && make` succeeds; `./sim` writes `out/scene.ppm`; `./sim --p0` still
  writes `out/p0.ppm`; `python3 render.py` produces PNGs.
- `out/scene.png` shows **every** section: clock digits + colon, day-of-week + date,
  today's 16×16 weather icon, today's min/max temp range, the 4-day forecast strip
  (8×8 icons + temps), outdoor temp/humidity, both train rows, the calendar line, the
  flight block, and the "Go Team Chrob!!" message.
- `git status` shows changes only under `sim/`.

## Report back

Same as P0: what you built, deviations and why, exact build/run commands, and — most
valuable — anything about the *firmware's* rendering you noticed while getting the scene
up (overlaps, off-by-ones, suspicious geometry). Do NOT fix firmware issues; list them.
