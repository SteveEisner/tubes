#ifndef LED_STRIP_H
#define LED_STRIP_H

#define MAX_LEDS    64
#define MAX_VIRTUAL_LEDS   (2*MAX_LEDS+1)

class LEDs {
  public:
    CRGB leds[MAX_LEDS];

    // Usable pins:
    //   Teensy LC:   1, 4, 5, 24
    //   Teensy 3.2:  1, 5, 8, 10, 31   (overclock to 120 MHz for pin 8)
    //   Teensy 3.5:  1, 5, 8, 10, 26, 32, 33, 48
    //   Teensy 3.6:  1, 5, 8, 10, 26, 32, 33
    //   Teensy 4.0:  1, 8, 14, 17, 20, 24, 29, 39
    //   Teensy 4.1:  1, 8, 14, 17, 20, 24, 29, 35, 47, 53
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
#ifdef USE_WS2812SERIAL
    FastLED.addLeds<WS2812SERIAL,DATA_PIN,BRG>(this->leds, this->num_leds).setCorrection(TypicalLEDStrip);
#else
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(this->leds, this->num_leds).setCorrection(TypicalLEDStrip);
#endif
    FastLED.setMaxPowerInMilliWatts(5000);
    Serial.println((char *)F("LEDs: ok"));
  }

  void reverse() {
    for (int i=1; i<8; i++) {
      CRGB c = this->leds[i];
      this->leds[i] = this->leds[16-i];
      this->leds[16-i] = c;
    }
  }
  
  void update(bool reverse=false) {
    EVERY_N_MILLISECONDS( this->REFRESH_PERIOD ) {
      // Update the LEDs
      if (reverse)
        this->reverse();
      FastLED.show();
      this->fps++;
    }

    EVERY_N_MILLISECONDS( 1000 ) {
      if (this->fps < (FRAMES_PER_SECOND - 30)) {
        Serial.print(this->fps);
        Serial.println((char *)F(" fps!"));
      }
      this->fps = 0;
    }
  }
};

#endif
