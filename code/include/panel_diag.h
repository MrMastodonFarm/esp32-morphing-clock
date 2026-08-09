#ifndef PANEL_DIAG_H
#define PANEL_DIAG_H

// Panel hardware diagnostic, compiled in only for the -diag builds (-DPANEL_DIAG).
// Call once per loop() iteration; it is non-blocking and steps its own phase timer.
void panelDiagUpdate();

#endif
