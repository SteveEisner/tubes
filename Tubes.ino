#include <Arduino.h>
#include <array.h>
#include <avr/pgmspace.h>

#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// #define MASTERCONTROL

#ifdef MASTERCONTROL

#define NUM_LEDS 64
#define USELCD
#define USERADIO
#define USEJOYSTICK

#else

#define NUM_LEDS 64
#define USERADIO

#endif

#include "beats.h"
#include "global_state.h"
#include "virtual_strip.h"
#include "controller.h"
#include "radio.h"
#include "debug.h"


BeatController beats;
Radio radio;
PatternController controller(NUM_LEDS, &beats, &radio);
DebugController debug(&controller);


void randomize(long seed) {
  for (int i = 0; i < seed % 16; i++) {
    randomSeed(random());
  }
  random16_add_entropy( random() );  
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  randomize(analogRead(0));

  // Start timing
  globalTimer.setup();
  beats.setup();
  controller.setup();
  debug.setup();
}

void loop()
{
  EVERY_N_MILLISECONDS( 1000 ) {
    randomize(random());
  }

  beats.update(); // ~30us
  controller.update(); // radio: 0-3000us   patterns: 0-3000us   lcd: ~50000us
  debug.update(); // ~25us

  // Draw after everything else is done
  controller.led_strip->update(); // ~25us
}
