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

  void setup()
  {
    globalTimer.setup();
    this->sync(DEFAULT_BPM << 8, 0, 0);
  }

  void update()
  {
    globalTimer.update();
    
    // Maintains an accumulator with 14 bits of precision
    this->accum += (1145 * (this->bpm>>8) * globalTimer.delta_micros); // 8.388 << 14
    while (this->accum >= (65536<<14)) {
      this->frame++;
      this->accum -= (65536<<14);
    }

  }

  void sync(accum88 bpm, uint32_t frame, uint32_t accum) {
    this->bpm = bpm;
    this->frame = frame;
    this->accum = accum;    
  }
  
  void set_bpm(accum88 bpm) {
    this->sync(bpm, this->frame, 0);
  }
};

#endif
