CXX := g++
CPPFLAGS := -DARDUINO=100 -Ishims -I../code/include -Ivendor/Adafruit-GFX -Ivendor/ArduinoJson/src
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic

TARGET := sim
FIRMWARE_SOURCES := \
	../code/src/common.cpp \
	../code/src/clock.cpp \
	../code/src/digit.cpp \
	../code/src/rgb_display.cpp \
	../code/src/weather.cpp
FIRMWARE_OBJECTS := $(patsubst ../code/src/%.cpp,out/%.o,$(FIRMWARE_SOURCES))
OBJECTS := out/sim_main.o out/sim_capture.o out/Adafruit_GFX.o $(FIRMWARE_OBJECTS)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean golden check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

out/sim_main.o: sim_main.cpp | out
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

out/sim_capture.o: shims/sim_capture.cpp | out
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

out/Adafruit_GFX.o: vendor/Adafruit-GFX/Adafruit_GFX.cpp | out
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

out/%.o: ../code/src/%.cpp | out
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

out:
	mkdir -p $@

golden: $(TARGET)
	mkdir -p golden
	./$(TARGET) --p0 --output golden/p0.ppm
	./$(TARGET) --scenario scenarios/default.scn --output golden/default.ppm
	./$(TARGET) --scenario scenarios/stress.scn --output golden/stress.ppm

check: $(TARGET)
	mkdir -p out/check
	./$(TARGET) --p0 --output out/check/p0.ppm
	./$(TARGET) --scenario scenarios/default.scn --output out/check/default.ppm
	./$(TARGET) --scenario scenarios/stress.scn --output out/check/stress.ppm
	@cmp -s golden/p0.ppm out/check/p0.ppm || { echo "golden mismatch: p0.ppm"; exit 1; }
	@cmp -s golden/default.ppm out/check/default.ppm || { echo "golden mismatch: default.ppm"; exit 1; }
	@cmp -s golden/stress.ppm out/check/stress.ppm || { echo "golden mismatch: stress.ppm"; exit 1; }
	@echo "golden check passed"

clean:
	rm -rf out $(TARGET)

-include $(DEPS)
