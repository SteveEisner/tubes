#ifndef BEATS_H
#define BEATS_H

#include "timer.h"

#define DEFAULT_BPM  120

// Regulates the beat counter, running patterns at 64 frames per beat
class BeatController {
  public:
    accum88 bpm = 0;
    uint32_t frame = 0;
    uint32_t accum = 0;

    uint32_t micros_per_frame;

  void setup()
  {
    globalTimer.setup();
    this->sync(DEFAULT_BPM << 8, 0);
  }

  void update()
  {
    globalTimer.update();
    
    // Maintains an accumulator with 14 bits of precision
    this->accum += globalTimer.delta_micros << 8;  // 24:8 bitwise float
    while (this->accum > this->micros_per_frame) {
      this->frame++;
      this->accum -= this->micros_per_frame;
    }
  }

  void sync(accum88 bpm, uint32_t frame) {
    bool changed = (bpm != this->bpm);
    this->bpm = bpm;
    this->frame = frame;
    this->accum = 0;

    this->micros_per_frame = (uint32_t)(61440000000.0 / (float)bpm); // 24:8 bitwise float

    if (changed)
      this->print_bpm();
  }
  
  void set_bpm(accum88 bpm) {
    this->sync(bpm, 0/*this->frame*/);
  }

  void adjust_bpm(saccum78 bpm) {
    this->sync(this->bpm + bpm, this->frame);
  }

  void start_phrase() {
    this->frame = 0;
    this->accum = 0;
  }

  void print_bpm() {
    Serial.print(this->bpm >> 8);
    uint8_t frac = scale8(100, this->bpm & 0xFF);
    Serial.print(F("."));
    if (frac < 10)
      Serial.print(F("0"));
    Serial.print(frac);
    Serial.println(F("bpm"));
  }

};

#endif
