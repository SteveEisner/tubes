#ifndef PARTICLE_H
#define PARTICLE_H

class Particle;

typedef void (*ParticleFn)(Particle *particle, CRGB strip[], uint8_t num_leds);

extern void drawPoint(Particle *particle, CRGB strip[], uint8_t num_leds);
extern void drawFlash(Particle *particle, CRGB strip[], uint8_t num_leds);


class Particle {
  const static uint16_t DEFAULT_BRIGHTNESS = (192<<8); // or 96

  public:
    BeatFrame_24_8 born;
    BeatFrame_24_8 lifetime;
    BeatFrame_24_8 age;

    uint16_t position = 0;
    int16_t velocity = 0;
    int16_t gravity = 0;
    void (*die_fn)(Particle *particle) = NULL;

    CRGBPalette16 palette;
    CRGB color;
    uint16_t brightness;
    ParticleFn drawFn;

  Particle(uint16_t position, CRGB color=CRGB::White, uint32_t lifetime=20000, ParticleFn drawFn=drawPoint)
  {
    this->born = currentState.beat_frame;
    this->age = 0;
    this->position = position;
    this->color = color;
    this->lifetime = lifetime;
    this->brightness = DEFAULT_BRIGHTNESS;
    this->drawFn = drawFn;
  }

  void update(BeatFrame_24_8 frame)
  {
    this->age = frame - this->born;
    this->position = this->udelta16(this->position, this->velocity);
    this->velocity = this->delta16(this->velocity, this->gravity);
  }

  uint8_t age_frac8(BeatFrame_24_8 age)
  {
    if (age >= this->lifetime)
      return 255;
    return (256 * age) / lifetime;
  }

  uint16_t udelta16(uint16_t x, int16_t dx)
  {
    if (dx > 0 && 65535-x < dx)
      return 65335;
    if (dx < 0 && x < -dx)
      return 0;
    return x + dx;
  }
  
  int16_t delta16(int16_t x, int16_t dx)
  {
    if (dx > 0 && 32767-x < dx)
      return 32767;
    if (dx < 0 && x < -32767 - dx)
      return -32767;
    return x + dx;
  }

  CRGB color_at(uint8_t age_frac) {
    // Particles get dimmer with age
    uint8_t brightness = scale8((uint8_t)(this->brightness>>8), 255 - age_frac);

    // a black pattern actually means to use the current palette
    if (this->color == CRGB(0,0,0))
      return ColorFromPalette(this->palette, age_frac, brightness);

    uint8_t r = scale8(this->color.r, brightness);
    uint8_t g = scale8(this->color.g, brightness);
    uint8_t b = scale8(this->color.b, brightness);
    return CRGB(r,g,b);
  }
  
};

void drawPoint(Particle *particle, CRGB strip[], uint8_t num_leds) {
  uint8_t age_frac = particle->age_frac8(particle->age);
  CRGB c = particle->color_at(age_frac);

  uint16_t pos = scale16(particle->position, num_leds-1);
  strip[pos] |= c;
}

void drawFlash(Particle *particle, CRGB strip[], uint8_t num_leds) {
  uint8_t age_frac = particle->age_frac8(particle->age);
  CRGB c = particle->color_at(age_frac);
  for (int pos = 0; pos < num_leds; pos++) {
    strip[pos] |= c;
  }
}

#endif
