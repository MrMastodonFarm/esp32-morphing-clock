# ESP32 Morphing Clock Shield Code

## Info

This is the code for my morphing clock project. See [the main Readme file](../) for features that are already implemented or still work-in-progress.

## Before You Start

This requires PlatformIO (and if you are not already using it, do yourself a favor and learn - I have waited way too long before moving to PlatformIO, and I truly regret not doing that sooner! :) ).

You will need some external components in order for all the features to work:
* The temperature/humidity display requires an external MQTT server, and a sensor pushing the data to that MQTT server
* OTA updates require a web server for serving the firmware, and an MQTT server for triggering the updates

Also, you will need to create your own versions of the creds\_\*.h files. I cannot share my versions (for obvious reasons :) ), but I did create some creds\_\*.h.sample files to show you what the contents should look like.

## Panel variants

This directory builds firmware for two different clocks:

```bash
pio run              # both
pio run -e 128x64    # the wide clock
pio run -e 64x64     # the square clock
```

Everything except geometry is shared. The variant flag selects a config header (`include/config_<variant>.h`), a HUB75 pin map (`include/panel_pins_<variant>.h`), and one weather-layout source file (`src/weather_layout_<variant>.cpp`). Anything you change elsewhere lands on both clocks.

Each clock also needs its own MQTT client id, update topic and OTA URL, since they share a broker — `include/device_identity.h` documents these and the build fails with instructions if the 64x64's are missing. See [CLAUDE.md](../CLAUDE.md#panel-variants) for the full picture.
