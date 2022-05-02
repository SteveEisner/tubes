#include "platform_config.h"
#include "options.h"

// #define MASTERCONTROL

#define MASTER_PIN 6
#define NUM_LEDS 64

#ifdef IS_TEENSY
#define USERADIO
#endif

#include "beats.h"
#include "virtual_strip.h"

#include "controller.h"
#include "master.h"
#include "radio.h"
#include "debug.h"


BeatController beats;
Radio radio;
PatternController controller(NUM_LEDS, &beats, &radio);
DebugController debug(&controller);
Master *master = NULL;


void randomize(long seed) {
  for (int i = 0; i < seed % 16; i++) {
    randomSeed(random());
  }
  random16_add_entropy( random() );  
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  randomize(analogRead(0));

  pinMode(MASTER_PIN, INPUT_PULLUP);
  if (digitalRead(MASTER_PIN) == LOW) {
    master = new Master(&controller);
    master->setup();
  }

  // Start timing
  globalTimer.setup();
  beats.setup();
  controller.setup(master != NULL);
  debug.setup();
}


void loop()
{
  EVERY_N_MILLISECONDS( 1000 ) {
    randomize(random());
  }

  beats.update(); // ~30us
  controller.update(); // radio: 0-3000us   patterns: 0-3000us   lcd: ~50000us
  debug.update(); // ~25us
  if (master)
    master->update();

  // Draw after everything else is done
  controller.led_strip->update(master != NULL); // ~25us
}
