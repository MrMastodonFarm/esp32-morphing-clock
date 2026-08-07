#ifndef SIM_WIFI_H
#define SIM_WIFI_H

constexpr int WL_IDLE_STATUS = 0;
constexpr int WL_CONNECTED = 3;

class WiFiClient {};

class WiFiClass {
public:
  int status() const { return WL_CONNECTED; }
};

inline WiFiClass WiFi;

#endif
