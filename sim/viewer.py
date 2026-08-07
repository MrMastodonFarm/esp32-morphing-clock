#!/usr/bin/env python3
"""Browser UI for editing and rendering panel simulator scenarios."""

import argparse
import html
import mimetypes
import subprocess
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import parse_qs, quote, unquote, urlsplit

import render


ROOT = Path(__file__).resolve().parent
OUT = ROOT / "out"
FIXTURES = ROOT / "fixtures"
DEFAULT_SCENARIO = ROOT / "scenarios" / "default.scn"
UI_SCENARIO = OUT / "ui.scn"
UI_PPM = OUT / "ui.ppm"
SIMULATOR = ROOT / "sim"

SCENARIO_FIELDS = (
    "time",
    "temp",
    "humidity",
    "sensor_dead",
    "trains",
    "bluetrains",
    "event",
    "event_days",
    "flight",
    "flight_dest",
    "weather",
)
FIELD_LABELS = {
    "time": "Time",
    "temp": "Temperature",
    "humidity": "Humidity",
    "sensor_dead": "Sensor dead",
    "trains": "Trains",
    "bluetrains": "Blue trains",
    "event": "Event",
    "event_days": "Event days",
    "flight": "Flight",
    "flight_dest": "Flight destination",
    "weather": "Weather fixture",
}
STATIC_SUFFIXES = {".png", ".gif", ".ppm"}
MAX_REQUEST_BYTES = 64 * 1024
MAX_FIELD_CHARS = 4096


def read_scenario(path: Path) -> dict[str, str]:
    """Read the simple key/value scenario format used by sim_main.cpp."""
    values: dict[str, str] = {}
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split(maxsplit=1)
        if len(parts) != 2:
            raise ValueError(f"{path}:{line_number}: missing value")
        key, value = parts
        if key not in SCENARIO_FIELDS:
            raise ValueError(f"{path}:{line_number}: unknown key: {key}")
        values[key] = value

    missing = [key for key in SCENARIO_FIELDS if key not in values]
    if missing:
        raise ValueError(f"{path}: missing fields: {', '.join(missing)}")
    return values


def write_scenario(values: dict[str, str]) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    contents = "".join(f"{key:<13}{values[key]}\n" for key in SCENARIO_FIELDS)
    UI_SCENARIO.write_text(contents, encoding="utf-8")


def validate_submission(values: dict[str, str]) -> None:
    for key, value in values.items():
        if not value:
            raise ValueError(f"missing value for {key}")
        if len(value) > MAX_FIELD_CHARS:
            raise ValueError(f"value for {key} is too long")
        if any(character in value for character in "\r\n#"):
            raise ValueError(f"value for {key} contains scenario syntax characters")

    weather_path = (ROOT / values["weather"]).resolve()
    try:
        weather_path.relative_to(FIXTURES.resolve())
    except ValueError as error:
        raise ValueError("weather must name a file under fixtures/") from error
    if not weather_path.is_file():
        raise ValueError(f"weather fixture does not exist: {values['weather']}")


def simulator_error(result: subprocess.CompletedProcess[str]) -> str:
    diagnostic = result.stderr.strip() or result.stdout.strip()
    return diagnostic or f"simulator exited with status {result.returncode}"


