#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "pgmspace.h"

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

using byte = uint8_t;

class String {
public:
  String() = default;
  String(const char *value) : value_(value == nullptr ? "" : value) {}
  String(const std::string &value) : value_(value) {}

  const char *c_str() const { return value_.c_str(); }
  std::size_t length() const { return value_.length(); }
  bool concat(const char *value) {
    if (value == nullptr) {
      return false;
    }
    value_ += value;
    return true;
  }

private:
  std::string value_;
};

extern uint32_t sim_millis;

unsigned long millis();
void delay(unsigned long milliseconds);
void sim_arm_capture(const char *dir);
void sim_disarm_capture();
inline void yield() {}
inline void pinMode(uint8_t, uint8_t) {}

constexpr uint8_t INPUT = 0;
constexpr uint8_t OUTPUT = 1;

#define B01100000 0b01100000
#define B01100110 0b01100110
#define B10110110 0b10110110
#define B10111110 0b10111110
#define B11011010 0b11011010
#define B11100000 0b11100000
#define B11110010 0b11110010
#define B11110110 0b11110110
#define B11111100 0b11111100
#define B11111110 0b11111110
#define bitRead(value, bit) (((value) >> (bit)) & 0x01U)

using std::max;
using std::min;

#include "Print.h"

class Printable {
public:
  virtual ~Printable() = default;
  virtual std::size_t printTo(Print &output) const = 0;
};

class Stream : public Print {
public:
  virtual std::size_t readBytes(char *buffer, std::size_t length) = 0;
};

class SimSerial final : public Print {
public:
  std::size_t write(uint8_t value) override;
};

inline SimSerial Serial;

#include <cstdio>

inline std::size_t SimSerial::write(uint8_t value) {
  return std::fputc(static_cast<int>(value), stdout) == EOF ? 0U : 1U;
}

#endif
