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

BeatFrame_24_8 swing(BeatFrame_24_8 frame) {
  uint16_t fr = (frame & 0x3FF); // grab 4 beats
  if (fr < 256)
    fr = ease8InOutApprox(fr) << 2;
  else
    fr = 0x3FF;
  
  return (frame & 0xFC00) + fr;  // recompose it
}

class VirtualStrip {
  const static uint16_t DEFAULT_BRIGHTNESS = 192;

  public:
    CRGB leds[MAX_LEDS];
    uint8_t num_leds;
    uint8_t brightness;

    // Fade in/out
    VirtualStripFade fade;
    uint16_t fader;
    uint8_t fade_speed;

    // Pattern parameters
    Animation animation;
    uint32_t frame;
    uint8_t beat;
    uint16_t beat16;  // 8 bits of beat and 8 bits of fractional
    uint8_t hue;

  VirtualStrip(uint8_t num_leds=MAX_LEDS)
  {
    this->fade = Dead;
    this->num_leds = num_leds;
  }

  void load(Animation &animation)
  {
    this->animation = animation;
    this->fade = FadeIn;
    this->fader = 0;
    this->fade_speed = animation.fade_speed;
    this->brightness = DEFAULT_BRIGHTNESS;
  }

  void fadeOut(uint8_t fade_speed=DEFAULT_FADE_SPEED)
  {
    if (this->fade == Dead)
      return;
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

  void update(BeatFrame_24_8 frame)
  {
    if (this->fade == Dead)
      return;

    this->frame = frame;
    switch (this->animation.sync) {
      case All:
        break;  

      case SinDrift:
        // Drift slightly
        this->frame = frame + (beatsin16( 5 ) >> 6);
        break;

      case Swing:
        // Swing the beat
        this->frame = swing(frame);
        break;

      case SwingDrift:
        // Swing the beat AND drift slightly
        this->frame = swing(frame) + (beatsin16( 5 ) >> 6);
        break;

      case Pulse:
        // Pulsing from 30 - 210 brightness
        this->brightness = scale8(beatsin8( 10 ), 180) + 30;
        break;
    }
    this->beat16 = (this->frame << 8);
    this->beat = (this->frame >> 8) % 16;
    this->hue = (this->frame >> 4) % 256;

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

    switch (this->animation.effect) {
      case None:
        break;  

      case Glitter:
        addGlitter(25);
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

  void blend(CRGB strip[], uint8_t brightness, bool overwrite=0) {
    if (this->fade == Dead)
      return;

    brightness = scale8(this->brightness, brightness);

    for (unsigned i=0; i < this->num_leds; i++) {
      CRGB c = this->leds[i];
      nscale8x3(c.r, c.g, c.b, brightness);
      nscale8x3(c.r, c.g, c.b, this->fader>>8);
      if (overwrite)
        strip[i] = c;
      else
        strip[i] |= c;
    }
  }
  
  uint8_t bpm_sin16( uint16_t lowest=0, uint16_t highest=65535 )
  {
    uint16_t beatsin = sin16( this->beat16 >> 1 ) + 32768;
    uint16_t rangewidth = highest - lowest;
    uint16_t scaledbeat = scale16( beatsin, rangewidth );
    uint16_t result = lowest + scaledbeat;
    return result;
  }

};

#endif
