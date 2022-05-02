#ifndef EFFECTS_H
#define EFFECTS_H

#include "particle.h"
#include "virtual_strip.h"

void addGlitter(CRGB color=CRGB::White, PenMode pen=Draw) 
{
  addParticle(new Particle(random16(), color, pen, 128));
}

void addSpark(CRGB color=CRGB::White, PenMode pen=Draw) 
{
  Particle *particle = new Particle(random16(), color, pen, 64);
  uint8_t r = random8();
  if (r > 128)
    particle->velocity = r;
  else
    particle->velocity = -(128 + r);
  addParticle(particle);
}

void addBeatbox(CRGB color=CRGB::White, PenMode pen=Draw) 
{
  Particle *particle = new Particle(random16(), color, pen, 256, drawBeatbox);
  addParticle(particle);
}

void addBubble(CRGB color=CRGB::White, PenMode pen=Draw) 
{
  Particle *particle = new Particle(random16(), color, pen, 1024, drawPop);
  particle->velocity = random16(0, 40) - 20;
  addParticle(particle);
}

void addFlash(CRGB color=CRGB::Blue, PenMode pen=Draw) 
{
  addParticle(new Particle(random16(), color, pen, 256, drawFlash));
}

void addDrop(CRGB color, PenMode pen=Draw)
{
   Particle *particle = new Particle(65535, color, pen, 360);
   particle->velocity = -500;
   particle->gravity = -10;
   addParticle(particle);
}


#ifdef UNUSED

void addFireworkAt(uint16_t pos, CRGB color)
{
  Particle *p1 = new Particle(pos, CRGB::White, 20);
  Particle *p2 = new Particle(pos, CRGB::White, 20);
  Particle *p3 = new Particle(pos, CRGB::White, 20);
  p1->velocity = 0;
  p2->velocity = 40 + random8(40);
  p3->velocity = -40 - random8(40);
  addParticle(p1);
  addParticle(p2);
  addParticle(p3);

  for (int i=0; i<20; i++)
  {
    Particle *p = new Particle(pos, color, 60);
    // p->palette = PartyColors_p;
    uint16_t vel = random8();
    if (vel < 128)
      p->velocity = -2 * vel;
    else
      p->velocity = 2 * (vel - 128);
    addParticle(p);
  }
}

void addFirework(fract8 chance)
{
  if( random8() >= chance)
    return;

  uint8_t hue = random8();
  CRGB color = CHSV(hue, 255, 255);
  addFireworkAt(random16(), color);
}

void explode(class Particle *particle)
{
  addFireworkAt(particle->position, particle->color);
}

void throwFirework(fract8 chance)
{
  if( random8() > chance)
    return;

  CRGB color = CHSV(random8(), 255, 255);
  
  Particle *particle = new Particle(0, color, 100);
  particle->velocity = 700 + random16(250);
  particle->gravity = -10;
  particle->die_fn = explode;
  addParticle(particle);
}

#endif

class Effects {
  public:
    EffectMode effect=None;
    PenMode pen=Draw;
    BeatPulse beat;
    uint8_t chance;

  void load(EffectParameters &params) {
    this->effect = params.effect;
    this->pen = params.pen;
    this->beat = params.beat;
    this->chance = params.chance;
  }

  void update(VirtualStrip *strip, BeatFrame_24_8 beat_frame, BeatPulse beat_pulse) {
    if (!this->beat || beat_pulse & this->beat) {

      if (random8() <= this->chance) {
        CRGB color = strip->palette_color(random8());
  
        switch (this->effect) {
          case None:
            break;
      
          case Glitter:
            addGlitter(color, this->pen);
            break;
      
          case Beatbox1:
          case Beatbox2:
            addBeatbox(color, this->pen);
            if (this->effect == Beatbox2)
              addBeatbox(color, this->pen);
            break;
      
          case Bubble:
            addBubble(color, this->pen);
            break;
      
          case Spark:
            addSpark(color, this->pen);
            break;
      
          case Flash:
            addFlash(CRGB::White, this->pen);
            break;  
        }
      }
    }

    this->animate(beat_frame, beat_pulse);
  }

  void animate(BeatFrame_24_8 frame, uint8_t beat_pulse) {
    unsigned int len = particles.length();
    for (unsigned i=len; i > 0; i--) {
      Particle *particle = particles[i-1];
  
      particle->update(frame);
      if (particle->age > particle->lifetime) {
        delete particle;
        particles.erase(i-1);
        continue;
      }
    }
  }

  void draw(CRGB strip[], uint8_t num_leds) {
    uint8_t len = particles.length();
    for (uint8_t i=0; i<len; i++) {
      Particle *particle = particles[i];
      particle->drawFn(particle, strip, num_leds);
    }
  }
  
};


typedef struct {
  EffectParameters params;
  ControlParameters control;
} EffectDef;


static const EffectDef gEffects[] STATIC_MEM = {
  {{None}, {LongDuration}},
  {{Flash, Brighten, Beat, 40}, {MediumDuration, MediumEnergy}},
  {{Flash, Darken, TwoBeats, 40}, {MediumDuration, MediumEnergy}},
  {{Flash, Brighten, Measure}, {ShortDuration, HighEnergy}},
  {{Flash, Brighten, Phrase}, {MediumDuration, HighEnergy}},
  {{Flash, Darken, Measure}, {ShortDuration, LowEnergy}},
  {{Glitter, Brighten, Eighth, 40}, {ShortDuration, LowEnergy}},
  {{Glitter, Brighten, Eighth, 80}, {MediumDuration, MediumEnergy}},
  {{Glitter, Brighten, Eighth, 40}, {MediumDuration, HighEnergy}},
  {{Glitter, Darken, Eighth, 40}, {MediumDuration, LowEnergy}},

  {{Glitter, Draw, Eighth, 10}, {LongDuration, LowEnergy}},
  {{Glitter, Draw, Eighth, 120}, {MediumDuration, LowEnergy}},
  {{Glitter, Invert, Eighth, 40}, {ShortDuration, LowEnergy}},
  {{Beatbox2, Black}, {MediumDuration, LowEnergy}},
  {{Beatbox2, Draw}, {ShortDuration, HighEnergy}},
  {{Bubble, Darken}, {MediumDuration, LowEnergy}},
  {{Bubble, Brighten}, {MediumDuration, LowEnergy}},
  {{Glitter, Darken, Eighth, 120}, {MediumDuration, LowEnergy}},
  {{Glitter, Flicker, Eighth, 120}, {MediumDuration, LowEnergy}},
};
const uint8_t gEffectCount = ARRAY_SIZE(gEffects);


#endif
