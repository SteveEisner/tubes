#include <Arduino.h>
#include <array.h>
#include <FastLED.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include "timer.h"
#include "beats.h"
#include "global_state.h"
#include "virtual_strip.h"
#include "controller.h"
#include "radio.h"
#include "debug.h"


BeatController beats;
PatternController controller(&beats);
Radio radio(&controller);
DebugController debug(&controller, &radio);


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
  beats.setup();
  radio.setup();
  controller.setup();
  debug.setup();
}

void loop()
{
  beats.update(); // ~30us
  radio.update(); // ~60us or ~3000us
  controller.update(); // patterns: 0-3000us   lcd: ~50000us
  debug.update(); // ~25us
  controller.led_strip->update(); // ~25us

  EVERY_N_MILLISECONDS( 1000 ) {
    randomize(globalTimer.now_micros + random());
  }
}
