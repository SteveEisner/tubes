#ifndef DEBUG_H
#define DEBUG_H

// ===== START freeMemory
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
 
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
// ===== END freeMemory


class DebugController {
  public:
    bool debugging = 0;
    PatternController *controller;
    LEDs *strip;
    Radio *radio;

  DebugController(PatternController *controller, Radio *radio) {
    this->controller = controller;
    this->strip = controller->led_strip;
    this->radio = radio;
  }
  
  void setup()
  {
  }

  void update()
  {
    if (this->controller->x_axis > 100) {
      this->debugging = 1;
      this->controller->beats->set_bpm(DEFAULT_BPM << 8);
    } else if (this->controller->x_axis < 30)
      this->debugging = 0;
    
    if (!debugging)
      return;

    uint8_t p1 = (currentState.frame >> 6) % 16;
    this->strip->leds[p1] = CRGB::White;

    uint8_t p2 = scale8(tubeId, this->strip->num_leds-1);
    this->strip->leds[p2] = CRGB::White;

    uint8_t p3 = scale8(masterTubeId, this->strip->num_leds-1);
    if (p3 == p2) {
      this->strip->leds[p3] = CRGB::Green;
    } else {
      this->strip->leds[p3] = CRGB::Yellow; 
    }

    if (this->radio->radioFailures) {
      this->strip->leds[1] = CRGB::Red;
    }
    if (this->radio->radioRestarts) {
      this->strip->leds[2] = CRGB::Red;
    }

    EVERY_N_MILLISECONDS( 10000 ) {
      Serial.print(F("Free memory: "));
      Serial.println( freeMemory() );
    }
  }
};

#endif
