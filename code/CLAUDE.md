# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based morphing clock that displays time with animated digit transitions on a 64x64 HUB75 RGB LED matrix. Also shows weather forecasts, temperature/humidity sensor data, transit arrival times, flight info, and calendar events via MQTT.

## Build Commands

This project uses PlatformIO (not Arduino IDE).

```bash
# Build firmware
pio run

# Build and upload via USB
pio run --target upload

# Monitor serial output (115200 baud)
pio device monitor
```

Output: `.pio/build/esp32dev/firmware.bin`

## Configuration

Before building, create credential files based on the samples (these are gitignored):
- `include/creds_mqtt.h` - MQTT broker settings, WiFi credentials, topic names
- `include/creds_weather.h` - Weather location (latitude/longitude for Open-Meteo API)
- SSL certs (optional): `client.crt.h`, `client.key.h`, `server_mqtt.crt.h`

Key settings in [config.h](include/config.h):
- `TIMEZONE_DELTA_SEC` - GMT offset in seconds (-18000 = UTC-5)
- `TIMEZONE_DST_SEC` - DST offset (3600)
- Display coordinates for all UI elements
- `WDT_TIMEOUT` - Watchdog timer (60s) - don't set too low or OTA fails

## Architecture

**Entry point**: [main.cpp](src/main.cpp) - `setup()` initializes display, WiFi, NTP, weather API, MQTT; `loop()` runs every 500ms handling refreshes and MQTT

**Display rendering** (all render to `dma_display` global):
- [digit.cpp](src/digit.cpp) - `Digit` class with `Morph0()` through `Morph9()` animation functions for 7-segment transitions
- [clock.cpp](src/clock.cpp) - `displayClock()` updates time using digit morphing
- [rgb_display.cpp](src/rgb_display.cpp) - HUB75 DMA init, text/sensor/train display helpers
- [weather.cpp](src/weather.cpp) - Open-Meteo API fetch and weather icon rendering

**Data flow**:
- NTP → `configTime()` → `timeinfo` struct → `displayClock()`
- Open-Meteo → HTTP GET → global weather vars → `displayWeatherData()`
- MQTT → `mqtt_callback()` ([mqtt.cpp](src/mqtt.cpp)) → sets `newSensorData`/`newTrainData`/etc flags → display functions in loop

**Global state** in [common.h](include/common.h)/[common.cpp](src/common.cpp):
- `dma_display` - MatrixPanel_I2S_DMA pointer
- `sensorTemp`, `sensorHumi`, `sensorTrain1-4`, `sensorBlueTrain1-4`
- `newSensorData`, `newTrainData`, `newCalendarData`, `newFlightNumber` flags

## Hardware

**LED Matrix**: 64x64 HUB75 RGB panel with FM6124 driver, DMA-controlled

**GPIO pins** (defined in [rgb_display.h](include/rgb_display.h)):
- RGB data: R1=25, G1=26, B1=27, R2=14, G2=12, B2=13
- Control: A=23, B=19, C=5, D=17, E=18, LAT=4, OE=15, CLK=16
- Button: GPIO 32
- I2C (for TSL2591 light sensor): 21, 22

Note: Different panel types may need different RGB pin ordering - see commented alternatives in rgb_display.h

## Debugging

WebSerial available at `http://<device-ip>/webserial` for remote debugging without USB connection.
