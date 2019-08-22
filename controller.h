#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "beats.h"

#include "pattern.h"
#include "palette.h"
#include "effects.h"
#include "global_state.h"


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
const static CommandId COMMAND_NEXT = 0x321;
const static CommandId COMMAND_RESET = 0x911;
const static CommandId COMMAND_FIREWORK = 0xFFF;
const static CommandId COMMAND_HELLO = 0x000;
const static CommandId COMMAND_OPTIONS = 0x123;
const static CommandId COMMAND_BRIGHTNESS = 0x888;


typedef struct {
  bool debugging;
  uint8_t brightness;
} ControllerOptions;

#define NEXT_PATTERN_TIME 53000
#define NEXT_PALETTE_TIME 27000

#define NUM_VSTRIPS 3

class PatternController : public MessageReceiver {
  public:
    const static int FRAMES_PER_SECOND = 300;  // how often we animate, in frames per second
    const static int REFRESH_PERIOD = 1000 / FRAMES_PER_SECOND;  // how often we animate, in milliseconds

    uint8_t num_leds;
    VirtualStrip *vstrips[NUM_VSTRIPS];
    uint8_t next_vstrip = 0;
    
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
    char key_buffer[20] = {0};

    Energy energy=LowEnergy;
    TubeState current_state;
    TubeState next_state;

  PatternController(uint8_t num_leds, BeatController *beats, Radio *radio) {
    this->num_leds = num_leds;
    this->lcd = new Lcd();
    this->led_strip = new LEDs(num_leds);
    this->beats = beats;
    this->radio = radio;
    this->effects = new Effects();

    for (uint8_t i=0; i < NUM_VSTRIPS; i++) {
#ifdef DOUBLED
      this->vstrips[i] = new VirtualStrip(num_leds * 2 + 1);
#else
      this->vstrips[i] = new VirtualStrip(num_leds);
#endif
    }
  }
  
  void setup()
  {
#ifdef MASTERCONTROL
    this->options.debugging = true;
#else
    this->options.debugging = false;
#endif
    this->options.brightness = DEFAULT_MASTER_BRIGHTNESS;
    
    this->lcd->setup();
    this->led_strip->setup();
    Serial.println(F("Graphics: ok"));

    this->set_next_pattern(0);
    this->set_next_palette(0);
    this->set_next_effect(0);
    this->next_state.pattern_phrase = 0;
    this->next_state.palette_phrase = 0;
    this->next_state.effect_phrase = 0;
    Serial.println(F("Patterns: ok"));

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
        this->setDebugging(1);
      }
      this->beats->set_bpm(DEFAULT_BPM << 8);
    } else if (this->x_axis < 30)
      if (this->options.debugging) {
        this->setDebugging(0);
      }
