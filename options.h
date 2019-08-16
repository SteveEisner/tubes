#ifndef OPTIONS_H
#define OPTIONS_H


typedef enum Duration {
  ShortDuration=0,
  MediumDuration=10,
  LongDuration=20,
  ExtraLongDuration=30,
} Duration;

typedef enum Energy {
  LowEnergy=0,
  MediumEnergy=10,
  HighEnergy=20,
} Energy;

typedef struct ControlParameters {
  Duration duration=MediumDuration;
  Energy energy=LowEnergy;
} ControlParams;

typedef enum PenMode {
  Draw=1,
  Erase=2,
  Blend=3,
  Invert=4,
  Black=5,
} PenMode;

typedef enum PenColor {
  WhitePen=1,
  BlackPen=2,
  PalettePen=3,
} PenColor;

typedef enum EffectMode {
  None=0,
  Glitter=1,
  Bubble=2,
  Beatbox1=3,
  Beatbox2=4,
  Spark=5,
  Flash4=6,
  Flash8=7,
  Flash16=8,
} EffectMode;

typedef struct {
  EffectMode effect;
  PenColor pen_color=WhitePen;
  PenMode pen=Draw;
  uint8_t chance=255;
} EffectParameters;


#endif
