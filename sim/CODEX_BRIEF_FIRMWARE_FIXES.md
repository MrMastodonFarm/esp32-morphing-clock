# Codex brief — firmware fixes (clock + forecast temps)

This task is different from the sim phases: **you are now authorized to edit firmware**,
but only these four files, for exactly the three bugs below:

```
code/src/clock.cpp
code/src/digit.cpp
code/include/digit.h
code/include/common.h                     (declarations: uint8_t → int8_t)
code/src/weather.cpp lines 11-12 ONLY     (the minTemp/maxTemp definitions live here,
                                           not in common.cpp — nothing else in
                                           weather.cpp may change)
```

Everything else in `code/` stays untouched. `sim/` rules from prior briefs still apply
(vendor read-only, no `creds*`), and the simulator is your verification tool — every
fix must be proven with renders before you're done.

Background: all three bugs were confirmed by the simulator (see `out/sweep-hour/00.png`,
`out/morph/` from `--morph 12:59:00`, `out/stress.png`). `sim/SPEC.md` "What to point
it at first" items 1, 2, and 4b describe them.

## Bug 1 — midnight renders `0:00` instead of `12:00`

`displayClock()` (`code/src/clock.cpp:27-86`) converts 24h→12h with only
`if (hh >= 13) hh -= 12;` — hour 0 is never mapped to 12. Fix: also map `hh == 0` to
12, so both the startup path and the morph path see 1–12. (The duplicate conversion
inside the `hh != prevhh` block is dead code — you may remove it or leave it consistent,
your call, but don't let it re-break hour 0.)

## Bug 2 — 12→1 rollover leaves a morphed `0` in the tens digit (`01:00`)

The startup path skips drawing `digit5` when `hh <= 9`, but the morph path calls
`digit5.Morph(h1)` whenever the tens value changes — morphing `1→0` draws a zero
instead of blanking. Required behavior for the tens-of-hours digit:

- tens becomes 0 (12→1 rollover): the digit **disappears** (area filled black);
- tens becomes 1 from blank (9→10): a clean `1` **appears** — note `Morph(1)` from a
  never-drawn state is wrong too; use `Draw` when coming from blank;
- 10→11→12 keep morphing normally;
- the startup path and the morph path must agree on this state.

Suggested implementation (you may improve on it): add `void Hide();` to `Digit` — a
black `drawFillRect` over the digit's full bounding box (local coords: x `0..segWidth+1`,
y `0..segHeight*2+2`; note `drawFillRect`'s local y param is the TOP edge — see how
`DrawColon` uses it) — plus whatever visible/blank state tracking `displayClock()`
needs. Do not disturb the colon (it belongs to `digit3` and sits left of it).

While you're in there: the startup path never initializes `prevhh`/`prevmm`; today that
is benign by accident (values collide with the constructor defaults). Set them at
startup draw so the state machine starts consistent.

## Bug 3 — forecast strip corrupts negative temperatures

`code/include/common.h:83-84` declares `uint8_t minTemp[5]; uint8_t maxTemp[5];`
(today's `minTempToday`/`maxTempToday` are already `int8_t`). A −11°F day renders
as 245. Change both arrays to `int8_t` (declaration and the definitions in
`common.cpp`). Check every read site still behaves (printf promotion handles `%d`).
Fahrenheit fits int8_t for this use; don't widen further.

## Verification (all via the simulator)

1. `cd sim && make clean && make` — the firmware files recompile in the sim.
2. **Prove no collateral first**: run `make check` — `p0` and `default` goldens must
   still pass byte-identical (the 7:42 PM default scene exercises none of the fixed
   paths); `stress` is expected to differ (its 9:42 winter scene shows the temp fix).
   Report the check output.
3. Then `make golden` to refresh goldens, and `make check` twice to re-prove
   determinism.
4. Render the evidence, and study the PNGs yourself (don't just generate them):
   - `./sim --sweep hour` — frame 00 must read `12:42`; 01–09 single hour digit;
     10–12 double; 13–23 correct 12h values.
   - `./sim --morph 12:59:00` — ends at `1:00` with the tens position blank.
   - `./sim --morph 09:59:00` — ends at `10:00` with a clean leading `1`.
   - `./sim --morph 23:59:00` — 11:59 → `12:00`.
   - `./sim --morph 00:59:00` — 12:59 → `1:00` (midnight side).
   - `./sim --stress` — forecast strip shows the true negative minima (−11/−9/−15/−7).
     If three-character values crowd the icons, report it; do not redesign the layout.
   - `python3 render.py` afterward so PNGs exist for review.
5. `git status` — modified files must be exactly the four firmware files (plus
   regenerated `sim/golden/` and `sim/out/`); nothing else.

## Constraints

- This box cannot compile for ESP32 (no PlatformIO). Keep the changes conservative
  C++11-compatible Arduino style, matching the file's existing idiom — no `<algorithm>`,
  no auto, no structured bindings. The final hardware build happens elsewhere.
- Do not fix anything else you notice (the overwide fillRect, icon dedup, etc.) —
  report, don't touch.
- Do not update CLAUDE.md/README.md — docs are handled after review.

## Report back

Usual format, plus: the `make check` output from step 2 (collateral proof), what each
morph's final frame shows, and the exact diff summary (`git diff --stat -- code`).
