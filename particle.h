#ifndef PARTICLE_H
#define PARTICLE_H

#define MAX_PARTICLES 20
#undef PARTICLE_PALETTES

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

#ifdef PARTICLE_PALETTES
    CRGBPalette16 palette;   // 48 bytes per particle!?
#endif

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

  uint16_t age_frac16(BeatFrame_24_8 age)
  {
    if (age >= this->lifetime)
      return 65535;
    uint32_t a = age * 65536;
    return a / this->lifetime;
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

  CRGB color_at(uint16_t age_frac) {
    // Particles get dimmer with age
    uint8_t a = age_frac >> 8;
    uint8_t brightness = scale8((uint8_t)(this->brightness>>8), 255-a);

#ifdef PARTICLE_PALETTES
    // a black pattern actually means to use the current palette
    if (this->color == CRGB(0,0,0))
      return ColorFromPalette(this->palette, a, brightness);
#endif

    uint8_t r = scale8(this->color.r, brightness);
    uint8_t g = scale8(this->color.g, brightness);
    uint8_t b = scale8(this->color.b, brightness);
    return CRGB(r,g,b);
  }
  
};

ustd::array<Particle*> particles = ustd::array<Particle*>(5);

void addParticle(Particle *particle) {
  particles.add(particle);
  if (particles.length() > MAX_PARTICLES) {
    Particle *old_particle = particles[0];
    delete old_particle;
    particles.erase(0);
  }
}


void drawFlash(Particle *particle, CRGB strip[], uint8_t num_leds) {
  uint16_t age_frac = particle->age_frac16(particle->age);
  CRGB c = particle->color_at(age_frac);
  for (int pos = 0; pos < num_leds; pos++) {
    strip[pos] |= c;
  }
}

void drawPoint(Particle *particle, CRGB strip[], uint8_t num_leds) {
  uint16_t age_frac = particle->age_frac16(particle->age);
  CRGB c = particle->color_at(age_frac);

  uint16_t pos = scale16(particle->position, num_leds-1);
  strip[pos] |= c;
}

void drawRadius(CRGB strip[], uint8_t num_leds, uint16_t pos, uint8_t radius, CRGB c, bool dim=true) {
  for (int i = 0; i < radius; i++) {
    uint8_t bright = dim ? ((radius-i) * 255) / radius : 255;
    nscale8(&c, 1, bright);

    uint8_t y = pos - i;
    if (y >= 0 && y < num_leds)
      strip[y] |= c;

    if (i == 0)
      continue;

    y = pos + i;
    if (y >= 0 && y < num_leds)
      strip[y] |= c;
  }
}

void drawPop(Particle *particle, CRGB strip[], uint8_t num_leds) {
  uint16_t age_frac = particle->age_frac16(particle->age);
  CRGB c = particle->color_at(age_frac);
  uint16_t pos = scale16(particle->position, num_leds-1);
  uint8_t radius = scale16((sin16(age_frac/2) - 32768) * 2, 8);

  drawRadius(strip, num_leds, pos, radius, c);
}

void drawBeatbox(Particle *particle, CRGB strip[], uint8_t num_leds) {
  uint16_t age_frac = particle->age_frac16(particle->age);
  CRGB c = particle->color_at(age_frac);
  uint16_t pos = scale16(particle->position, num_leds-1);
  uint8_t radius = 5;

  drawRadius(strip, num_leds, pos, radius, c, false);
}


#endif
