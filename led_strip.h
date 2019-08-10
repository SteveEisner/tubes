#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define MAX_LEDS    64

class LEDs {
  public:
    CRGB leds[MAX_LEDS];
    const static int DATA_PIN = 1;
    const static int FRAMES_PER_SECOND = 100;  // how often we refresh the strip, in frames per second
    const static int REFRESH_PERIOD = 1000 / FRAMES_PER_SECOND;  // how often we refresh the strip, in milliseconds
    int num_leds;

  LEDs(int num_leds=MAX_LEDS) {
    this->num_leds = num_leds;
  }
  
  void setup() {
    // tell FastLED about the LED strip configuration
    FastLED.addLeds<WS2812SERIAL,DATA_PIN,RGB>(this->leds, this->num_leds).setCorrection(TypicalLEDStrip);
    Serial.println(F("LEDs: ok"));
  }
  
  void update() {
    EVERY_N_MILLISECONDS( this->REFRESH_PERIOD ) {
      // Update the LEDs
      FastLED.show();
    }
  }
};

#endif
