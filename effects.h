#ifndef EFFECTS_H
#define EFFECTS_H

#include "particle.h"

ustd::array<Particle*> particles = ustd::array<Particle*>(5);

void addGlitter(fract8 chance) 
{
  if (random8() >= chance)
    return;

  Particle *particle = new Particle(random16(), CRGB::White, 18);
  particles.add(particle);
}

void addFlash(fract8 chance) 
{
  if (random8() >= chance)
    return;

  Particle *particle = new Particle(random16(), CRGB::Blue, 150, drawFlash);
  particles.add(particle);
}

void addDrop(fract8 chance)
{
  if( random8() >= chance)
    return;

   uint8_t hue = random8();
   CRGB color = CHSV(hue, 255, 255);
   Particle *particle = new Particle(65535, color, 90);
   particle->velocity = -500;
   particle->gravity = -10;
   particles.add(particle);
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
  particles.add(p1);
  particles.add(p2);
  particles.add(p3);

  for (int i=0; i<20; i++)
  {
    Particle *p = new Particle(pos, color, 60);
    // p->palette = PartyColors_p;
    uint16_t vel = random8();
    if (vel < 128)
      p->velocity = -2 * vel;
    else
      p->velocity = 2 * (vel - 128);
    particles.add(p);
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
  particles.add(particle);
}

#endif

void animateParticles(uint32_t frame) {
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
