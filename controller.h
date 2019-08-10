#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <EasyButton.h>
#include "pattern.h"
#include "palette.h"
#include "led_strip.h"
#include "lcd.h"

#define X_AXIS_PIN 20
#define Y_AXIS_PIN 21

#define BUTTON_PIN_1   2
#define BUTTON_PIN_2   3
#define BUTTON_PIN_3   4
#define BUTTON_PIN_4   5
EasyButton button1(BUTTON_PIN_1);
EasyButton button2(BUTTON_PIN_2);
EasyButton button3(BUTTON_PIN_3);
EasyButton button4(BUTTON_PIN_4);

typedef uint8_t TubeId;

TubeId tubeId = 0;
TubeId masterTubeId = 0;

typedef uint16_t CommandId;
const static CommandId COMMAND_UPDATE = 0x411;
const static CommandId COMMAND_RESET = 0x911;
const static CommandId COMMAND_FIREWORK = 0x321;
const static CommandId COMMAND_HELLO = 0x000;
const static CommandId COMMAND_OPTIONS = 0x123;

void onButton1Pressed()
{
  Serial.println(F("button 1 pressed"));
  // throwFirework(255);
  // sendRadioMessage(COMMAND_FIREWORK);
}

void onButton2Pressed()
{
  Serial.println(F("button 2 pressed"));
  // throwFirework(255);
  // sendRadioMessage(COMMAND_FIREWORK);
}

void onButton3Pressed()
{
  Serial.println(F("button 3 pressed"));
  // throwFirework(255);
  // sendRadioMessage(COMMAND_FIREWORK);
}

