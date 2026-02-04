# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based morphing digital clock displayed on a 128x64 HUB75 RGB LED matrix. Features morphing 7-segment digit animations, date display, weather forecasts (via Open-Meteo API), and real-time data via MQTT (temperature/humidity sensors, train times, calendar events, flight info).

## Build Commands

All commands run from the `code/` directory using PlatformIO:

```bash
pio run                    # Build firmware
pio run --target upload    # Build and upload via USB
pio monitor                # Serial monitor (115200 baud)
```

**OTA Updates:** Trigger via MQTT message to `MorphingClock/update/req` with payload `1`. Requires firmware hosted at URL defined in `OTA_URL` (creds_mqtt.h).

## Architecture

**Event-driven display updates:** MQTT callbacks set flags (`newSensorData`, `newTrainData`, `newCalendarData`, `newFlightNumber`) in `common.h`. Main loop checks flags and redraws only changed display sections.

**Timing:** 30ms Ticker calls `displayUpdater()` for morphing clock animations. Main loop runs at 500ms intervals with watchdog reset (60s timeout).

**Key modules:**
- `main.cpp` - Setup, main loop, periodic NTP/weather refresh scheduling
- `clock.cpp` / `digit.cpp` - Morphing 7-segment digit rendering and animation
- `rgb_display.cpp` - HUB75 matrix abstraction (ESP32 DMA library)
- `mqtt.cpp` - All MQTT topic subscriptions and callback handlers
- `weather.cpp` - Open-Meteo API fetch, WMO weather code mapping to icons

**Display layout** is defined in `config.h` with `*_X`, `*_Y`, `*_WIDTH`, `*_HEIGHT` constants for each element (clock, date, sensors, weather, etc.).

## Configuration

**`include/config.h`:**
- Display positions, colors (color565 format), pin assignments
- Timing intervals: `NTP_REFRESH_INTERVAL_SEC`, `WEATHER_REFRESH_INTERVAL_SEC`, `SENSOR_DEAD_INTERVAL_SEC`
- Weather location: `WEATHER_LATITUDE`, `WEATHER_LONGITUDE`
- Timezone: `TIMEZONE_DELTA_SEC`, `TIMEZONE_DST_SEC`

**`include/creds_mqtt.h`:** (create from existing file, update values)
- WiFi: `WIFI_SSID`, `WIFI_PASSWORD`
- MQTT: `MQTT_SERVER`, `MQTT_PORT`, `MQTT_USERNAME`, `MQTT_PASSWORD`
- OTA: `OTA_URL` - HTTP URL where firmware.bin is hosted
- All MQTT topic definitions (`MQTT_*_TOPIC`)

**Optional SSL:** Uncomment `#define MQTT_USE_SSL` in config.h, create certificate files from `.sample` templates.

## MQTT Topics

Default topic prefix: `MorphingClock/` (configurable via `MQTT_CLIENT_ID`)

| Topic | Payload | Description |
|-------|---------|-------------|
| `.../sensor/temperature` | float | Outdoor temp (triggers display update) |
| `.../sensor/humidity` | int | Outdoor humidity |
| `.../sensor/train1-4` | int | Yellow line train arrival times |
| `.../sensor/bluetrain1-4` | int | Blue line train arrival times |
| `.../sensor/vacationCalendarEvent` | string | Next calendar event name |
| `.../sensor/vacationCalendarDaysTill` | int | Days until next event |
| `.../lastFlight/flightNumber` | string | Flight number display |
| `.../lastFlight/destination` | string | Flight destination code |
| `.../update/req` | "1" | Trigger OTA update |

## Hardware

- ESP32 dev board (pins defined in ESP32 HUB75 DMA library defaults)
- 128x64 HUB75 RGB LED matrix (two 64x64 panels chained, or one 128x64)
- Custom PCB shield (schematics in `pcb/`)
- Optional: TSL2591 I2C light sensor (currently disabled in code)
