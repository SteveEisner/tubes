#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "beats.h"

#include "pattern.h"
#include "palette.h"
#include "effects.h"
#include "led_strip.h"
#include "lcd.h"
#include "radio.h"

#include <EasyButton.h>

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

const static uint8_t DEFAULT_MASTER_BRIGHTNESS = 144;

const static CommandId COMMAND_UPDATE = 0x411;
const static CommandId COMMAND_RESET = 0x911;
const static CommandId COMMAND_FIREWORK = 0x321;
const static CommandId COMMAND_HELLO = 0x000;
const static CommandId COMMAND_OPTIONS = 0x123;
const static CommandId COMMAND_BRIGHTNESS = 0x888;


// List of patterns to cycle through.  Each is defined as a separate function below.
BackgroundFn gPatterns[] = { 
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
  juggle, 
  drawNoise,
  confetti,
  drawNoise,
  bpm
};
/*
*/



typedef struct {
  bool debugging;
  uint8_t brightness;
} ControllerOptions;

uint8_t gPatternsCount = ARRAY_SIZE( gPatterns );

#define NEXT_PATTERN_TIME 53000
#define NEXT_PALETTE_TIME 27000

#define NUM_VSTRIPS 3

class PatternController : public MessageReceiver {
  public:
    const static int FRAMES_PER_SECOND = 150;  // how often we animate, in frames per second
    const static int REFRESH_PERIOD = 1000 / FRAMES_PER_SECOND;  // how often we animate, in milliseconds

    uint8_t num_leds;
    VirtualStrip *vstrips[NUM_VSTRIPS];
    uint8_t next_vstrip = 0;
    
    Timer patternTimer;
    Timer paletteTimer;
    Timer graphicsTimer;
    Timer updateTimer;
    Timer slaveTimer;

    Lcd *lcd;
    LEDs *led_strip;
    BeatController *beats;
    Radio *radio;
    Effects *effects;

    uint8_t x_axis;
    uint8_t y_axis;
    uint16_t b;
    ControllerOptions options;

  PatternController(uint8_t num_leds, BeatController *beats, Radio *radio) {
    this->num_leds = num_leds;
    this->lcd = new Lcd();
    this->led_strip = new LEDs(num_leds);
    this->beats = beats;
    this->radio = radio;
    this->effects = new Effects();

    for (uint8_t i=0; i < NUM_VSTRIPS; i++) {
      this->vstrips[i] = new VirtualStrip();
    }
  }
  
  void setup()
  {
    this->options.debugging = 1;
    this->options.brightness = DEFAULT_MASTER_BRIGHTNESS;
    
    this->lcd->setup();
    this->led_strip->setup();

    this->setPattern(0, random8(gGradientPaletteCount), randomSyncMode());
    this->patternTimer.start(NEXT_PATTERN_TIME);
    this->paletteTimer.start(NEXT_PALETTE_TIME);
    Serial.println(F("Graphics: ok"));

    button1.begin();
    button2.begin();
    button3.begin();
    button4.begin();
    Serial.println(F("Controls: ok"));

    this->radio->setup();
    this->radio->sendCommand(COMMAND_HELLO);

    this->slaveTimer.start(RADIO_SENDPERIOD * 3); // Assume we're a slave at first, just listen for a master.
    this->updateTimer.start(RADIO_SENDPERIOD); // Ready to send an update as soon as we're able to
  }

