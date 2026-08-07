#ifndef SIM_ESP32_HUB75_MATRIXPANEL_I2S_DMA_H
#define SIM_ESP32_HUB75_MATRIXPANEL_I2S_DMA_H

#include <cstdint>
#include <fstream>

#include "Adafruit_GFX.h"
#include "config.h"   // PANEL_WIDTH / PANEL_HEIGHT for the selected variant

struct HUB75_I2S_CFG {
  struct i2s_pins {
    int r1;
    int g1;
    int b1;
    int r2;
    int g2;
    int b2;
    int a;
    int b;
    int c;
    int d;
    int e;
    int lat;
    int oe;
    int clk;
  };

  enum shift_driver { SHIFTREG, FM6124 };

  HUB75_I2S_CFG(int width = PANEL_WIDTH, int height = PANEL_HEIGHT, int chain = 1,
                i2s_pins pins = {})
      : mx_width(width), mx_height(height), chain_length(chain), gpio(pins) {}

  int mx_width;
  int mx_height;
  int chain_length;
  i2s_pins gpio;
  bool clkphase = true;
  shift_driver driver = SHIFTREG;
};

class MatrixPanel_I2S_DMA : public Adafruit_GFX {
public:
  static constexpr int kWidth = PANEL_WIDTH;
  static constexpr int kHeight = PANEL_HEIGHT;

  explicit MatrixPanel_I2S_DMA(const HUB75_I2S_CFG & = HUB75_I2S_CFG())
      : Adafruit_GFX(kWidth, kHeight), fb{} {}

  uint16_t fb[kWidth * kHeight];

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x >= 0 && x < kWidth && y >= 0 && y < kHeight) {
      fb[y * kWidth + x] = color;
    }
  }

  bool begin() { return true; }
  void setPanelBrightness(int) {}

  uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) const {
    return static_cast<uint16_t>(((red & 0xF8U) << 8U) |
                                 ((green & 0xFCU) << 3U) | (blue >> 3U));
  }

  uint16_t color444(uint8_t red, uint8_t green, uint8_t blue) const {
    return static_cast<uint16_t>(((red & 0x0FU) << 12U) |
                                 ((green & 0x0FU) << 7U) |
                                 ((blue & 0x0FU) << 1U));
  }

  bool writePPM(const char *path) const {
    if (path == nullptr) {
      return false;
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) {
      return false;
    }

    output << "P6\n" << kWidth << ' ' << kHeight << "\n255\n";
    for (uint16_t pixel : fb) {
      const uint8_t red5 = static_cast<uint8_t>((pixel >> 11U) & 0x1FU);
      const uint8_t green6 = static_cast<uint8_t>((pixel >> 5U) & 0x3FU);
      const uint8_t blue5 = static_cast<uint8_t>(pixel & 0x1FU);
      const char rgb[3] = {
          static_cast<char>((red5 << 3U) | (red5 >> 2U)),
          static_cast<char>((green6 << 2U) | (green6 >> 4U)),
          static_cast<char>((blue5 << 3U) | (blue5 >> 2U)),
      };
      output.write(rgb, sizeof(rgb));
    }
    return output.good();
  }
};

#endif
