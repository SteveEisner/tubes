#ifndef FASTLED_CONFIG_H
#define FASTLED_CONFIG_H

#define FASTLED_TEENSYLC
#define FASTLED_ARM
#define FASTLED_ARM_M0_PLUS

#ifndef INTERRUPT_THRESHOLD
#define INTERRUPT_THRESHOLD 1
#endif

#define FASTLED_SPI_BYTE_ONLY

// Default to allowing interrupts
#ifndef FASTLED_ALLOW_INTERRUPTS
#define FASTLED_ALLOW_INTERRUPTS 1
#endif

#if FASTLED_ALLOW_INTERRUPTS == 1
#define FASTLED_ACCURATE_CLOCK
#endif

// Default to using PROGMEM since TEENSYLC provides it
// even though all it does is ignore it.  Just being
// conservative here in case TEENSYLC changes.
#ifndef FASTLED_USE_PROGMEM
#define FASTLED_USE_PROGMEM 1
#endif

#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later"
#endif

uint8_t scaled16to8( uint16_t v, uint16_t lowest=0, uint16_t highest=65535) {
  uint16_t rangewidth = highest - lowest;
  uint16_t scaledbeat = scale16( v, rangewidth );
  uint16_t result = lowest + scaledbeat;
  return result;
}

#endif
