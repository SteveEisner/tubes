#ifndef EFFECTS_H
#define EFFECTS_H

#include "particle.h"


typedef enum EffectMode {
  None=0,
  Glitter=1,
  Bubble=2,
  Beatbox=3,
  Spark=4,
  Flash=5,
} EffectMode;


void addGlitter(fract8 chance) 
{
  if (random8() >= chance)
    return;

  addParticle(new Particle(random16(), CRGB::White, 128));
}

void addSpark(fract8 chance) 
{
  if (random8() >= chance)
    return;

  Particle *particle = new Particle(random16(), CRGB::White, 64);
  uint8_t r = random8();
  if (r > 128)
    particle->velocity = r;
  else
    particle->velocity = -(128 + r);
  addParticle(particle);
}

void addBeatbox(fract8 chance, CRGB color=CRGB::White) 
{
  if (random8() >= chance)
    return;

  Particle *particle = new Particle(random16(), color, 256, drawBeatbox);
  addParticle(particle);
}

void addBubble(fract8 chance, CRGB color=CRGB::White) 
{
  if (random8() >= chance)
    return;

  Particle *particle = new Particle(random16(), color, 1024, drawPop);
  particle->velocity = random16(0, 40) - 20;
  addParticle(particle);
}

void addFlash(fract8 chance) 
{
  if (random8() >= chance)
    return;

  addParticle(new Particle(random16(), CRGB::Blue, 512, drawFlash));
}

void addDrop(fract8 chance)
{
  if( random8() >= chance)
    return;

   uint8_t hue = random8();
   CRGB color = CHSV(hue, 255, 255);
   Particle *particle = new Particle(65535, color, 360);
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

void animateParticles(BeatFrame_24_8 frame) {
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

void drawParticles(CRGB strip[], uint8_t num_leds) {
  unsigned int len = particles.length();
  for (unsigned i=len; i > 0; i--) {
    Particle *particle = particles[i-1];
    particle->drawFn(particle, strip, num_leds);
  }
}

#endif
