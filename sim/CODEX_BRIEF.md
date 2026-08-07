# Codex brief — panel simulator, Phase 0

You are implementing **Phase 0 only** of the panel simulator specified in `sim/SPEC.md`.
Read that file first, in full. It is the contract; this brief is the work order.

## Mission

Prove the approach: get the shim headers + vendored Adafruit_GFX compiling natively with
g++, and render a filled rect, built-in-font text, and TomThumb text into a PPM file.

**P0 does NOT compile any firmware source yet.** It only proves the shim/GFX foundation
those files will later sit on. Do not touch `code/` at all in this phase.

## Hard rules (violating any of these fails the phase)

1. **Zero edits outside `sim/`.** Nothing under `code/` may change, ever. If a shim seems
   to require a firmware edit, stop and report instead — that is a design failure per the
   spec, not something to work around.
2. **Do not create any file whose name starts with `creds`** — the root `.gitignore`
   pattern `creds*` matches at any depth and will silently swallow it.
3. **Do not re-download or modify `sim/vendor/`.** It is already populated and pinned:
   `vendor/Adafruit-GFX/` (v1.10.6) and `vendor/ArduinoJson/` (6.x, header-only).
   Vendored files are compiled as-is; warnings from them are acceptable, edits are not.
4. Plain `g++` + `make`. No cmake, no PlatformIO, no new dependencies, no network.

## Environment

- Linux box, g++ 13.3, GNU make, python3 + Pillow available (Pillow not needed for P0).
- Everything happens in `sim/`. Build artifacts go to `sim/out/` (gitignored via
  `sim/.gitignore`, which you create).

## Deliverables

```
sim/
  Makefile               builds ./sim from sim_main.cpp + shims + vendored GFX core
  .gitignore             out/
  shims/
    Arduino.h            see surface list below
    Print.h              base class for Serial and (via Arduino.h) Adafruit_GFX
    pgmspace.h           PROGMEM/pgm_read_* no-op macros for host
    ESP32-HUB75-MatrixPanel-I2S-DMA.h   the framebuffer panel shim
  sim_main.cpp           P0 driver: draw rect + text, write out/p0.ppm
```

Only shim what P0 needs. `WiFi.h`, `HTTPClient.h`, `PubSubClient.h`, `ESPNtpClient.h`,
`esp_task_wdt.h` are Phase 1+ — do not write them yet.

## Shim surface (from SPEC.md "Why this is cheap", already verified against source)

- `Arduino.h`: `millis()` (virtual-time counter, starts at 0, advanced by `delay()`),
  `delay(ms)`, `yield()`, `pinMode` no-op, `String` **backed by std::string** exposing at
  least `c_str()` and `length()`, integer typedefs (`byte`, `uint8_t`… via <cstdint>),
  `min`/`max` if needed, and a `Serial` object (an instance of your `Print` writing to
  stdout).
- `Print.h`: `write(uint8_t)` core; `print`/`println` overloads for
  `const char*`, `String`, `char`, `int`, `long`, `unsigned`, `float`, `double`,
  and the `__FlashStringHelper*`/`F()` path; **plus the two ESP32-core extensions**:
  `printf(fmt, ...)` and the strftime-style `print(const struct tm*, const char*)` /
  `println(const struct tm*, const char*)`. These extensions are load-bearing —
  `clock.cpp` calls both in Phase 1.
- `pgmspace.h`: `PROGMEM` → empty, `pgm_read_byte/word/dword/ptr` → plain derefs,
  `PSTR`/`F` passthrough. TomThumb.h and glcdfont.c must compile against it.
- Panel shim `ESP32-HUB75-MatrixPanel-I2S-DMA.h`:
  - `class MatrixPanel_I2S_DMA : public Adafruit_GFX` with a `uint16_t fb[128*64]`
    framebuffer (RGB565), `drawPixel(x, y, color)` override writing it (bounds-checked),
    `begin()` returning true, `setPanelBrightness(int)` no-op, and a
    `writePPM(const char* path)` helper that emits binary P6 at 128×64, RGB565→RGB888.
  - A `HUB75_I2S_CFG` struct mirroring what `rgb_display.cpp:24-33` uses: nested
    `i2s_pins` aggregate of 14 int fields (R1,G1,B1,R2,G2,B2,A,B,C,D,E,LAT,OE,CLK), a
    constructor accepting (width, height, chain, pins) with defaults, a `clkphase` member,
    a `driver` member and `FM6124` enumerator. Constants `PANEL_WIDTH`/`PANEL_HEIGHT` come
    from the firmware's `config.h` later; for P0 hardcode 128×64 locally in sim_main.
    (Not exercised in P0 beyond compiling; keep it minimal but present.)

## GFX vendoring rule

Compile **only** `vendor/Adafruit-GFX/Adafruit_GFX.cpp` (which includes `glcdfont.c`)
plus your sources. Do not add `Adafruit_SPITFT.cpp` or anything else from the library to
the build — they drag in SPI/BusIO and will not build on host. Include paths:
`-Ishims -Ivendor/Adafruit-GFX -Ivendor/ArduinoJson/src` (ArduinoJson unused in P0 but
the -I proves the layout).

## P0 driver (`sim_main.cpp`)

Instantiate the panel shim, then:
1. `fillRect(2, 2, 40, 20, <a color from color565>)`
2. default font: `setCursor`, `setTextColor`, `print("12:34")`
3. `setFont(&TomThumb)`, print a second string
4. also exercise `printf("%3d/%3d", -12, 4)` and
   `print(&some_tm, "%a")` so the Print extensions are proven, not just present
5. `writePPM("out/p0.ppm")`

## Done when (the phase's acceptance test)

- `make` succeeds from a clean tree (`make clean && make`) with no errors; warnings from
  `vendor/` are tolerated, warnings from `shims/` and `sim_main.cpp` should be zero.
- `./sim --p0` (or plain `./sim`) writes `out/p0.ppm`, a valid 128×64 P6 file, containing
  the rect and legibly different text in both fonts.
- `git status` shows changes **only** under `sim/`.

## Report back

Finish with a short report: what compiled, any deviations from this brief and why, the
exact commands to build and render, and anything discovered that Phase 1 must know
(e.g., GFX methods that needed more shim surface than listed). If P0 cannot be made to
work within the rules above, say so plainly with the blocking error — do not bend rule 1
to force it.
