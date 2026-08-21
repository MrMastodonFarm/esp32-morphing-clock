#ifndef SIM_HTTPCLIENT_H
#define SIM_HTTPCLIENT_H

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "Arduino.h"

inline std::string sim_http_fixture;

inline void sim_set_http_fixture(const char *path) {
  sim_http_fixture.clear();
  if (path == nullptr) {
    return;
  }

  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error(std::string("could not open HTTP fixture: ") + path);
  }
  sim_http_fixture.assign(std::istreambuf_iterator<char>(input),
                          std::istreambuf_iterator<char>());
}

class HTTPClient {
public:
  void begin(const char *) {}
  void setConnectTimeout(int) {}
  void setTimeout(int) {}
  int GET() const { return 200; }
  String getString() const { return String(sim_http_fixture); }
  void end() {}
};

#endif
