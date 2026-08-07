#include "Arduino.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

extern MatrixPanel_I2S_DMA *dma_display;

uint32_t sim_millis = 0;

namespace {

bool captureArmed = false;
std::filesystem::path captureDirectory;
unsigned int captureFrame = 0;

}  // namespace

unsigned long millis() { return sim_millis; }

void delay(unsigned long milliseconds) {
  sim_millis += static_cast<uint32_t>(milliseconds);
  if (!captureArmed) {
    return;
  }
  if (dma_display == nullptr) {
    throw std::runtime_error("delay capture requested without a framebuffer");
  }

  std::ostringstream filename;
  filename << "frame-" << std::setw(4) << std::setfill('0') << captureFrame++
           << ".ppm";
  const std::filesystem::path path = captureDirectory / filename.str();
  if (!dma_display->writePPM(path.c_str())) {
    throw std::runtime_error("could not capture " + path.string());
  }
}

void sim_arm_capture(const char *dir) {
  if (dir == nullptr || *dir == '\0') {
    throw std::invalid_argument("capture directory must not be empty");
  }
  captureDirectory = dir;
  std::filesystem::create_directories(captureDirectory);
  captureFrame = 0;
  captureArmed = true;
}

void sim_disarm_capture() {
  captureArmed = false;
  captureDirectory.clear();
  captureFrame = 0;
}
