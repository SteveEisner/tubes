#ifndef TIMER_H
#define TIMER_H

class GlobalTimer {
  public:
    uint32_t now_millis;
    uint32_t now_micros;
    uint32_t last_micros;
    uint32_t last_millis;
    uint32_t delta_micros;
    uint32_t delta_millis;

  void setup()
  {
    this->last_millis = this->now_millis = millis();
    this->last_micros = this->now_micros = micros();
  }

  void update()
  {
    this->last_millis = this->now_millis;
    this->now_millis = millis();
    this->delta_millis = this->now_millis - this->last_millis;
    
    this->last_micros = this->now_micros;
    this->now_micros = micros();
    this->delta_micros = this->now_micros - this->last_micros;
  }
};

GlobalTimer globalTimer;

#endif
