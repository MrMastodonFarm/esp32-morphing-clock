# Codex brief — panel simulator, Phase 2

Phases 0–1 passed review. This is **Phase 2** of `sim/SPEC.md`: scenario files, sweeps,
the morph GIF, the LED-look render, and golden-frame regression. Re-read `SPEC.md`
first. Prior briefs' hard rules all still apply: zero edits under `code/`, no `creds*`
filenames, `vendor/` read-only, no network, `--p0` and the default scene must keep
working.

One correction since your P1 run: the default flight number is now `AA123` — 5 chars is
the firmware buffer's maximum (`char sensorFlightNumber[6]`). Never feed longer values
through the normal path; see "stress" below.

## 1. Scenario files

Parse the SPEC's `.scn` format (whitespace-separated key/value lines, `#` comments):

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

`event` takes the rest of the line (spaces allowed). Parse `time` with `strptime`, then
`mktime` to fill `tm_wday`/`tm_yday`. Unknown keys = hard error. Missing keys fall back
to the current defaults. Ship `scenarios/default.scn` reproducing today's scene exactly
(byte-identical PPM to the hardcoded path).

CLI: `./sim --scenario scenarios/default.scn` (and plain `./sim` keeps rendering the
built-in defaults, which must equal default.scn's output).

## 2. Frame capture through `delay()` (needed for morph + is the SPEC's design)

Add to the shims: `sim_arm_capture(const char* dir)` / `sim_disarm_capture()`. While
armed, every `delay()` call snapshots the framebuffer to `<dir>/frame-NNNN.ppm`
(zero-padded, monotonically increasing). `delay()` keeps advancing virtual `millis()`
regardless.

## 3. Sweeps

- `./sim --sweep hour` — 24 frames, `out/sweep-hour/HH.ppm`, the default scene at
  HH:42 for HH 0..23. **Expect two of these frames to expose known firmware bugs**
  (midnight `0:00`; night-hours moon swap) — render faithfully, do not fix.
- `./sim --sweep moon` — 8 frames, `out/sweep-moon/N.ppm`: pick 8 dates across one
  synodic month (~3.7 days apart, verify each yields a distinct `getMoonPhase()` value
  0..7) at a night hour so the moon path triggers.
- `./sim --sweep icons` — all 6 internal icons: for each icon id, one frame with that
  id as today's 16×16 icon and in the 8×8 forecast strip. Setting `forecast5Days[]` /
  temps directly after fixture load is fine — the globals are the interface.
- `./sim --morph 09:59:50` — render the scene at the given time, then advance to the
  next minute with capture armed so `Digit::Morph*()` emits its animation frames into
  `out/morph/`. Also try `--morph 12:59:00` in your acceptance run — that's the
  suspected 12→1 rollover bug; report what the frames show, don't fix.

## 4. render.py additions

- Directory mode: convert every PPM under `out/` recursively (stable names).
- `--gif <dir> <output.gif>`: assemble a directory of frames into an animated GIF
  (Pillow), ~60 ms/frame, looped, ×6 upscale.
- `--led`: "LED look" pass — each panel pixel becomes a round dot with slight bloom on
  a dark grid (SPEC "LED look"). Apply to any PPM; name output `*-led.png`.

## 5. Stress scenario

`scenarios/stress.scn` + `fixtures/open-meteo-winter.json` (below-zero minima):
64-char event name, 3-digit train times, `-12`° temps, legal-max 5-char flight. The
winter fixture will show the `uint8_t minTemp[]` corruption (SPEC suspect #4b) —
expected, report the render. Do NOT test over-long flight/event payloads: inside the
sim process that's undefined behavior, not a simulation. Note it as untestable-here.

## 6. Golden regression

- `make golden` — re-renders `--p0`, default scene, and stress scene into `sim/golden/`
  (committed, small PPMs).
- `make check` — renders them fresh and byte-compares against `golden/`; nonzero exit
  on any mismatch, with a message naming the differing file. Run it twice in a row to
  prove determinism.

## Done when

- All four sweep/morph commands produce the described outputs; `render.py` converts
  them; a morph GIF animates; an `--led` render exists for the default scene.
- `./sim` (defaults) and `--scenario scenarios/default.scn` produce byte-identical PPMs.
- `make check` passes; `make clean && make && make check` also passes.
- `--p0` still works; `git status` shows changes only under `sim/`.

## Report back

Usual format: what you built, deviations, exact commands. Plus: what the hour sweep's
00:42 frame and the 12:59→13:00 morph frames actually show (the two suspected clock
bugs), and what the winter stress render does to the forecast strip.
