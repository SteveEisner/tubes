#ifndef MASTER_H
#define MASTER_H

#include "timer.h"
#include "controller.h"
#include "led_strip.h"

#define X_AXIS_PIN 20
#define Y_AXIS_PIN 21

#define BUTTON_PIN_1   2
#define BUTTON_PIN_2   3
#define BUTTON_PIN_3   4
#define BUTTON_PIN_4   5
#define BUTTON_PIN_5   15


class Master {
  public:
    uint8_t x_axis;
    uint8_t y_axis;
    uint8_t joystick_angle=0;
    bool joystick_active=false;

    uint8_t taps=0;
    Timer tapTimer;
    uint16_t tapTime[16];

    Background background;
    uint8_t palette_mode = false;
    uint8_t palette_id = 0;

    PatternController *controller;
    Button button[5];

  Master(PatternController *controller) {
    this->controller = controller;
  }

  void setup() {
    this->button[0].setup(BUTTON_PIN_1);
    this->button[1].setup(BUTTON_PIN_2);
    this->button[2].setup(BUTTON_PIN_3);
    this->button[3].setup(BUTTON_PIN_4);
    this->button[4].setup(BUTTON_PIN_5);
    Serial.println((char *)F("Master: ok"));
  }

  void update() {
    double x = analogRead(X_AXIS_PIN)-512;
    double y = analogRead(Y_AXIS_PIN)-512;

    // Compensate for my shitty drilling
    if (x < 0) {
      if (x < -430)
        x = -430;
      x = x * 512 / 430;
    }
    this->x_axis = int((x+512) / 4);
    this->y_axis = int((y+512) / 4);
    this->joystick_active = x*x + y*y > 30000; // enough movement 
    if (this->joystick_active) {
      double deg = atan2(x,y);
      if (deg < 0)
        deg = 6.28 + deg;
      deg = (deg*256)/6.28;
      this->joystick_angle = uint8_t(deg-0.5);
    } else {
      this->joystick_angle = 0;
    }

    for (uint8_t i=0; i < 5; i++) {
      if (button[i].triggered()) {
        if (button[i].pressed())
          this->onButtonPress(i);
        else
          this->onButtonRelease(i);
      }
      if (button[i].pressed()) {
        this->onButtonHeld(i);
      }
    }

    if (this->taps && this->tapTimer.since_mark() > 10000) {
      this->taps = 0;
      this->fail();
    }

    this->updateStatus(this->controller, this->controller->led_strip);
  }

  void ok() {
    addFlash(CRGB::Green);
  }

  void fail() {
    addFlash(CRGB::Red);
  }

  uint8_t pos16() {
    uint8_t joy_pos = (this->joystick_angle + 8) >> 4;  // Center on 16s
    return joy_pos % 16;
  }

  uint8_t pos(uint8_t segments) {
    uint16_t per = 65535 / segments;
    
    uint8_t joy_pos = ((this->joystick_angle<<8) + (per/2) ) / segments;
    return joy_pos % segments;
  }

  void onButtonPress(uint8_t button) {
    if (button == 0)
      return;

    if (button == 1) {
      Serial.println((char *)F("Skip >>"));
      this->controller->force_next();
      this->ok();
      return;
    }

    if (button == 3) {
      this->tap();
      return;
    }

    Serial.print((char *)F("Pressed "));
    Serial.println(button);
  }

  void onButtonRelease(uint8_t button) {
    if (button == 2) {
      if (this->palette_mode)
        this->controller->_load_palette(this->palette_id);
      this->palette_mode = false;
    }
    
    if (button == 3) {
      if (this->taps == 0)
        return;
      this->tap();
      return;
    }

    Serial.print((char *)F("Released "));
    Serial.println(button);
  }

  void onButtonHeld(uint8_t button) {
    if (button == 0) {
      uint8_t brightness;
      if (this->y_axis >= 128) {
        brightness = DEFAULT_MASTER_BRIGHTNESS + scale8(255-DEFAULT_MASTER_BRIGHTNESS, (this->y_axis - 128) * 2);
      } else {
        brightness = 32 + scale8(DEFAULT_MASTER_BRIGHTNESS-32, this->y_axis * 2);
      }

      EffectParameters params = this->controller->current_state.effect_params;
      if (this->x_axis < 100 && this->y_axis > 75) {
        params.effect = Glitter;
        params.beat = Eighth;
        params.chance = (this->y_axis - 75);
        params.pen = White;
      } else if (this->x_axis > 150 && this->y_axis > 75) {
        params.effect = Flash;
        params.beat = Eighth;
        params.chance = (this->y_axis - 75) / 4;
        params.pen = White;
      } else if (this->y_axis < 200) {
        params.effect = None;
        params.beat = Continuous;
      }
      
      this->controller->_load_effect(params);
      this->controller->setBrightness(brightness);
      return;
    }

    if (button == 2) {
      this->palette_mode = this->joystick_active;
      if (this->joystick_active) {
        this->palette_id = pos(gGradientPaletteCount);
        Serial.println(this->palette_id);
        this->background.palette = gPalettes[this->palette_id];
      }
    }
  }

  void tap() {
    Serial.println((char *)F("tap"));
    if (!this->taps) {
      // Joystick does a "push to BPM"
      if (this->joystick_active) {
        uint8_t pos16 = this->pos16();
        uint8_t beat16 = (this->controller->beats->frac >> 8) % 16;
        uint16_t frac = this->controller->beats->frac & 0xFF;
        
        if (beat16 == pos16) {
          // Same beat: push back
          this->controller->beats->adjust_bpm(-frac*2);
        } else if ((beat16+1)%16 == pos16) {
          // One beat behind: push forward
          this->controller->beats->adjust_bpm((255-frac) * 2);
        }
        this->controller->set_phrase_position(pos16);
        this->controller->beats->print_bpm();
        this->ok();
        return;
      }
      this->tapTimer.start(0);
    }

    uint32_t time = this->tapTimer.since_mark();
    this->tapTime[this->taps++] = time;

    uint32_t bpm = 0;
    if (this->taps > 4) {
      // Can study this later to make BPM detection better

      bpm = 60000*256*(this->taps-1) / time;  // 120 beats per min = 500ms per beat
      if (bpm < 70*256)
        bpm *= 2;
      else if (bpm > 140*256)
        bpm /= 2;
    }
    
    if (this->taps == 16) {
      this->taps = 0;
      this->controller->set_tapped_bpm(bpm);
      this->ok();
    }
  }

  void updateStatus(PatternController *controller, LEDs *strip) {
    if (this->taps) {
      this->displayProgress(this->taps);
    } else if (this->palette_mode) {
      this->displayPalette(this->background);
    } else {
      uint8_t beat_pos = (controller->current_state.beat_frame >> 8) % 16;
      strip->leds[beat_pos] = CRGB::White;      
    }

    if (this->joystick_active) {
      strip->leds[this->pos16()] = CRGB::Green;
    }
  }

  void displayProgress(uint8_t progress) {
    fill_solid(this->controller->led_strip->leds, 16, CRGB::Black);
    fill_solid(this->controller->led_strip->leds, progress % 16, CRGB(128,128,128));
  }

  void displayPalette(Background &background) {
    for (int i = 0; i < 16; i++) {
      CRGB color = ColorFromPalette(background.palette, i * 16 );
      this->controller->led_strip->leds[i] = color;
    }
  }

};

#endif
