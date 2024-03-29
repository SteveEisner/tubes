#pragma once


// ================
// Teensy LC
// ================
#if defined(TEENSYDUINO) && defined(__MKL26Z64__)
#define FASTLED_TEENSYLC
#define FASTLED_ARM
#define FASTLED_ARM_M0_PLUS
#define IS_TEENSY
#define STATIC_MEM PROGMEM
#define __ATMEGA__

// Default to using PROGMEM since TEENSYLC provides it
// even though all it does is ignore it.
#ifndef FASTLED_USE_PROGMEM
#define FASTLED_USE_PROGMEM 1
#endif

// ================
// Teensy 3.X
// ================
#elif defined(TEENSYDUINO) && (defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__))
#define FASTLED_TEENSY3
#define FASTLED_ARM
#define FASTLED_ARM_M0_PLUS
#define IS_TEENSY
#define __ATMEGA__

// ================
// Teensy 4.X
// ================
#elif defined(TEENSYDUINO) && (defined(__IMXRT1052__) || defined(__IMXRT1062__))
#define FASTLED_TEENSY4
#define IS_TEENSY
#define __TEENSY40__

#endif

// Only try to use PROGMEM on Teensy LCs
#ifndef STATIC_MEM
#define STATIC_MEM
#endif

#define FASTLED_INTERNAL  # turn off pragmas

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

#ifdef IS_TEENSY
#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#endif

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

#include <ustd_array.h>
#include "util.h"