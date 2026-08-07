#include <array>
#include <cctype>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "HTTPClient.h"
#include "clock.h"
#include "common.h"
#include "rgb_display.h"
#include "weather.h"

namespace {

constexpr int kPanelWidth = PANEL_WIDTH;
constexpr int kPanelHeight = PANEL_HEIGHT;

struct Scenario {
  std::tm time = {};
  float temp = 88.5F;
  int humidity = 61;
  bool sensorDead = false;
  std::array<int, 4> trains = {4, 12, 19, 27};
  std::array<int, 4> blueTrains = {2, 9, 16, 24};
  std::string event = "Beach trip";
  int eventDays = 12;
  std::string flight = "AA123";
  std::string flightDestination = "MI";
  std::string weather = "fixtures/open-meteo-summer.json";
};

std::string trim(const std::string &value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::tm parseTime(const std::string &value, const char *format) {
  std::tm parsed = {};
  parsed.tm_isdst = -1;
  char *end = strptime(value.c_str(), format, &parsed);
  if (end == nullptr || *end != '\0') {
    throw std::runtime_error("invalid time: " + value);
  }
  if (std::mktime(&parsed) == static_cast<std::time_t>(-1)) {
    throw std::runtime_error("time is outside the supported range: " + value);
  }
  return parsed;
}

Scenario defaultScenario() {
  Scenario scenario;
  scenario.time = parseTime("2026-08-07 19:42:00", "%Y-%m-%d %H:%M:%S");
  return scenario;
}

template <typename T>
T parseScalar(const std::string &value, const std::string &key) {
  std::istringstream input(value);
  T result;
  if (!(input >> result)) {
    throw std::runtime_error("invalid value for " + key + ": " + value);
  }
  input >> std::ws;
  if (!input.eof()) {
    throw std::runtime_error("extra value for " + key + ": " + value);
  }
  return result;
}

std::array<int, 4> parseTrains(const std::string &value,
                               const std::string &key) {
  std::istringstream input(value);
  std::array<int, 4> result = {};
  for (int &train : result) {
    if (!(input >> train)) {
      throw std::runtime_error(key + " requires exactly four integers");
    }
  }
  input >> std::ws;
  if (!input.eof()) {
    throw std::runtime_error(key + " requires exactly four integers");
  }
  return result;
}

std::string parseToken(const std::string &value, const std::string &key) {
  std::istringstream input(value);
  std::string token;
  if (!(input >> token)) {
    throw std::runtime_error("missing value for " + key);
  }
  input >> std::ws;
  if (!input.eof()) {
    throw std::runtime_error(key + " must be a single whitespace-free value");
  }
  return token;
}

void validateScenario(const Scenario &scenario) {
  if (scenario.event.size() > 64) {
    throw std::runtime_error("event exceeds the firmware's 64-character limit");
  }
  if (scenario.flight.size() > 5) {
    throw std::runtime_error("flight exceeds the firmware's 5-character limit");
  }
  if (scenario.flightDestination.size() > 2) {
    throw std::runtime_error(
        "flight_dest exceeds the firmware's 2-character limit");
  }
}

Scenario loadScenario(const std::string &path) {
  Scenario scenario = defaultScenario();
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("could not open scenario: " + path);
  }

  std::string line;
  unsigned int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    const std::size_t comment = line.find('#');
    if (comment != std::string::npos) {
      line.erase(comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }

    const std::size_t split = line.find_first_of(" \t");
    if (split == std::string::npos) {
      throw std::runtime_error(path + ':' + std::to_string(lineNumber) +
                               ": missing value");
    }
    const std::string key = line.substr(0, split);
    const std::string value = trim(line.substr(split + 1));
    if (value.empty()) {
      throw std::runtime_error(path + ':' + std::to_string(lineNumber) +
                               ": missing value for " + key);
    }

    try {
      if (key == "time") {
        scenario.time = parseTime(value, "%Y-%m-%d %H:%M:%S");
      } else if (key == "temp") {
        scenario.temp = parseScalar<float>(value, key);
      } else if (key == "humidity") {
        scenario.humidity = parseScalar<int>(value, key);
      } else if (key == "sensor_dead") {
        if (value == "true") {
          scenario.sensorDead = true;
        } else if (value == "false") {
          scenario.sensorDead = false;
        } else {
          throw std::runtime_error("sensor_dead must be true or false");
        }
      } else if (key == "trains") {
        scenario.trains = parseTrains(value, key);
      } else if (key == "bluetrains") {
        scenario.blueTrains = parseTrains(value, key);
      } else if (key == "event") {
        scenario.event = value;
      } else if (key == "event_days") {
        scenario.eventDays = parseScalar<int>(value, key);
      } else if (key == "flight") {
        scenario.flight = parseToken(value, key);
      } else if (key == "flight_dest") {
        scenario.flightDestination = parseToken(value, key);
      } else if (key == "weather") {
        scenario.weather = parseToken(value, key);
      } else {
        throw std::runtime_error("unknown key: " + key);
      }
    } catch (const std::exception &error) {
      throw std::runtime_error(path + ':' + std::to_string(lineNumber) +
                               ": " + error.what());
    }
  }

  validateScenario(scenario);
  return scenario;
}

bool writePanel(const std::string &path) {
  const std::filesystem::path outputPath(path);
  if (!outputPath.parent_path().empty()) {
    std::filesystem::create_directories(outputPath.parent_path());
  }
  if (dma_display == nullptr || !dma_display->writePPM(path.c_str())) {
    std::cerr << "could not write " << path << '\n';
    return false;
  }
  std::cout << "wrote " << path << " (" << kPanelWidth << 'x' << kPanelHeight
            << " P6)\n";
  return true;
}

void resetPanel() {
  if (dma_display != nullptr) {
    delete dma_display;
    dma_display = nullptr;
  }
  display_init();
  if (dma_display == nullptr) {
    throw std::runtime_error("panel initialization failed");
  }
}

template <std::size_t N>
void setCString(char (&destination)[N], const std::string &source) {
  if (source.size() >= N) {
    throw std::runtime_error("scenario string does not fit firmware buffer");
  }
  std::memcpy(destination, source.c_str(), source.size() + 1);
}

void drawScene(const Scenario &scenario, std::optional<uint8_t> icon = {}) {
  resetPanel();
  timeinfo = scenario.time;

  sensorTemp = scenario.temp;
  sensorHumi = scenario.humidity;
  sensorTrain1 = scenario.trains[0];
  sensorTrain2 = scenario.trains[1];
  sensorTrain3 = scenario.trains[2];
  sensorTrain4 = scenario.trains[3];
  sensorBlueTrain1 = scenario.blueTrains[0];
  sensorBlueTrain2 = scenario.blueTrains[1];
  sensorBlueTrain3 = scenario.blueTrains[2];
  sensorBlueTrain4 = scenario.blueTrains[3];
  setCString(sensorNextEvent, scenario.event);
  sensorDaysTillNextEvent = scenario.eventDays;
  setCString(sensorFlightNumber, scenario.flight);
  setCString(sensorFlightDestination, scenario.flightDestination);
  sensorDead = scenario.sensorDead;

  newSensorData = true;
  newTrainData = true;
  newCalendarData = true;
  newFlightNumber = true;
  newFlightDestination = true;
  clockStartingUp = true;

  sim_set_http_fixture(scenario.weather.c_str());
  getOpenMeteoData();
  if (icon.has_value()) {
    for (std::size_t i = 0; i < 5; ++i) {
      forecast5Days[i] = *icon;
    }
  }

  drawTestBitmap();
  displayWeatherData();
  displaySensorData();
  displayTodaysWeather();
  displayTrainData();
  displayCalendarData();
  displayFlightNumber();
  displayFlightDestination();
  displayClock();

  clearStatusMessage();
  drawTestBitmap();
  CJBMessage(CJB_MESSAGE);
}

int renderP0(const std::string &path) {
  resetPanel();

  const uint16_t red = dma_display->color565(255, 32, 32);
  const uint16_t green = dma_display->color565(64, 255, 96);
  const uint16_t cyan = dma_display->color565(64, 224, 255);
  const uint16_t yellow = dma_display->color565(255, 224, 32);

  dma_display->fillRect(2, 2, 40, 20, red);
  dma_display->setFont();
  dma_display->setCursor(48, 4);
  dma_display->setTextColor(green);
  dma_display->print("12:34");
  dma_display->setFont(&TomThumb);
  dma_display->setCursor(3, 34);
  dma_display->setTextColor(cyan);
  dma_display->print("TomThumb");
  dma_display->setFont();
  dma_display->setCursor(2, 44);
  dma_display->setTextColor(yellow);
  dma_display->printf("%3d/%3d", -12, 4);
  std::tm friday = {};
  friday.tm_wday = 5;
  dma_display->setCursor(62, 44);
  dma_display->print(&friday, "%a");

  return writePanel(path) ? 0 : 1;
}

int renderScenario(const Scenario &scenario, const std::string &path) {
  drawScene(scenario);
  return writePanel(path) ? 0 : 1;
}

int renderHourSweep() {
  const std::filesystem::path directory = "out/sweep-hour";
  std::filesystem::remove_all(directory);
  Scenario scenario = defaultScenario();
  for (int hour = 0; hour < 24; ++hour) {
    scenario.time.tm_hour = hour;
    scenario.time.tm_min = 42;
    scenario.time.tm_sec = 0;
    std::mktime(&scenario.time);
    drawScene(scenario);
    std::ostringstream name;
    name << std::setw(2) << std::setfill('0') << hour << ".ppm";
    if (!writePanel((directory / name.str()).string())) {
      return 1;
    }
  }
  return 0;
}

int renderMoonSweep() {
  const std::filesystem::path directory = "out/sweep-moon";
  std::filesystem::remove_all(directory);
  const std::array<const char *, 8> dates = {
      "2026-08-01 21:42:00", "2026-08-05 21:42:00",
      "2026-08-08 21:42:00", "2026-08-12 21:42:00",
      "2026-08-16 21:42:00", "2026-08-19 21:42:00",
      "2026-08-23 21:42:00", "2026-08-27 21:42:00",
  };
  std::array<bool, 8> seen = {};
  Scenario scenario = defaultScenario();
  for (const char *date : dates) {
    scenario.time = parseTime(date, "%Y-%m-%d %H:%M:%S");
    timeinfo = scenario.time;
    const uint8_t phase = getMoonPhase();
    if (phase > 7 || seen[phase]) {
      throw std::runtime_error("moon sweep dates do not yield eight distinct phases");
    }
    seen[phase] = true;
    drawScene(scenario);
    if (!writePanel((directory / (std::to_string(phase) + ".ppm")).string())) {
      return 1;
    }
  }
  return 0;
}

int renderIconSweep() {
  const std::filesystem::path directory = "out/sweep-icons";
  std::filesystem::remove_all(directory);
  Scenario scenario = defaultScenario();
  scenario.time.tm_hour = 12;
  std::mktime(&scenario.time);
  for (uint8_t icon = 0; icon < 6; ++icon) {
    drawScene(scenario, icon);
    if (!writePanel((directory / (std::to_string(icon) + ".ppm")).string())) {
      return 1;
    }
  }
  return 0;
}

int renderMorph(const std::string &clockTime) {
  const std::filesystem::path directory = "out/morph";
  std::filesystem::remove_all(directory);
  Scenario scenario = defaultScenario();
  scenario.time = parseTime("2026-08-07 " + clockTime,
                            "%Y-%m-%d %H:%M:%S");
  drawScene(scenario);

  // The next normal loop establishes the firmware's previous-minute/hour state.
  displayClock();

  sim_arm_capture(directory.c_str());
  delay(0);  // Include the steady starting scene via the delay-based frame hook.
  timeinfo.tm_sec = 0;
  ++timeinfo.tm_min;
  if (std::mktime(&timeinfo) == static_cast<std::time_t>(-1)) {
    sim_disarm_capture();
    throw std::runtime_error("could not advance morph time");
  }
  displayClock();
  delay(0);  // capture the settled end state — Hide()/Draw() paint without delays
  sim_disarm_capture();
  return 0;
}

enum class Mode { Scene, P0, Scenario, Stress, Hour, Moon, Icons, Morph };

struct Command {
  Mode mode = Mode::Scene;
  std::string argument;
  std::optional<std::string> output;
};

void usage(const char *program) {
  std::cerr << "usage: " << program
            << " [--scene|--p0|--scenario FILE|--stress|--sweep "
               "hour|moon|icons|--morph HH:MM:SS] [--output FILE]\n";
}

Command parseCommand(int argc, char **argv) {
  Command command;
  bool selected = false;
  for (int i = 1; i < argc; ++i) {
    const std::string option = argv[i];
    if (option == "--output") {
      if (++i >= argc || command.output.has_value()) {
        throw std::runtime_error("--output requires one file");
      }
      command.output = argv[i];
      continue;
    }
    if (selected) {
      throw std::runtime_error("only one render mode may be selected");
    }
    selected = true;
    if (option == "--scene") {
      command.mode = Mode::Scene;
    } else if (option == "--p0") {
      command.mode = Mode::P0;
    } else if (option == "--stress") {
      command.mode = Mode::Stress;
    } else if (option == "--scenario") {
      if (++i >= argc) {
        throw std::runtime_error("--scenario requires a file");
      }
      command.mode = Mode::Scenario;
      command.argument = argv[i];
    } else if (option == "--sweep") {
      if (++i >= argc) {
        throw std::runtime_error("--sweep requires hour, moon, or icons");
      }
      const std::string sweep = argv[i];
      if (sweep == "hour") {
        command.mode = Mode::Hour;
      } else if (sweep == "moon") {
        command.mode = Mode::Moon;
      } else if (sweep == "icons") {
        command.mode = Mode::Icons;
      } else {
        throw std::runtime_error("unknown sweep: " + sweep);
      }
    } else if (option == "--morph") {
      if (++i >= argc) {
        throw std::runtime_error("--morph requires HH:MM:SS");
      }
      command.mode = Mode::Morph;
      command.argument = argv[i];
    } else {
      throw std::runtime_error("unknown option: " + option);
    }
  }
  return command;
}

}  // namespace

int main(int argc, char **argv) {
  try {
    const Command command = parseCommand(argc, argv);
    switch (command.mode) {
      case Mode::Scene:
        return renderScenario(defaultScenario(),
                              command.output.value_or("out/scene.ppm"));
      case Mode::P0:
        return renderP0(command.output.value_or("out/p0.ppm"));
      case Mode::Scenario:
        return renderScenario(loadScenario(command.argument),
                              command.output.value_or("out/scene.ppm"));
      case Mode::Stress:
        return renderScenario(loadScenario("scenarios/stress.scn"),
                              command.output.value_or("out/stress.ppm"));
      case Mode::Hour:
        return renderHourSweep();
      case Mode::Moon:
        return renderMoonSweep();
      case Mode::Icons:
        return renderIconSweep();
      case Mode::Morph:
        return renderMorph(command.argument);
    }
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    usage(argv[0]);
    return 2;
  }
  return 2;
}
