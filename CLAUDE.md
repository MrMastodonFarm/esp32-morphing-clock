# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based morphing digital clock displayed on a 128x64 HUB75 RGB LED matrix. The clock shows time with morphing animations, date, weather forecasts (via AccuWeather API), and sensor data (received via MQTT from external sensors).

## Build Commands

All commands run from the `code/` directory using PlatformIO:

```bash
pio run                    # Build firmware
pio run --target upload    # Build and upload via USB
pio monitor                # Serial monitor (115200 baud)
```

For OTA updates: `./ota_build.sh` (requires MQTT server and Docker nginx container for firmware hosting)

## Project Structure

```
code/                      # PlatformIO firmware project
├── src/                   # C++ source files
├── include/               # Headers and configuration
│   ├── config.h           # Display layout, colors, pins, intervals
│   ├── creds_*.h          # Credentials (create from .sample files)
│   └── common.h           # Global state and extern declarations
├── platformio.ini         # Build configuration
└── ota_build.sh           # OTA deployment script
pcb/                       # Eagle CAD schematics for ESP32-Matrix shield
case/                      # Laser-cut enclosure drawings (DWG)
```

## Architecture

**Main modules:**
- `main.cpp` - Entry point, setup/loop, periodic task scheduling (NTP, weather updates)
- `clock.cpp` - Morphing digit animation (7-segment style transitions)
- `rgb_display.cpp` - HUB75 matrix abstraction using ESP32 DMA library
- `mqtt.cpp` - MQTT callbacks for sensor data, train times, calendar, flights, OTA triggers
- `weather.cpp` - AccuWeather API integration for 5-day forecasts
- `digit.cpp` - 7-segment digit rendering

**Data flow:**
1. MQTT callbacks set global variables and flags in `common.h`
2. Main loop checks flags and calls display update functions
3. 30ms ticker drives `displayUpdater()` for smooth animations

**Key globals (in common.h):** Display flags like `newSensorData`, `newTrainData` trigger section redraws when set to true.

## Configuration

- **Display layout/colors:** `include/config.h` (positions, color565 values, pin assignments)
- **Credentials:** Create `creds_mqtt.h` and `creds_accuweather.h` from `.sample` files
- **Timezone:** Configured in `config.h` (default: EST with DST support)

## External Dependencies

- MQTT server for receiving sensor data and triggering OTA updates
- AccuWeather API key for weather forecasts
- Optional: TSL2591 I2C light sensor for ambient light detection

## Hardware

- ESP32 dev board
- 128x64 HUB75 RGB LED matrix panel
- Custom PCB shield (schematics in `pcb/`)
- Optional: TSL2591 light sensor, buzzer
