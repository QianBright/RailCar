#include <Arduino.h>

#include <analogWrite.h>

#define breakPin 18    // yellow
#define pwmPin 19      // white
#define reversalPin 21 // green

void setup()
{
    Serial.begin(115200);
    pinMode(breakPin, OUTPUT);
    pinMode(pwmPin, OUTPUT);
    pinMode(reversalPin, OUTPUT);
    digitalWrite(breakPin, 1);
    digitalWrite(reversalPin, 0);
}

void loop()
{
    //  digitalWrite(pwmPin, HIGH); // sets the pin on
    //  delayMicroseconds(10);      // pauses for 50 microseconds
    //  digitalWrite(pwmPin, LOW);  // sets the pin off
    //  delayMicroseconds(40);
    analogWrite(pwmPin, 200);
    // 20-30KHZ
    delayMicroseconds(floor(1e6 / 3e4));
}

for (int i = 200; i < 256; i++)
{
    analogWrite(motorPin, i);
    delay(delayTime);
}
