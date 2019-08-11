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
    this->bpm = bpm;
    this->frame = frame;
    this->accum = 0;

    this->micros_per_frame = (uint32_t)(61440000000.0 / (float)bpm); // 24:8 bitwise float
  }
  
  void set_bpm(accum88 bpm) {
    this->sync(bpm, 0/*this->frame*/);
  }
};

#endif
