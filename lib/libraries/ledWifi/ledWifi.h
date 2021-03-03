#ifndef ledWifi_h
#define ledWifi_h

#include <Arduino.h>

class ledWifi{
  public:
    ledWifi(int pin);
    void flash();
    void analog(int Pwm);
  private:
    int _pin;
};


#endif
