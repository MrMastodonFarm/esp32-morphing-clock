#ifndef PANEL_PINS_128X64_H
#define PANEL_PINS_128X64_H

// HUB75 pin mapping for the 128x64 clock's custom shield (pcb/ v0.3).
// This is NOT the ESP32-HUB75-MatrixPanel-I2S-DMA library default.

/* Custom board - original RGB ordering, kept for reference
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN 32
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16
*/

#define R1_PIN 25
#define G1_PIN 27
#define B1_PIN 26
#define R2_PIN 14
#define G2_PIN 13
#define B2_PIN 12

#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN 18
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16
//E' or GND

/*
4 5
12 13 14 15 16 17 18 19
23 25 26 27

free:
2
21 22 --> I2C
32 33

TODO:
JTAG: 15 14 13 12
*/

#endif
