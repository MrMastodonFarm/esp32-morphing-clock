#ifndef SIM_PRINT_H
#define SIM_PRINT_H

#ifndef SIM_ARDUINO_H
#include "Arduino.h"
#endif

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

class Print {
public:
  virtual ~Print() = default;
  virtual std::size_t write(uint8_t value) = 0;

  std::size_t write(const uint8_t *buffer, std::size_t size) {
    if (buffer == nullptr) {
      return 0;
    }
    std::size_t written = 0;
    while (written < size && write(buffer[written]) == 1) {
      ++written;
    }
    return written;
  }

  std::size_t write(const char *text) {
    return text == nullptr
               ? 0
               : write(reinterpret_cast<const uint8_t *>(text),
                       std::char_traits<char>::length(text));
  }

  std::size_t print(const char *value) { return write(value); }
  std::size_t print(const String &value) { return print(value.c_str()); }
  std::size_t print(char value) { return write(static_cast<uint8_t>(value)); }
  std::size_t print(int value) { return printNumber(value); }
  std::size_t print(long value) { return printNumber(value); }
  std::size_t print(unsigned value) { return printNumber(value); }
  std::size_t print(float value) { return printFloating(value); }
  std::size_t print(double value) { return printFloating(value); }
  std::size_t print(const __FlashStringHelper *value) {
    return print(reinterpret_cast<const char *>(value));
  }

  std::size_t print(const struct tm *value, const char *format) {
    if (value == nullptr || format == nullptr) {
      return 0;
    }
    char buffer[128] = {};
    const std::size_t length = std::strftime(buffer, sizeof(buffer), format, value);
    return write(reinterpret_cast<const uint8_t *>(buffer), length);
  }

  std::size_t println() { return write("\r\n"); }
  std::size_t println(const char *value) { return print(value) + println(); }
  std::size_t println(const String &value) { return print(value) + println(); }
  std::size_t println(char value) { return print(value) + println(); }
  std::size_t println(int value) { return print(value) + println(); }
  std::size_t println(long value) { return print(value) + println(); }
  std::size_t println(unsigned value) { return print(value) + println(); }
  std::size_t println(float value) { return print(value) + println(); }
  std::size_t println(double value) { return print(value) + println(); }
  std::size_t println(const __FlashStringHelper *value) {
    return print(value) + println();
  }
  std::size_t println(const struct tm *value, const char *format) {
    return print(value, format) + println();
  }

  std::size_t printf(const char *format, ...) {
    if (format == nullptr) {
      return 0;
    }

    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    const int required = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (required < 0) {
      va_end(args);
      return 0;
    }

    std::vector<char> buffer(static_cast<std::size_t>(required) + 1U);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
    return write(reinterpret_cast<const uint8_t *>(buffer.data()),
                 static_cast<std::size_t>(required));
  }

protected:
  template <typename Number> std::size_t printNumber(Number value) {
    return print(std::to_string(value).c_str());
  }

  template <typename Number> std::size_t printFloating(Number value) {
    char buffer[48] = {};
    const int length = std::snprintf(buffer, sizeof(buffer), "%.2f",
                                     static_cast<double>(value));
    return length > 0 ? write(reinterpret_cast<const uint8_t *>(buffer),
                              static_cast<std::size_t>(length))
                      : 0;
  }
};

#endif

