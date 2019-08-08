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
  uint32_t frame = 0;         // current frame #
  uint32_t accum = 0;         // accumulator toward next beat, 14 bits of precision

  // Pattern parameters
  uint8_t pattern = 0;        // Index number of current pattern
  uint8_t palette_id = 0;     // Index of current palette
  uint32_t timer = 0;         // Timer count-up to next transition
  uint8_t sync;               // Whether patterns should stay in sync
};
TubeState currentState;

void printState(TubeState *state)
{
  Serial.print("[");
  Serial.print(state->pattern);
  Serial.print(",");
  Serial.print(state->palette_id);
  Serial.print(" ");
  Serial.print(state->timer);
  Serial.print("ms/");
  Serial.print(state->frame);
  Serial.print("f ");
  Serial.print(state->bpm);
  Serial.print("]");
}

#endif
