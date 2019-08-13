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
    PatternController *controller;
    LEDs *strip;
    Radio *radio;
    uint32_t lastPhraseTime;
    uint32_t lastFrame;

  DebugController(PatternController *controller) {
    this->controller = controller;
    this->strip = controller->led_strip;
    this->radio = controller->radio;
  }
  
  void setup()
  {
    this->lastPhraseTime = globalTimer.now_micros;
    this->lastFrame = (uint32_t)-1;
  }

  void update()
  {
    if (!this->controller->options.debugging)
      return;

    uint32_t frame = currentState.beat_frame % (32 * 256);
    if (frame == 0 && this->lastFrame != 0) {
      uint32_t t = globalTimer.now_micros;
      Serial.print(t - this->lastPhraseTime);
      Serial.print(F(" from "));
      this->controller->beats->print_bpm();
      this->lastPhraseTime = t;
    }
    this->lastFrame = frame;
    
    uint8_t p1 = (currentState.beat_frame >> 8) % 16;
    this->strip->leds[p1] = CRGB::White;

    uint8_t p2 = scale8(this->controller->radio->tubeId, this->strip->num_leds-1);
    this->strip->leds[p2] = CRGB::White;

    uint8_t p3 = scale8(this->controller->radio->masterTubeId, this->strip->num_leds-1);
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