void onButton4Pressed()
{
  Serial.println(F("button 4 pressed"));
  // throwFirework(255);
  // sendRadioMessage(COMMAND_FIREWORK);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
AnimationFn gPatterns[] = { 
  drawNoise, 
  drawNoise, 
  drawNoise, 
  rainbow, 
  drawNoise, 
  drawNoise, 
  confetti,
  drawNoise, 
  confetti, 
  drawNoise, 
//  sinelon, 
  juggle, 
  drawNoise,
  confetti,
  drawNoise,
  bpm
};
/*
*/

uint8_t gPatternsCount = ARRAY_SIZE( gPatterns );

#define NEXT_PATTERN_TIME 53000
#define NEXT_PALETTE_TIME 27000

class PatternController {
  public:
    const static int FRAMES_PER_SECOND = 150;  // how often we animate, in frames per second
    const static int REFRESH_PERIOD = 1000 / FRAMES_PER_SECOND;  // how often we animate, in milliseconds

    ustd::array<VirtualStrip*> vstrips = ustd::array<VirtualStrip*>(3);
    Timer patternTimer;
    Timer paletteTimer;
    Timer graphicsTimer;
    Lcd *lcd;
    LEDs *led_strip;
    BeatController *beats;

    uint8_t x_axis;
    uint8_t y_axis;
    uint16_t b;

  PatternController(BeatController *beats) {
    this->lcd = new Lcd();
    this->led_strip = new LEDs();
    this->beats = beats;
  }
  
  void setup()
  {
    this->lcd->setup();
    this->led_strip->setup();

    this->setPattern(0, random8(gGradientPaletteCount), randomSyncMode());
    this->patternTimer.start(NEXT_PATTERN_TIME);
    this->paletteTimer.start(NEXT_PALETTE_TIME);
    Serial.println("Graphics: ok");

    button1.begin();
    button2.begin();
    button3.begin();
    button4.begin();
    button1.onPressed(onButton1Pressed);
    button2.onPressed(onButton2Pressed);
    button3.onPressed(onButton3Pressed);
    button4.onPressed(onButton4Pressed);
    Serial.println(F("Controls: ok"));
  }

  void update()
  {
    currentState.timer += globalTimer.delta_millis;

    button1.read();
    button2.read();
    button3.read();
    button4.read();
    this->x_axis = analogRead(X_AXIS_PIN) >> 3;
    this->y_axis = analogRead(Y_AXIS_PIN) >> 3;

    currentState.bpm = this->beats->bpm;
    currentState.frame = this->beats->frame;

    if (this->patternTimer.ended()) {
      this->nextPattern();
      this->patternTimer.snooze(NEXT_PATTERN_TIME);
    }

    if (this->paletteTimer.ended()) {
      this->nextPalette();
      this->paletteTimer.snooze(NEXT_PALETTE_TIME);
    }

    if (this->graphicsTimer.every(REFRESH_PERIOD)) {
      this->updateGraphics();
    }

    if (this->lcd->active) {
      this->lcd->size(1);
      this->lcd->write(0,56, currentState.frame);
      this->lcd->write(80,56, this->x_axis);
      this->lcd->write(100,56, this->y_axis);
      this->lcd->show();

      this->lcd->update();
    }
  }

  void setPalette(uint8_t palette_id) {
    this->setPattern(currentState.pattern, palette_id, (SyncMode)currentState.sync);
  }

  void startPattern(uint8_t pattern_id, uint8_t palette_id, SyncMode sync) {
    if (pattern_id == currentState.pattern && palette_id == currentState.palette_id)
      return;
    this->setPattern(pattern_id, palette_id, sync);
  }

  void setPattern(uint8_t pattern_id, uint8_t palette_id, SyncMode sync) {
    currentState.pattern = pattern_id;
    currentState.palette_id = palette_id;
    currentState.sync = (uint8_t)sync;
  
    Serial.print(F("new pattern: "));
    printState(&currentState);
    Serial.println();
  
    unsigned int len = vstrips.length();
    for (unsigned int i = 0; i < len; i++) {
      vstrips[i]->fadeOut();
    }
  
    Animation animation;
    animation.animate = gPatterns[currentState.pattern];
    animation.palette = gGradientPalettes[currentState.palette_id];
    animation.sync = sync;
    animation.effect = None;
    
    VirtualStrip *vstrip = new VirtualStrip();
    vstrip->load(animation);
    this->vstrips.add(vstrip);
  }

  SyncMode randomSyncMode() {
    return Swing;

    uint8_t r = random() % 128;
    if (r < 40)
      return SinDrift;
    if (r < 65)
      return Pulse;
    if (r < 72)
      return Swing;
    if (r < 84)
      return SwingDrift;
    return All;
  }

  void nextPalette() {
    // If we're not in control - don't do anything
    if (masterTubeId)
      return;

    this->setPalette(random8(gGradientPaletteCount));
  }

  void nextPattern() {
    // If we're not in control - don't do anything
    if (masterTubeId)
      return;
  
    // add one to the current pattern number, and wrap around at the end
    uint8_t p = addmod8(currentState.pattern, 1, gPatternsCount);
    SyncMode s = this->randomSyncMode();
    if (gPatterns[currentState.pattern] == biwave)
      s = All;
    this->startPattern(p, random8(gGradientPaletteCount), s);
    currentState.timer = 0;
  }

  void updateGraphics() {
    animateParticles(currentState.frame);

    bool first = 1;   
    unsigned int len = this->vstrips.length();
    for (unsigned i=len; i > 0; i--) {
      VirtualStrip *vstrip = this->vstrips[i-1];

      vstrip->update(currentState.frame);
      vstrip->blend(this->led_strip->leds, first);
      first = 0;

      // Eliminate dead strips
      if (vstrip->fade == Dead) {
        delete vstrip;
        this->vstrips.erase(i-1);
        continue;
      }
    }
  }

  void onCommandReceived(uint8_t fromId, CommandId command, byte *data) {
    Serial.print(F("From "));
    Serial.print(fromId);
    Serial.print(F(": "));
    
    switch (command) {
      case COMMAND_FIREWORK:
        Serial.println(F("fireworks"));
        // throwFirework(255);
        return;
  
      case COMMAND_RESET:
        Serial.println(F("reset"));
        return;
  
      case COMMAND_HELLO:
        Serial.println(F("hello"));
        return;
  
      case COMMAND_OPTIONS: {
        Serial.println(F("options"));
        uint8_t options;
        memcpy(&options, data, sizeof(options));
        // debug.debugging = options & 1;
        return;
      }
  
      case COMMAND_UPDATE:
        TubeState state;
        memcpy(&state, data, sizeof(TubeState));
        printState(&state);
  
        if (fromId < masterTubeId) {
          Serial.println(F(" (ignoring)"));
          return;
        } 
        Serial.println(F(" (obeying)"));
  
        this->startPattern(state.pattern, state.palette_id, (SyncMode)state.sync);
        this->beats->sync(state.bpm, state.frame);
        currentState = state;    
        return;
    }
  
    Serial.print(F("UNKNOWN "));
    Serial.println(command, HEX);
  }

};

#endif