  void update()
  {
    currentState.timer += globalTimer.delta_millis;

    button1.read();
    button2.read();
    button3.read();
    button4.read();

    button1.read();
    if (button1.isPressed()) {
      this->onButton(1);
    }
    button2.read();
    if (button2.isPressed()) {
      this->onButton(2);
    }
    button1.read();
    if (button1.isPressed()) {
      this->onButton(3);
    }
    button1.read();
    if (button1.isPressed()) {
      this->onButton(4);
    }

 #ifdef USEJOYSTICK
    this->x_axis = analogRead(X_AXIS_PIN) >> 3;
    this->y_axis = analogRead(Y_AXIS_PIN) >> 3;

    if (this->x_axis > 100) {
      if (!this->options.debugging) {
        this->options.debugging = 1;
        this->radio->sendCommand(COMMAND_OPTIONS, &this->options, sizeof(this->options));
      }
      this->beats->set_bpm(DEFAULT_BPM << 8);
    } else if (this->x_axis < 30)
      if (this->options.debugging) {
        this->options.debugging = 0;
        this->radio->sendCommand(COMMAND_OPTIONS, &this->options, sizeof(this->options));
      }
#endif

    this->readSerial();

    currentState.bpm = this->beats->bpm;
    currentState.beat_frame = this->beats->frac;
    uint8_t beat = this->beats->frac >> 8;

    if (this->isMaster()) {
      if (this->radio->masterTubeId) {
        Serial.println(F("I have no master"));
        this->radio->masterTubeId = 0;
      }

      // Only change patterns on a 32-beat phrase
      if ((beat % 32 == 0) && this->patternTimer.ended()) {
        this->nextPattern();
      }

      // Change palette any time
      if (this->paletteTimer.ended()) {
        this->nextPalette();
      }
  
      // run periodic timer
      if (this->updateTimer.ended()) {
        Serial.print(F("Update "));
        printState(&currentState);
        Serial.print(F(" "));
  
        if (this->radio->sendCommand(COMMAND_UPDATE, &currentState, sizeof(currentState))) {
          this->radio->radioFailures = 0;
          if (currentState.timer < RADIO_SENDPERIOD) {
            this->updateTimer.snooze(RADIO_SENDPERIOD / 4);
          } else {
            this->updateTimer.snooze(RADIO_SENDPERIOD);
          }
        } else {
          // might have been a collision.  Back off by a small amount determined by ID
          Serial.println(F("Radio update failed"));
          this->updateTimer.snooze( (this->radio->tubeId & 0x7F) * 1000 );
          this->radio->radioFailures++;
          if (this->radio->radioFailures > 100) {
            this->radio->setup();
            this->radio->radioRestarts++;
          }
        }
      }
    }

    this->radio->receiveCommands(this);

    if (this->graphicsTimer.every(REFRESH_PERIOD)) {
      this->updateGraphics();
    }

    if (this->lcd->active) {
      this->lcd->size(1);
      this->lcd->write(0,56, currentState.beat_frame);
      this->lcd->write(80,56, this->x_axis);
      this->lcd->write(100,56, this->y_axis);
      this->lcd->show();

      this->lcd->update();
    }
  }

