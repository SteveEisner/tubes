#ifndef PATTERNS_H
#define PATTERNS_H

#include "palette.h"
#include "virtual_strip.h"

void rainbow(VirtualStrip *strip) 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( strip->leds, strip->num_leds, strip->hue, 7);
}

void particleTest(VirtualStrip *strip)
{
  fill_solid( strip->leds, strip->num_leds, CRGB::Black);
  fill_solid( strip->leds, 2, strip->palette_color(0, strip->hue));
}

void solidBlack(VirtualStrip *strip)
{
  fill_solid( strip->leds, strip->num_leds, CRGB::Black);
}

void solidWhite(VirtualStrip *strip) 
{
  fill_solid( strip->leds, strip->num_leds, CRGB::White);
}

void solidRed(VirtualStrip *strip) 
{
  fill_solid( strip->leds, strip->num_leds, CRGB::Red);
}

void solidBlue(VirtualStrip *strip) 
{
  fill_solid( strip->leds, strip->num_leds, CRGB::Blue);
}

void confetti(VirtualStrip *strip) 
{
  strip->darken(8);
  
  int pos = random16(strip->num_leds);
  strip->leds[pos] += strip->palette_color(random8(64), strip->hue);
}

uint16_t random_offset = random16();

void biwave(VirtualStrip *strip)
{
  uint16_t frame = (uint16_t)(swing(strip->frame % 512) << 7);

  uint16_t l = sin16(frame+random_offset) + 32768;
  uint16_t r = sin16(frame+random_offset+4000) + 32768;
  
  uint16_t p1 = scale16(l, strip->num_leds-1);
  uint16_t p2 = scale16(r, strip->num_leds-1);
  if (p2 < p1) {
    uint16_t t = p1;
    p1 = p2;
    p2 = t;
  }

  strip->fill(CRGB::Black);
  for (uint16_t p = p1; p <= p2; p++) {
    strip->leds[p] = strip->palette_color(strip->hue);
  }
}

void sinelon(VirtualStrip *strip) 
{
  // a colored dot sweeping back and forth, with fading trails
  strip->darken(30);

  int pos = scale16(sin16( strip->frame << 5 ) + 32768, strip->num_leds-1);   // beatsin16 re-implemented
  strip->leds[pos] += strip->hue_color();
}

void bpm(VirtualStrip *strip) 
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  CRGBPalette16 palette = PartyColors_p;

  uint8_t beat = strip->bpm_sin16(64, 255);
  for (int i = 0; i < strip->num_leds; i++) {
    strip->leds[i] = ColorFromPalette(palette, strip->hue+(i*2), beat-strip->hue+(i*10));
  }
}

void juggle(VirtualStrip *strip) 
{
  // eight colored dots, weaving in and out of sync with each other
  strip->darken(20);

  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    CRGB c = strip->palette_color(dothue + strip->hue);
    // c = CHSV(dothue, 200, 255);
    strip->leds[beatsin16( i+7, 0, strip->num_leds-1 )] |= c;
    dothue += 32;
  }
}

uint8_t noise[MAX_VIRTUAL_LEDS];

void fillnoise8(uint32_t frame, uint8_t num_leds) {
  uint16_t scale = 17;
  uint8_t dataSmoothing = 240;
  
  for (int i = 0; i < num_leds; i++) {
    uint8_t data = inoise8(i * scale, frame>>2);

    // The range of the inoise8 function is roughly 16-238.
    // These two operations expand those values out to roughly 0..255
    data = qsub8(data,16);
    data = qadd8(data,scale8(data,39));

    uint8_t olddata = noise[i];
    uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
    noise[i] = newdata;
  }
}

void drawNoise(VirtualStrip *strip)
{
  // generate noise data
  fillnoise8(strip->frame >> 2, strip->num_leds);

  for(int i = 0; i < strip->num_leds; i++) {
    CRGB color = strip->palette_color(noise[i], strip->hue);
    strip->leds[i] = color;
  }
}

typedef struct {
  BackgroundFn backgroundFn;
  ControlParameters control;
} PatternDef;


// List of patterns to cycle through.  Each is defined as a separate function below.
PatternDef gPatterns[] = { 
  {drawNoise, {ShortDuration}},
  {drawNoise, {ShortDuration}},
  {drawNoise, {MediumDuration}},
  {drawNoise, {MediumDuration}},
  {drawNoise, {MediumDuration}},
  {drawNoise, {LongDuration}},
  {drawNoise, {LongDuration}},
  {rainbow, {ShortDuration}},
  {confetti, {ShortDuration}},
  {confetti, {MediumDuration}},
  {juggle, {ShortDuration}},
  {bpm, {ShortDuration}},
  {bpm, {MediumDuration, HighEnergy}},
};
/*
*/
const uint8_t gPatternCount = ARRAY_SIZE(gPatterns);

#endif