#endif

    this->read_keys();

    // If master has expired, clear masterId
    if (this->radio->masterTubeId && this->slaveTimer.ended()) {
      Serial.println(F("I have no master"));
      this->radio->masterTubeId = 0;
    }

    // Update patterns to the beat
    this->update_beat();

    uint16_t phrase = this->current_state.beat_frame >> 12;
    if (phrase >= this->next_state.pattern_phrase) {
      this->load_pattern(this->next_state);
      this->next_state.pattern_phrase = phrase + this->set_next_pattern(phrase);
    }
    if (phrase >= this->next_state.palette_phrase) {
      this->load_palette(this->next_state);
      this->next_state.palette_phrase = phrase + this->set_next_palette(phrase);
    }
    if (phrase >= this->next_state.effect_phrase) {
      this->load_effect(this->next_state);
      this->next_state.effect_phrase = phrase + this->set_next_effect(phrase);
    }

    // If alone or master, send out updates
    if (!this->radio->masterTubeId and this->updateTimer.ended()) {
      this->send_update();
    }

    this->radio->receiveCommands(this);

    if (this->graphicsTimer.every(REFRESH_PERIOD)) {
      this->updateGraphics();
    }

    if (this->lcd->active) {
      this->lcd->size(1);
      this->lcd->write(0,56, this->current_state.beat_frame);
      this->lcd->write(80,56, this->x_axis);
      this->lcd->write(100,56, this->y_axis);
      this->lcd->show();

      this->lcd->update();
    }
  }

  void update_beat() {
    this->current_state.bpm = this->next_state.bpm = this->beats->bpm;
    this->current_state.beat_frame = particle_beat_frame = this->beats->frac;  // (particle_beat_frame is a hack)
    if (this->current_state.bpm >= 125>>8)
      this->energy = HighEnergy;
    else if (this->current_state.bpm >= 122>>8)
      this->energy = MediumEnergy;
    else
      this->energy = LowEnergy;    
  }
  
  void send_update() {
    this->current_state.print();
    Serial.print(F(" "));

    if (this->radio->sendCommand(COMMAND_UPDATE, &this->current_state, sizeof(this->current_state))) {
      this->radio->radioFailures = 0;
      this->updateTimer.snooze(RADIO_SENDPERIOD);
    } else {
      // might have been a collision.  Back off by a small amount determined by ID
      Serial.println(F("Radio update failed"));
      this->updateTimer.snooze( this->radio->tubeId & 0x7F );
      this->radio->radioFailures++;
      if (this->radio->radioFailures > 100) {
        this->radio->setup();
        this->radio->radioRestarts++;
      }
    }

    uint16_t phrase = this->current_state.beat_frame >> 12;
    Serial.print(F("    "));
    Serial.print(this->next_state.pattern_phrase - phrase);
    Serial.print(F("P "));
    Serial.print(this->next_state.palette_phrase - phrase);
    Serial.print(F("C "));
    Serial.print(this->next_state.effect_phrase - phrase);
    Serial.print(F("E: "));
    this->next_state.print();
    Serial.print(F(" "));
    this->radio->sendCommand(COMMAND_NEXT, &this->next_state, sizeof(this->next_state));
    Serial.println();    
  }

  void background_changed() {
    this->update_background();
    this->current_state.print();
    Serial.println();
  }

  void load_pattern(TubeState &tube_state) {
    if (this->current_state.pattern_id == tube_state.pattern_id 
        && this->current_state.pattern_sync_id == tube_state.pattern_sync_id)
      return;

    this->current_state.pattern_phrase = tube_state.pattern_phrase;
    this->current_state.pattern_id = tube_state.pattern_id % gPatternCount;
    this->current_state.pattern_sync_id = tube_state.pattern_sync_id;

    Serial.print(F("Change pattern "));
    this->background_changed();
  }

  uint16_t set_next_pattern(uint16_t phrase) {
    uint8_t pattern_id = random8(gPatternCount);
    PatternDef def = gPatterns[pattern_id];
    if (def.control.energy > this->energy) {
      pattern_id = 0;
      def = gPatterns[0];
    }

    this->next_state.pattern_id = pattern_id;
    this->next_state.pattern_sync_id = this->randomSyncMode();

    switch (def.control.duration) {
      case ShortDuration: return random8(5,15);
      case MediumDuration: return random8(15,25);
      case LongDuration: return random8(35,45);
      case ExtraLongDuration: return random8(70, 100);
    }
    return 5;
  }

  void load_palette(TubeState &tube_state) {
    if (this->current_state.palette_id == tube_state.palette_id)
      return;

    this->current_state.palette_phrase = tube_state.palette_phrase;
    this->current_state.palette_id = tube_state.palette_id % gGradientPaletteCount;

    Serial.print(F("Change palette "));
    this->background_changed();
  }

  uint16_t set_next_palette(uint16_t phrase) {
    this->next_state.palette_id = random8(gGradientPaletteCount);
    return random8(4,40);
  }

  void load_effect(TubeState &tube_state) {
    if (this->current_state.effect_params.effect == tube_state.effect_params.effect && 
        this->current_state.effect_params.pen == tube_state.effect_params.pen && 
        this->current_state.effect_params.chance == tube_state.effect_params.chance)
      return;

    this->current_state.effect_params = tube_state.effect_params;
    this->current_state.palette_id = tube_state.palette_id % gGradientPaletteCount;
  
    Serial.print(F("Change effect "));
    this->current_state.print();
    Serial.println();
    
    this->effects->load(this->current_state.effect_params);
  }

  uint16_t set_next_effect(uint16_t phrase) {
    EffectDef def = gEffects[random8(gEffectCount)];
    if (random8() < 200 || def.control.energy > this->energy)
      def = gEffects[0];

    this->next_state.effect_params = def.params;

    switch (def.control.duration) {
      case ShortDuration: return 3;
      case MediumDuration: return 6;
      case LongDuration: return 10;
      case ExtraLongDuration: return 20;
    }
    return 1;
  }

  void update_background() {
    Background background;
    background.animate = gPatterns[this->current_state.pattern_id].backgroundFn;
    background.palette = gPalettes[this->current_state.palette_id];
    background.sync = (SyncMode)this->current_state.pattern_sync_id;

    // re-use virtual strips to prevent heap fragmentation
    for (uint8_t i = 0; i < NUM_VSTRIPS; i++) {
      this->vstrips[i]->fadeOut();
    }
    this->vstrips[this->next_vstrip]->load(background);
    this->next_vstrip = (this->next_vstrip + 1) % NUM_VSTRIPS; 
  }

  void onButton(uint8_t button) {
    if (button == 1) {
      Serial.print(F("Button "));
      Serial.println(button);
      this->radio->sendCommand(COMMAND_FIREWORK, NULL, 0);
      return;
    }
  }

  void optionsChanged() {
    if (this->radio->tubeId >= 250) {
      this->radio->sendCommand(COMMAND_OPTIONS, &options, sizeof(options));
    }
  }

  void setBrightness(uint8_t brightness) {
    Serial.print(F("brightness "));
    Serial.println(brightness);

    this->options.brightness = brightness;
    this->optionsChanged();
  }

  void setDebugging(bool debugging) {
    Serial.print(F("debugging "));
    Serial.println(debugging);

    this->options.debugging = debugging;
    this->optionsChanged();
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

  void updateGraphics() {
    static BeatFrame_24_8 lastFrame = 0;
    BeatFrame_24_8 beat_frame = this->current_state.beat_frame;

    uint8_t beat_pulse = 0;
    for (int i = 0; i < 8; i++) {
      if ( (beat_frame >> (5+i)) != (lastFrame >> (5+i)))
        beat_pulse |= 1<<i;
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
      vstrip->blend(this->led_strip->leds, this->led_strip->num_leds, this->options.brightness, vstrip == first_strip);
    }

    this->effects->update(first_strip, beat_frame, (BeatPulse)beat_pulse);
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
        this->updateTimer.stop();
        return;
  
      case COMMAND_OPTIONS: {
        Serial.println(F("options"));
        memcpy(&this->options, data, sizeof(this->options));
        this->acknowledge();
        return;
      }

      case COMMAND_NEXT: {
        Serial.print(F(" next "));
        if (fromId < this->radio->masterTubeId) {
          Serial.println(F(" (ignoring)"));
          return;
        } 

        memcpy(&this->next_state, data, sizeof(TubeState));
        this->next_state.print();
        Serial.println(F(" (obeying)"));
        return;
      }
  
      case COMMAND_UPDATE: {
        Serial.print(F(" update "));
        if (fromId < this->radio->masterTubeId) {
          Serial.println(F(" (ignoring)"));
          return;
        } 

        TubeState state;
        memcpy(&state, data, sizeof(TubeState));
        state.print();
        Serial.println(F(" (obeying)"));
  
        // Track the last time we received a message from our master
        this->slaveTimer.start(RADIO_SENDPERIOD * 8);

        // Catch up to this state
        this->load_pattern(state);
        this->load_palette(state);
        this->load_effect(state);
        this->beats->sync(state.bpm, state.beat_frame);
        return;
      }
    }
  
    Serial.print(F("UNKNOWN "));
    Serial.println(command, HEX);
  }

  void read_keys() {
    if (!Serial.available())
      return;
      
    char c = Serial.read();
    char *k = this->key_buffer;
    uint8_t max = sizeof(this->key_buffer);
    for (uint8_t i=0; *k && (i < max-1); i++) {
      k++;
    }
    if (c == 10) {
      this->keyboard_command(this->key_buffer);
      this->key_buffer[0] = 0;
    } else {
      *k++ = c;
      *k = 0;    
    }
  }

  accum88 parse_number(char *s) {
    uint16_t n=0, d=0;
    
    while (*s == ' ')
      s++;
    while (*s) {
      if (*s < '0' || *s > '9')
        break;
      n = n*10 + (*s++ - '0');
    }
    n = n << 8;
    
    if (*s == '.') {
      uint16_t div = 1;
      s++;
      while (*s) {
        if (*s < '0' || *s > '9')
          break;
        d = d*10 + (*s++ - '0');
        div *= 10;
      }
      d = (d << 8) / div;
    }
    return n+d;
  }

  void keyboard_command(char *command) {
    uint8_t b;
    accum88 arg = this->parse_number(command+1);
    
    switch (command[0]) {
      case 'f':
        this->radio->sendCommandFrom(255, COMMAND_FIREWORK, NULL, 0);
        this->onCommand(0, COMMAND_FIREWORK, NULL);
        break;

      case 'i':
        this->radio->resetId(arg >> 8);
        break;

      case 'd':
        this->setDebugging(!this->options.debugging);
        break;
      
      case '-':
        b = this->options.brightness;
        while (*command++ == '-')
          b -= 5;
        this->setBrightness(b - 5);
        break;
      case '+':
        b = this->options.brightness;
        while (*command++ == '+')
          b += 5;
        this->setBrightness(b + 5);
        return;
      case 'l':
        if (arg < 5*256) {
          Serial.println(F("nope"));
          return;
        }
        this->setBrightness(arg >> 8);
        return;

      case 'b':
        if (arg < 60*256) {
          Serial.println(F("nope"));
          return;
        }
        this->beats->set_bpm(arg);
        this->update_beat();
        this->send_update();
        return;

      case 's':
        this->beats->start_phrase();
        this->update_beat();
        this->send_update();
        return;

      case 'p':
        this->next_state.pattern_phrase = 0;
        this->next_state.pattern_id = arg >> 8;
        this->next_state.pattern_sync_id = All;
        this->radio->sendCommand(COMMAND_NEXT, &this->next_state, sizeof(this->next_state));
        return;        
        
      case 'm':
        this->next_state.pattern_phrase = 0;
        this->next_state.pattern_id = this->current_state.pattern_id;
        this->next_state.pattern_sync_id = arg >> 8;
        this->radio->sendCommand(COMMAND_NEXT, &this->next_state, sizeof(this->next_state));
        return;
        
      case 'c':
        this->next_state.palette_phrase = 0;
        this->next_state.palette_id = arg >> 8;
        this->radio->sendCommand(COMMAND_NEXT, &this->next_state, sizeof(this->next_state));
        return;
        
      case 'e':
        this->next_state.effect_phrase = 0;
        this->next_state.effect_params = gEffects[(arg >> 8) % gEffectCount].params;
        this->radio->sendCommand(COMMAND_NEXT, &this->next_state, sizeof(this->next_state));
        return;

      case '%':
        this->next_state.effect_phrase = 0;
        this->next_state.effect_params = this->current_state.effect_params;
        this->next_state.effect_params.chance = arg;
        this->radio->sendCommand(COMMAND_NEXT, &this->next_state, sizeof(this->next_state));
        return;

      case 'h':
        // Pretend to receive a HELLO
        this->onCommand(0, COMMAND_HELLO, NULL);
        return;

      case 'g':
        for (int i=0; i< 10; i++)
          addGlitter();
        break;

      case '?':
        Serial.println(F("b###.# - set bpm"));
        Serial.println(F("s - start phrase"));
        Serial.println();
        Serial.println(F("p### - patterns"));
        Serial.println(F("m### - sync mode"));
        Serial.println(F("c### - colors"));
        Serial.println(F("e### - effects"));
        Serial.println();
        Serial.println(F("i### - set ID"));
        Serial.println(F("d - toggle debugging"));
        Serial.println(F("l### - brightness"));
    }
  }

};



// What's interesting?
// c53 - clouds
// m4 - swing drift


#endif
