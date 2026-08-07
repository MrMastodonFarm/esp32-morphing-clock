#ifndef SIM_PGMSPACE_H
#define SIM_PGMSPACE_H

#include <cstdint>

class __FlashStringHelper;

#define PROGMEM
#define pgm_read_byte(address) (*reinterpret_cast<const uint8_t *>(address))
#define pgm_read_word(address) (*reinterpret_cast<const uint16_t *>(address))
#define pgm_read_dword(address) (*reinterpret_cast<const uint32_t *>(address))
#define pgm_read_ptr(address) (*reinterpret_cast<const void *const *>(address))
#define PSTR(text) (text)
#define F(text) (reinterpret_cast<const __FlashStringHelper *>(PSTR(text)))

#endif
