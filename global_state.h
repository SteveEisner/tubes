#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

typedef enum SyncMode {
  All=0,
  SinDrift=1,
  Pulse=2,
  Swing=3,
  SwingDrift=4,
} SyncMode;

class TubeState {
  public:
  
  // Global clock: frames are defined as 1/64th of a beat
  accum88 bpm = 0;            // BPM in high 8 bits, fraction in low 8 bits 
  BeatFrame_24_8 beat_frame = 0;  // current beat (24 bits) and fractional beat (8bits)

  // Pattern parameters
  uint8_t pattern = 0;        // Index number of current pattern
  uint8_t palette_id = 0;     // Index of current palette
  uint32_t timer = 0;         // Timer count-up to next transition
  uint8_t sync;               // Whether patterns should stay in sync
};
TubeState currentState;

void printState(TubeState *state)
{
  Serial.print(F("["));
  Serial.print(state->pattern);
  Serial.print(F(","));
  Serial.print(state->palette_id);
  Serial.print(F(" "));
  Serial.print(state->timer);
  Serial.print(F("ms/"));
  Serial.print(state->beat_frame);
  Serial.print(F("f "));
  Serial.print(state->bpm);
  Serial.print(F("]"));
}

#endif
