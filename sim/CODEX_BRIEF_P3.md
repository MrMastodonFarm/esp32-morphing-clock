# Codex brief — panel simulator, Phase 3

Phases 0–2 passed review. This is **Phase 3** of `sim/SPEC.md`: the browser UI with
live controls, server-side re-render. All prior hard rules stand: zero edits under
`code/`, no `creds*` filenames, `vendor/` read-only, no network, no new dependencies
(Python stdlib + Pillow only), and `--p0` / default scene / `make check` must keep
passing.

## Mission

`sim/viewer.py` — a single-file Python web app (stdlib `http.server`) that lets a
browser edit the scenario and see the re-rendered panel.

## Behavior

- `GET /` — an HTML page with:
  - a form containing every scenario field (`time`, `temp`, `humidity`, `sensor_dead`,
    `trains`, `bluetrains`, `event`, `event_days`, `flight`, `flight_dest`, `weather`),
    prefilled from `scenarios/default.scn` on first load and from the last submission
    afterward;
  - the current render displayed at the top (`<img>`, pixelated CSS scaling so it stays
    crisp), with a checkbox to show the LED-look version instead;
  - links to whatever exists under `out/` (sweeps, morph GIF) as a simple gallery.
- `POST /render` — validate + write the submitted fields to `out/ui.scn`, run
  `./sim --scenario out/ui.scn --output out/ui.ppm`, then `render.py` on it (import
  render.py as a module rather than shelling out, if that's cleaner; LED variant too
  when the checkbox is set), and redirect back to `/`.
- Serve `out/` statically (PNG/GIF/PPM). Cache-bust the img URL (mtime query param) so
  the browser always shows the fresh render.
- Errors from the scenario parser (bad time, over-long flight, unknown key) must come
  back as a readable message on the page, not a stack trace or a 500.
- `--port N` flag, default 8099; bind `0.0.0.0`. Do not manage firewalls.
- No shell string interpolation anywhere — `subprocess.run` with argument lists only.
  Treat all form input as untrusted even though this is LAN-only.

## Constraints

- The sim binary already supports `--scenario` and `--output` (you built them in P2) —
  if `--output` doesn't exist for scenario mode, add it to `sim_main.cpp`.
- Re-render must NOT rebuild the binary — a render round trip should be well under
  10 s (SPEC acceptance); it's typically well under 1 s. If `./sim` is missing, say so
  in the page rather than trying to `make`.
- The existing plain `python3 -m http.server` on :8099 is running outside your sandbox;
  don't try to kill it. Test your server on a throwaway high port (e.g. 8098 bound to
  127.0.0.1) and shut it down when done — deployment on 8099 happens after review.

## Done when (test non-interactively)

- Start `python3 viewer.py --port 8098` in the background; then with curl:
  `GET /` returns the form; a `POST /render` with a changed field (e.g. `temp=42`)
  causes `out/ui.ppm` + `out/ui.png` to be (re)written and a subsequent `GET /` shows
  the updated form values; a POST with an invalid field (e.g. 6-char flight) returns
  the readable error. Kill the server afterward — leave no process running.
- `make check` still passes; `./sim --p0` still works.
- `git status` shows changes only under `sim/`.

## Report back

Usual: what you built, deviations, exact commands to run the viewer, and the curl
transcript proving the Done-when checks.