def render_submission(values: dict[str, str], led: bool) -> None:
    validate_submission(values)
    write_scenario(values)
    if not SIMULATOR.is_file():
        raise RuntimeError("simulator binary ./sim is missing; run make before starting the viewer")

    try:
        result = subprocess.run(
            ["./sim", "--scenario", "out/ui.scn", "--output", "out/ui.ppm"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )
    except (OSError, subprocess.SubprocessError) as error:
        raise RuntimeError(f"could not run simulator: {error}") from error
    if result.returncode != 0:
        raise ValueError(simulator_error(result))

    try:
        render.flat_render(UI_PPM)
        if led:
            render.led_render(UI_PPM)
    except Exception as error:
        raise RuntimeError(f"could not create PNG render: {error}") from error


def gallery_files() -> list[Path]:
    if not OUT.is_dir():
        return []
    return sorted(
        path
        for path in OUT.rglob("*")
        if path.is_file() and path.suffix.lower() in STATIC_SUFFIXES
    )


class ViewerState:
    def __init__(self) -> None:
        self.values = read_scenario(DEFAULT_SCENARIO)
        self.led = False
        self.has_ui_render = False
        self.error: str | None = None

    def ensure_current_render(self) -> None:
        if (OUT / "scene.png").is_file():
            return
        try:
            render_submission(self.values, self.led)
        except (ValueError, RuntimeError) as error:
            self.error = str(error)
        else:
            self.has_ui_render = True


class ViewerHandler(BaseHTTPRequestHandler):
    server_version = "PanelViewer/1.0"

    @property
    def state(self) -> ViewerState:
        return self.server.state  # type: ignore[attr-defined]

    def send_bytes(
        self,
        body: bytes,
        content_type: str,
        status: HTTPStatus = HTTPStatus.OK,
        *,
        head_only: bool = False,
    ) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if not head_only:
            self.wfile.write(body)

    def send_html(
        self,
        page: str,
        status: HTTPStatus = HTTPStatus.OK,
        *,
        head_only: bool = False,
    ) -> None:
        self.send_bytes(
            page.encode("utf-8"),
            "text/html; charset=utf-8",
            status,
            head_only=head_only,
        )

    def do_GET(self) -> None:
        path = urlsplit(self.path).path
        if path == "/":
            self.send_html(self.page())
        elif path.startswith("/out/"):
            self.send_static(path)
        else:
            self.send_error(HTTPStatus.NOT_FOUND)

    def do_HEAD(self) -> None:
        path = urlsplit(self.path).path
        if path == "/":
            self.send_html(self.page(), head_only=True)
        elif path.startswith("/out/"):
            self.send_static(path, head_only=True)
        else:
            self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:
        if urlsplit(self.path).path != "/render":
            self.send_error(HTTPStatus.NOT_FOUND)
            return

        try:
            values, led = self.parse_form()
        except ValueError as error:
            self.state.error = str(error)
            self.send_html(self.page(), HTTPStatus.BAD_REQUEST)
            return

        self.state.values = values
        self.state.led = led
        try:
            render_submission(values, led)
        except (ValueError, RuntimeError) as error:
            self.state.error = str(error)
            self.send_html(self.page(), HTTPStatus.BAD_REQUEST)
            return

        self.state.has_ui_render = True
        self.state.error = None
        self.send_response(HTTPStatus.SEE_OTHER)
        self.send_header("Location", "/")
        self.send_header("Content-Length", "0")
        self.send_header("Cache-Control", "no-store")
        self.end_headers()

    def parse_form(self) -> tuple[dict[str, str], bool]:
        content_type = self.headers.get("Content-Type", "").split(";", 1)[0].strip()
        if content_type != "application/x-www-form-urlencoded":
            raise ValueError("render requires application/x-www-form-urlencoded data")
        try:
            content_length = int(self.headers.get("Content-Length", ""))
        except ValueError as error:
            raise ValueError("invalid Content-Length") from error
        if content_length < 0 or content_length > MAX_REQUEST_BYTES:
            raise ValueError("form submission is too large")

        try:
            body = self.rfile.read(content_length).decode("utf-8")
            submitted = parse_qs(body, keep_blank_values=True, max_num_fields=32)
        except (UnicodeDecodeError, ValueError) as error:
            raise ValueError(f"invalid form submission: {error}") from error

        allowed = set(SCENARIO_FIELDS) | {"led"}
        unknown = sorted(set(submitted) - allowed)
        if unknown:
            raise ValueError(f"unknown field: {', '.join(unknown)}")

        values: dict[str, str] = {}
        for key in SCENARIO_FIELDS:
            entries = submitted.get(key)
            if entries is None:
                raise ValueError(f"missing field: {key}")
            if len(entries) != 1:
                raise ValueError(f"field supplied more than once: {key}")
            values[key] = entries[0]

        led_entries = submitted.get("led", [])
        if len(led_entries) > 1 or (led_entries and led_entries[0] != "on"):
            raise ValueError("invalid LED-look checkbox value")
        return values, bool(led_entries)

    def send_static(self, request_path: str, *, head_only: bool = False) -> None:
        relative = unquote(request_path.removeprefix("/out/"))
        candidate = (OUT / relative).resolve()
        try:
            candidate.relative_to(OUT.resolve())
        except ValueError:
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        if not candidate.is_file() or candidate.suffix.lower() not in STATIC_SUFFIXES:
            self.send_error(HTTPStatus.NOT_FOUND)
            return

        content_type = mimetypes.guess_type(candidate.name)[0] or "application/octet-stream"
        self.send_bytes(candidate.read_bytes(), content_type, head_only=head_only)

    def current_image(self) -> Path | None:
        stem = "ui" if self.state.has_ui_render else "scene"
        suffix = "-led.png" if self.state.led else ".png"
        image_path = OUT / f"{stem}{suffix}"
        return image_path if image_path.is_file() else None

    def page(self) -> str:
        image_path = self.current_image()
        if image_path is None:
            image_html = '<p class="empty">No current render is available yet.</p>'
        else:
            relative = image_path.relative_to(OUT).as_posix()
            image_url = f"/out/{quote(relative, safe='/')}?v={image_path.stat().st_mtime_ns}"
            image_html = (
                f'<a href="{image_url}"><img class="panel" src="{image_url}" '
                'alt="Current panel render"></a>'
            )

        error_html = ""
        if self.state.error:
            error_html = (
                '<section class="error"><h2>Render error</h2><pre>'
                f"{html.escape(self.state.error)}</pre></section>"
            )

        fields_html = []
        for key in SCENARIO_FIELDS:
            value = html.escape(self.state.values.get(key, ""), quote=True)
            label = html.escape(FIELD_LABELS[key])
            fields_html.append(
                f'<label for="{key}">{label}</label>'
                f'<input id="{key}" name="{key}" value="{value}" required>'
            )

        gallery_html = []
        for path in gallery_files():
            relative = path.relative_to(OUT).as_posix()
            href = f"/out/{quote(relative, safe='/')}"
            gallery_html.append(f'<li><a href="{href}">{html.escape(relative)}</a></li>')
        gallery = "\n".join(gallery_html) or "<li>No generated files yet.</li>"
        led_checked = " checked" if self.state.led else ""

        return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Panel simulator</title>
  <style>
    :root {{ color-scheme: dark; font-family: system-ui, sans-serif; }}
    body {{ max-width: 1050px; margin: 0 auto; padding: 1.5rem; background: #11151b; color: #e9eef5; }}
    h1, h2 {{ margin-bottom: .65rem; }}
    .render {{ padding: 1rem; overflow-x: auto; background: #050608; border: 1px solid #39414d; border-radius: .5rem; }}
    .panel {{ display: block; width: min(100%, 768px); height: auto; image-rendering: pixelated; image-rendering: crisp-edges; }}
    .error {{ margin: 1rem 0; padding: .75rem 1rem; background: #3d151a; border: 1px solid #e36874; border-radius: .5rem; }}
    .error h2 {{ margin-top: 0; }}
    .error pre {{ white-space: pre-wrap; overflow-wrap: anywhere; }}
    form {{ display: grid; grid-template-columns: minmax(9rem, auto) minmax(16rem, 1fr); gap: .65rem 1rem; align-items: center; }}
    input {{ box-sizing: border-box; width: 100%; padding: .5rem; color: inherit; background: #202630; border: 1px solid #536071; border-radius: .25rem; }}
    .check {{ grid-column: 2; display: flex; gap: .5rem; align-items: center; }}
    .check input {{ width: auto; }}
    button {{ grid-column: 2; justify-self: start; padding: .6rem 1rem; font-weight: 700; cursor: pointer; }}
    .gallery {{ columns: 3 15rem; padding-left: 1.25rem; }}
    a {{ color: #7fc7ff; }}
    @media (max-width: 600px) {{ form {{ grid-template-columns: 1fr; }} .check, button {{ grid-column: 1; }} }}
  </style>
</head>
<body>
  <h1>Panel simulator</h1>
  <section class="render">{image_html}</section>
  {error_html}
  <h2>Scenario</h2>
  <form method="post" action="/render">
    {''.join(fields_html)}
    <label class="check"><input type="checkbox" name="led"{led_checked}> Show LED-look render</label>
    <button type="submit">Render</button>
  </form>
  <h2>Generated output</h2>
  <ul class="gallery">{gallery}</ul>
</body>
</html>
"""


class ViewerServer(HTTPServer):
    def __init__(self, address: tuple[str, int], state: ViewerState):
        super().__init__(address, ViewerHandler)
        self.state = state


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Serve the panel simulator web UI")
    parser.add_argument("--port", type=int, default=8099, help="HTTP port (default: 8099)")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    state = ViewerState()
    state.ensure_current_render()
    server = ViewerServer(("0.0.0.0", args.port), state)
    print(f"Panel simulator viewer: http://0.0.0.0:{args.port}/", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
