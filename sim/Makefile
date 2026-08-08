# Panel simulator. Builds the real firmware rendering code against host shims.
#
#   make                     simulate the 128x64 clock (default)
#   make VARIANT=64x64       simulate the square clock
#   make VARIANT=64x64 check compare against that variant's goldens
#
# Build products and golden images are namespaced per variant, so the two never
# clobber each other and `make check` compares like with like.

VARIANT ?= 128x64
VALID_VARIANTS := 128x64 64x64
ifeq ($(filter $(VARIANT),$(VALID_VARIANTS)),)
$(error VARIANT must be one of: $(VALID_VARIANTS) (got '$(VARIANT)'))
endif

# 128x64 -> PANEL_VARIANT_128X64
VARIANT_FLAG := -DPANEL_VARIANT_$(subst x,X,$(VARIANT))

OUTDIR := out/$(VARIANT)
GOLDENDIR := golden/$(VARIANT)
TARGET := sim-$(VARIANT)

CXX := g++
CPPFLAGS := -DARDUINO=100 $(VARIANT_FLAG) -Ishims -I../code/include -Ivendor/Adafruit-GFX -Ivendor/ArduinoJson/src
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic

FIRMWARE_SOURCES := \
	../code/src/common.cpp \
	../code/src/clock.cpp \
	../code/src/digit.cpp \
	../code/src/rgb_display.cpp \
	../code/src/weather.cpp \
	../code/src/weather_layout_$(VARIANT).cpp
FIRMWARE_OBJECTS := $(patsubst ../code/src/%.cpp,$(OUTDIR)/%.o,$(FIRMWARE_SOURCES))
OBJECTS := $(OUTDIR)/sim_main.o $(OUTDIR)/sim_capture.o $(OUTDIR)/Adafruit_GFX.o $(FIRMWARE_OBJECTS)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean golden check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OUTDIR)/sim_main.o: sim_main.cpp | $(OUTDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTDIR)/sim_capture.o: shims/sim_capture.cpp | $(OUTDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTDIR)/Adafruit_GFX.o: vendor/Adafruit-GFX/Adafruit_GFX.cpp | $(OUTDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTDIR)/%.o: ../code/src/%.cpp | $(OUTDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OUTDIR):
	mkdir -p $@

golden: $(TARGET)
	mkdir -p $(GOLDENDIR)
	./$(TARGET) --p0 --output $(GOLDENDIR)/p0.ppm
	./$(TARGET) --scenario scenarios/default.scn --output $(GOLDENDIR)/default.ppm
	./$(TARGET) --scenario scenarios/stress.scn --output $(GOLDENDIR)/stress.ppm

check: $(TARGET)
	mkdir -p $(OUTDIR)/check
	./$(TARGET) --p0 --output $(OUTDIR)/check/p0.ppm
	./$(TARGET) --scenario scenarios/default.scn --output $(OUTDIR)/check/default.ppm
	./$(TARGET) --scenario scenarios/stress.scn --output $(OUTDIR)/check/stress.ppm
	@cmp -s $(GOLDENDIR)/p0.ppm $(OUTDIR)/check/p0.ppm || { echo "golden mismatch: p0.ppm"; exit 1; }
	@cmp -s $(GOLDENDIR)/default.ppm $(OUTDIR)/check/default.ppm || { echo "golden mismatch: default.ppm"; exit 1; }
	@cmp -s $(GOLDENDIR)/stress.ppm $(OUTDIR)/check/stress.ppm || { echo "golden mismatch: stress.ppm"; exit 1; }
	@echo "golden check passed ($(VARIANT))"

# Regenerate the animation in the top-level README. Renders a run of consecutive
# minute morphs (including an hour rollover, which is the most interesting one to
# watch) and composites them into an LED-panel-look GIF.
# Re-run this after any layout change so the README stops matching reality only
# in the way the old inherited-from-upstream photo did.
README_GIF := ../photos/panel-$(VARIANT).gif
README_MORPHS := 10:58:00 10:59:00 11:00:00

readme-gif: $(TARGET)
	@rm -rf $(OUTDIR)/readme-frames && mkdir -p $(OUTDIR)/readme-frames
	@i=0; for t in $(README_MORPHS); do \
		./$(TARGET) --morph $$t >/dev/null || exit 1; \
		for f in $$(ls out/morph/*.ppm | sort); do \
			cp $$f $(OUTDIR)/readme-frames/$$(printf '%04d' $$i).ppm; \
			i=$$((i+1)); \
		done; \
	done; echo "collected $$i frames"
	python3 make_readme_gif.py $(OUTDIR)/readme-frames $(README_GIF)

clean:
	rm -rf out sim-128x64 sim-64x64

.PHONY: readme-gif

-include $(DEPS)
