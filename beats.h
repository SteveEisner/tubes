#ifndef BEATS_H
#define BEATS_H

#include "timer.h"

#define DEFAULT_BPM  120

typedef uint32_t BeatFrame_24_8;  // 24:8 bitwise float

// Regulates the beat counter, running patterns at 256 "fracs" per beat
class BeatController {
  public:
    accum88 bpm = 0;
    BeatFrame_24_8 frac = 0;
    uint32_t accum = 0;
    uint32_t micros_per_frac;

  void setup()
  {
    globalTimer.setup();
    this->sync(DEFAULT_BPM << 8, 0);
  }

  void update()
  {
    globalTimer.update();

    // Maintains an accumulator with 14 bits of precision
    this->accum += globalTimer.delta_micros << 8;
    while (this->accum > this->micros_per_frac) {
      this->frac++;
      this->accum -= this->micros_per_frac;
    }
  }

  void sync(accum88 bpm, BeatFrame_24_8 frac) {
    accum88 last_bpm = this->bpm;
    this->bpm = bpm;
    this->frac = frac;
    this->accum = 0;

    this->micros_per_frac = (uint32_t)(15360000000.0 / (float)bpm);

    if (last_bpm != this->bpm)
      this->print_bpm();
  }
  
  void set_bpm(accum88 bpm) {
    this->sync(bpm, this->frac);
  }

  void adjust_bpm(saccum78 bpm) {
    this->sync(this->bpm + bpm, this->frac);
  }

  void start_phrase() {
    this->frac = 0;
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


class BeatTimer {
  public:
    uint32_t endTime;

  void start(uint32_t duration_ms) {
    this->endTime = globalTimer.now_millis + duration_ms;
  }

  void stop() {
    this->start(0);
  }

  void snooze(uint32_t duration_ms) {
    this->endTime += duration_ms;
  }

  bool ended() {
    return globalTimer.now_millis > this->endTime;
  }

  bool every(uint32_t duration_ms) {
    if (!this->ended())
      return 0;
    this->snooze(duration_ms);
    return 1;
  }
};

#endif
