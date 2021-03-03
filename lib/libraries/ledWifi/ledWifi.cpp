#include <Arduino.h>
#include "ledWifi.h"

#include <analogWrite.h>

ledWifi::ledWifi(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  _pin = pin;
}

void ledWifi::flash() {
  digitalWrite(_pin, HIGH);
  delay(100);
  digitalWrite(_pin, LOW);
  delay(100);
}

void ledWifi::analog(int Pwm){
  analogWrite(_pin, Pwm);
}
