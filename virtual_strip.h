#ifndef VIRTUAL_STRIP_H
#define VIRTUAL_STRIP_H

#include <FastLED.h>
#include "effects.h"
#include "led_strip.h"

#define DEFAULT_FADE_SPEED 100

class VirtualStrip;
typedef void (*AnimationFn)(VirtualStrip *strip);

typedef enum EffectMode {
  None=0,
  Glitter=1
} EffectMode;

class Animation {
  public:
    AnimationFn animate;
    CRGBPalette16 palette;
    SyncMode sync=All;
    EffectMode effect=Glitter;
    uint8_t fade_speed=DEFAULT_FADE_SPEED;
};

typedef enum VirtualStripFade {
  Steady=0,
  FadeIn=1,
  FadeOut=2,
  Dead=99,
} VirtualStripFade;

uint32_t swing(uint32_t frame) {
  fract8 fr = (frame & 0xFF);
  if (fr < 64)
    fr = ease8InOutApprox(fr << 2);
  else
    fr = 255;
  
  return (frame & 0xFF00) + fr;
}

class VirtualStrip {
  const static uint16_t DEFAULT_BRIGHTNESS = 144;

  public:
    CRGB leds[MAX_LEDS];
    uint8_t num_leds = MAX_LEDS;
    uint8_t brightness;

    // Fade in/out
    VirtualStripFade fade;
    uint16_t fader;
    uint8_t fade_speed;

    // Pattern parameters
    Animation animation;
    uint32_t frame;
    uint8_t beat;
    uint16_t beat16;
    uint8_t hue;

  VirtualStrip(Animation &animation)
  {
    this->animation = animation;
    this->fade = FadeIn;
    this->fader = 0;
    this->fade_speed = animation.fade_speed;
    this->brightness = DEFAULT_BRIGHTNESS;
  }

  ~VirtualStrip() {
    destroyParticles(this);
  }

  void fadeOut(uint8_t fade_speed=DEFAULT_FADE_SPEED)
  {
    this->fade = FadeOut;
    this->fade_speed = fade_speed;
  }

  void darken(uint8_t amount=10)
  {
    fadeToBlackBy( this->leds, this->num_leds, amount);
  }

  void fill(CRGB crgb) 
  {
    fill_solid( this->leds, this->num_leds, crgb);

  }

  void update(uint32_t frame)
  {
    this->frame = frame;
    switch (this->animation.sync) {
      case All:
        break;  

      case SinDrift:
        this->frame = frame + beatsin8( 5 );
        break;

      case Swing:
        this->frame = swing(frame);
        break;

      case SwingDrift:
        this->frame = swing(frame) + beatsin8( 5 );
        break;

      case Pulse:
        this->brightness = scale8(beatsin8( 10 ), 128) + 64;
        break;
    }

    switch (this->fade) {
      case Steady:
      case Dead:
        break;
        
      case FadeIn:
        if (65535 - this->fader < this->fade_speed) {
          this->fader = 65535;
          this->fade = Steady;
        } else {
          this->fader += this->fade_speed;
        }
        break;
        
      case FadeOut:
        if (this->fader < this->fade_speed) {
          this->fader = 0;
          this->fade = Dead;
          return;
        } else {
          this->fader -= this->fade_speed;
        }
        break;
    }

    this->beat16 = (this->frame << 9);  // should be 10, but halving it...
    this->beat = (this->frame >> 6) % 16; // 64 frames per beat
    this->hue = (this->frame >> 2) % 256;

    switch (this->animation.effect) {
      case None:
        break;  

      case Glitter:
        addGlitter(25, this);
        break;
    }

    // Animate this virtual strip
    this->animation.animate(this);
  }

  CRGB palette_color(uint8_t c, uint8_t offset=0) {
    return ColorFromPalette( this->animation.palette, c + offset );
  }

  CRGB hue_color(uint8_t offset=0, uint8_t saturation=255, uint8_t value=192) {
    return CHSV(this->hue + offset, saturation, value);
  }

  void blend(CRGB strip[], bool overwrite=0) {
    for (unsigned i=0; i < this->num_leds; i++) {
      CRGB c = this->leds[i];
      nscale8x3(c.r, c.g, c.b, this->brightness);
      nscale8x3(c.r, c.g, c.b, this->fader>>8);
      if (overwrite)
        strip[i] = c;
      else
        strip[i] |= c;
    }

    unsigned int len = particles.length();
    for (unsigned i=len; i > 0; i--) {
      Particle *particle = particles[i-1];
      if (particle->owner != this)
        continue;
  
      uint16_t pos = scale16(particle->position, this->num_leds-1);
      uint32_t age = particle->age;

      CRGB c = particle->color_at(age);
      nscale8x3(c.r, c.g, c.b, this->fader>>8);
      strip[pos] |= c;
    }
  
  }
  
  uint8_t beatsin16( uint16_t lowest=0, uint16_t highest=65535 )
  {
    uint16_t beatsin = sin16( this->beat16 ) + 32768;
    uint16_t rangewidth = highest - lowest;
    uint16_t scaledbeat = scale16( beatsin, rangewidth );
    uint16_t result = lowest + scaledbeat;
    return result;
  }

};

#endif
