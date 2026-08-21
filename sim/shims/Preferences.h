#ifndef SIM_PREFERENCES_H
#define SIM_PREFERENCES_H

#include <cstddef>

// No-op stand-in for the ESP32 NVS Preferences API. The simulator has no flash;
// loadWeatherCache() always reports "nothing cached".
class Preferences {
public:
  bool begin(const char *, bool = false) { return false; }
  void end() {}
  size_t putBytes(const char *, const void *, size_t) { return 0; }
  size_t getBytes(const char *, void *, size_t) { return 0; }
};

#endif
