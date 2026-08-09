// Panel hardware diagnostic. Only compiled into the -diag build (-DPANEL_DIAG); the
// normal firmware does not carry any of this.
//
// It exists to answer one question a photo of the running clock cannot: is a suspect
// pixel stuck on, or is it a ghosting artifact of whatever is lit next to it? On the
// normal display every candidate sits next to lit content, and camera bloom makes the
// two indistinguishable. Here the panel is driven to states with no ambiguity:
//
//   black  - every pixel is commanded off. Anything still lit is stuck on, full stop.
//   grid   - dim blue lattice every 8px with a brighter origin marker at (0,0), so a
//            photo can be converted into actual pixel coordinates. Blue is chosen
//            because the pixels under investigation are green.
//   solids - red/green/blue/white in turn. A pixel dark in one solid but lit in the
//            others has a dead channel; a whole line dark in one solid is a dead
//            driver output. This is the half of the diagnosis that "black" cannot give.
//
// Deliberately non-blocking. loop() still has to reach client.loop(), perform_update()
// and esp_task_wdt_reset() on every iteration - if it did not, this build could not be
// replaced over the air, and recovering the clock would mean a USB cable and taking it
// down off the wall.

#ifdef PANEL_DIAG

#include <Arduino.h>

#include "common.h"
#include "panel_diag.h"
#include "rgb_display.h"

namespace {

// A 1/32-scan panel clocks row y and row y+32 together - upper half on R1/G1/B1, lower
// half on R2/G2/B2. If a channel leaks between the two, one half shows a coloured copy
// of the other, offset by exactly 32 rows. HALF_BARS lights dashed bars in one half only
// and leaves the other black, which is the only way to see that: any full-screen fill
// drives both halves identically, so a copy of the other half is invisible by
// construction. The bars are dashed because the artifact being chased is dashed - it
// tracks the dithering in the forecast icons.
enum PhaseKind { FILL, FILL_WITH_GRID, BARS_UPPER, BARS_LOWER };

struct DiagPhase {
  const char *name;
  uint16_t color;
  uint32_t holdMs;
  PhaseKind kind;
};

// Cyan is held longest because it is the phase that answers the actual question. The
// reported fault is green specks, the panel's cyan content (clock digits, temp range) is
// where they show, and green is exactly what cyan looks like with the blue channel
// missing - so a pixel that is green on a full cyan field has a dead blue.
//
// The grid is overlaid *on* the cyan rather than shown separately: one photo then
// carries both the fault and the coordinate reference, instead of needing two shots
// taken from the same position. Grid lines are drawn full-white, which on a cyan field
// reads as white and stays distinct from a green fault pixel.
//
// Solid red/green/blue are last and short. They are near-useless from a phone camera -
// the shutter beats the panel's multiplex and photographs a slice of the refresh as
// dark banding - so they are kept only for looking at directly, by eye.
// Held static rather than cycled. The artifact was localised on 2026-08-09 to the row
// directly above the lower-right bar, so this is now a fixed reference to tune timing
// against over MQTT - a pattern that changed under you would make it impossible to tell
// a real improvement from the next phase starting.
const DiagPhase PHASES[] = {
    {"bars-lower", 0xFFFF, 3600000, BARS_LOWER},
};
const int PHASE_COUNT = sizeof(PHASES) / sizeof(PHASES[0]);

const int GRID_SPACING = 8;

// Bars are placed so the two phases are mirror images across the 32-row split: the
// upper bars sit exactly 32 rows above the lower ones. Whichever half is lit, a leak
// puts its copy where the *other* phase draws its bars, which makes the offset easy to
// confirm by eye without measuring anything.
const int BAR_ROWS_UPPER[] = {8, 22};
const int BAR_ROWS_LOWER[] = {40, 54};
const int BAR_HEIGHT = 3;
const int SCAN_PAIR_OFFSET = 32;

void drawDashedBars(const int *rows, int rowCount, uint16_t color) {
  for (int r = 0; r < rowCount; ++r) {
    // Two runs per row at different x ranges, so a copy cannot be confused with a
    // reflection or a photo artifact - the shape is asymmetric.
    const int xStart = (r == 0) ? 8 : 64;
    const int xEnd = (r == 0) ? 56 : 120;
    for (int y = rows[r]; y < rows[r] + BAR_HEIGHT; ++y) {
      for (int x = xStart; x < xEnd; x += 2) {  // dashed, matching the observed artifact
        dma_display->drawPixel(x, y, color);
      }
    }
  }
}

void drawPhase(const DiagPhase &phase) {
  if (phase.kind == BARS_UPPER || phase.kind == BARS_LOWER) {
    dma_display->fillScreen(0);
    if (phase.kind == BARS_UPPER) {
      drawDashedBars(BAR_ROWS_UPPER, 2, phase.color);
    } else {
      drawDashedBars(BAR_ROWS_LOWER, 2, phase.color);
    }
    return;
  }

  dma_display->fillScreen(phase.color);

  if (phase.kind != FILL_WITH_GRID) {
    return;
  }

  // Full white: on the cyan field it stays clearly distinct from a green fault pixel,
  // and a dead-blue pixel sitting on a grid line shows up as yellow rather than hiding.
  const uint16_t gridColor = dma_display->color565(255, 255, 255);
  for (int x = 0; x < PANEL_WIDTH; x += GRID_SPACING) {
    dma_display->drawFastVLine(x, 0, PANEL_HEIGHT, gridColor);
  }
  for (int y = 0; y < PANEL_HEIGHT; y += GRID_SPACING) {
    dma_display->drawFastHLine(0, y, PANEL_WIDTH, gridColor);
  }

  // Origin marker, so a photo cannot be read upside down or mirrored.
  dma_display->fillRect(0, 0, 3, 3, dma_display->color565(255, 255, 255));
}

}  // namespace

void panelDiagUpdate() {
  static int phase = -1;
  static uint32_t phaseStartedAt = 0;

  const uint32_t now = millis();
  if (phase >= 0 && now - phaseStartedAt < PHASES[phase].holdMs) {
    return;
  }

  phase = (phase + 1) % PHASE_COUNT;
  phaseStartedAt = now;
  drawPhase(PHASES[phase]);
  Serial.printf("[diag] phase: %s\n", PHASES[phase].name);
}

#endif  // PANEL_DIAG