  void onButton(uint8_t button) {
    if (button == 1) {
      Serial.print(F("Button "));
      Serial.println(button);
      this->radio->sendCommand(COMMAND_FIREWORK, NULL, 0);
      return;
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

  void optionsChanged() {
#ifdef MASTERCONTROL
    this->radio->sendCommand(COMMAND_OPTIONS, &options, sizeof(options));
#endif
  }

  void setBrightness(uint8_t brightness) {
    Serial.print(F("brightness "));
    Serial.println(brightness);

    this->options.brightness = brightness;
    this->optionsChanged();
  }

  void setDebugging(bool debugging) {
    this->options.debugging = debugging;
    this->optionsChanged();
  }
  
  void setPattern(uint8_t pattern_id, uint8_t palette_id, SyncMode sync) {
    currentState.pattern = pattern_id;
    currentState.palette_id = palette_id;
    currentState.sync = (uint8_t)sync;
  
    Serial.print(F("new pattern: "));
    printState(&currentState);
    Serial.println();
  
    for (uint8_t i = 0; i < NUM_VSTRIPS; i++) {
      this->vstrips[i]->fadeOut();
    }

    Background background;
    background.animate = gPatterns[currentState.pattern];
    background.palette = gGradientPalettes[currentState.palette_id];
    background.sync = sync;

    // re-use virtual strips to prevent heap fragmentation
    this->vstrips[this->next_vstrip]->load(background);
    EffectParameters params = gEffects[1];
    this->effects->load(params);
    this->next_vstrip = (this->next_vstrip + 1) % NUM_VSTRIPS;
  }

  SyncMode randomSyncMode() {
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

  bool isMaster() {
    return this->slaveTimer.ended();
  }

  void nextPalette() {
    // If we're not in control - don't do anything
    this->setPalette(random8(gGradientPaletteCount));
    this->paletteTimer.start(NEXT_PALETTE_TIME);
  }

  void nextPattern() {
    // add one to the current pattern number, and wrap around at the end
    uint8_t p = addmod8(currentState.pattern, 1, gPatternsCount);
    SyncMode s = this->randomSyncMode();
    if (gPatterns[currentState.pattern] == biwave)
      s = All;
    this->startPattern(p, random8(gGradientPaletteCount), s);
    currentState.timer = 0;

    this->patternTimer.start(NEXT_PATTERN_TIME);
  }

  void updateGraphics() {
    static BeatFrame_24_8 lastFrame = 0;
    BeatFrame_24_8 beat_frame = currentState.beat_frame;

    uint8_t beat_pulse = 0;
    for (int i = 0; i < 6; i++) {
      if ( (beat_frame >> (8+i)) != (lastFrame >> (8+i)))
        beat_pulse |= (1<<i);
    }
    lastFrame = beat_frame;

    VirtualStrip *first_strip = NULL;
    for (uint8_t i=0; i < NUM_VSTRIPS; i++) {
      VirtualStrip *vstrip = this->vstrips[i];
      if (vstrip->fade == Dead)
        continue;

      // Remember the first strip
      if (first_strip == NULL)
        first_strip = vstrip;
     
      vstrip->update(beat_frame, beat_pulse);
      vstrip->blend(this->led_strip->leds, this->options.brightness, vstrip == first_strip);
    }

    this->effects->update(first_strip, beat_frame, beat_pulse);
    this->effects->draw(this->led_strip->leds, this->num_leds);    
  }

  virtual void acknowledge() {
    addFlash();
  }

  virtual void onCommand(uint8_t fromId, CommandId command, void *data) {
    if (fromId) {
      Serial.print(F("From "));
      Serial.print(fromId);
      Serial.print(F(": "));
    }
    
    switch (command) {
      case COMMAND_FIREWORK:
        Serial.println(F("fireworks"));
        this->acknowledge();
        return;
  
      case COMMAND_RESET:
        Serial.println(F("reset"));
        return;
  
      case COMMAND_BRIGHTNESS: {
        uint8_t *bright = (uint8_t *)data;
        this->setBrightness(*bright);
        return;
      }
  
      case COMMAND_HELLO:
        Serial.println(F("hello"));
        return;
  
      case COMMAND_OPTIONS: {
        Serial.println(F("options"));
        memcpy(&this->options, data, sizeof(this->options));
        this->acknowledge();
        return;
      }
  
      case COMMAND_UPDATE: {
        TubeState state;
        memcpy(&state, data, sizeof(TubeState));
        printState(&state);
  
        if (fromId < this->radio->masterTubeId) {
          Serial.println(F(" (ignoring)"));
          return;
        } 
        Serial.println(F(" (obeying)"));

        // Track the last time we received a message from our master
        this->slaveTimer.start(RADIO_SENDPERIOD * 8);

        this->startPattern(state.pattern, state.palette_id, (SyncMode)state.sync);
        this->beats->sync(state.bpm, state.beat_frame);
        currentState = state;    
        return;
      }
    }
  
    Serial.print(F("UNKNOWN "));
    Serial.println(command, HEX);
  }

  void readSerial() {
    if (!Serial.available())
      return;
      
    char c = Serial.read();
    switch (c) {
      case 'f':
        this->radio->sendCommandFrom(255, COMMAND_FIREWORK, NULL, 0);
        this->onCommand(0, COMMAND_FIREWORK, NULL);
        break;

      case 'i':
        this->radio->resetId();
        break;
      case 'm':
        this->radio->tubeId = 255;
        break;

      case 'd': 
        this->setDebugging(!this->options.debugging);
        break;
      case '-':
        this->setBrightness(this->options.brightness - 5);
        break;
      case '+':
        this->setBrightness(this->options.brightness + 5);
        break;
        
      case '[':
        this->beats->adjust_bpm(-(1<<8));
        break;
      case '{':
        this->beats->adjust_bpm(-(1<<4));
        break;
      case '}':
        this->beats->adjust_bpm(1<<4);
        break;
      case ']':
        this->beats->adjust_bpm(1<<8);
        break;
      case 's':
        this->beats->start_phrase();
        break;

      case 'g':
        addGlitter();
        break;
      case 'G':
        addFlash();
        break;

      case 'n':
        this->nextPattern();
        break;
      case 'p':
        this->nextPalette();
        break;
    }
  }

};

#endif
