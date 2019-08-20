#ifndef LED_STRIP_H
#define LED_STRIP_H

#define MAX_LEDS    64
#define MAX_VIRTUAL_LEDS   (2*MAX_LEDS+1)

class LEDs {
  public:
    CRGB leds[MAX_LEDS];
    const static int DATA_PIN = 1;
    const static int FRAMES_PER_SECOND = 300;  // how often we refresh the strip, in frames per second
    const static int REFRESH_PERIOD = 1000 / FRAMES_PER_SECOND;  // how often we refresh the strip, in milliseconds
    int num_leds;

    uint16_t fps = 0;

  LEDs(int num_leds=MAX_LEDS) {
    this->num_leds = num_leds;
  }
  
  void setup() {
    // tell FastLED about the LED strip configuration
    FastLED.addLeds<WS2812SERIAL,DATA_PIN,BRG>(this->leds, this->num_leds).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInMilliWatts(5000);
    Serial.println(F("LEDs: ok"));
  }
  
  void update() {
    EVERY_N_MILLISECONDS( this->REFRESH_PERIOD ) {
      // Update the LEDs
      FastLED.show();
      this->fps++;
    }

    EVERY_N_MILLISECONDS( 1000 ) {
      if (this->fps < (FRAMES_PER_SECOND - 30)) {
        Serial.print(this->fps);
        Serial.println(F(" fps!"));
      }
      this->fps = 0;
    }
  }
};

#endif
